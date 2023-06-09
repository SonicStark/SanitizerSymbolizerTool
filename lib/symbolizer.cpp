//===-- symbolizer.cpp ----------------------------------------------------===//
//
// Based on the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file gives implementation of some common basic classes, structs
// and functions for launching and using a symbolizer. Some of them
// are platform specific.
//===----------------------------------------------------------------------===//

#include "symbolizer.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#if SANITIZER_POSIX

#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#else // SANITIZER_POSIX
#error ONLY SUPPORT POSIX NOW
#endif  // SANITIZER_POSIX

namespace SANSYMTOOL_NS
{

SymbolizerProcess::SymbolizerProcess(const char *path, bool use_posix_spawn)
    : active_pid_(-1),
      path_(path),
      input_fd_(kInvalidFd),
      output_fd_(kInvalidFd),
      times_restarted_(0),
      failed_to_start_(false),
      reported_invalid_path_(false),
      use_posix_spawn_(use_posix_spawn) {
  CHECK(path_);
  CHECK_NE(path_[0], '\0');
}

static bool IsSameModule(const char* path) {
  // Sanitizer may be used for symbolizer itself.
  // This will never happen for our tool.
  return false;
}

const char *SymbolizerProcess::SendCommand(const char *command) {
  if (failed_to_start_)
    return nullptr;
  if (IsSameModule(path_)) {
    SAYSTH("WARNING: Symbolizer was blocked from starting itself!\n");
    failed_to_start_ = true;
    return nullptr;
  }
  for (; times_restarted_ < kMaxTimesRestarted; times_restarted_++) {
    // Start or restart symbolizer if we failed to send command to it.
    if (const char *res = SendCommandImpl(command))
      return res;
    Restart();
  }
  if (!failed_to_start_) {
    SAYSTH("WARNING: Failed to use and restart external symbolizer!\n");
    failed_to_start_ = true;
  }
  return nullptr;
}

const char *SymbolizerProcess::SendCommandImpl(const char *command) {
  if (input_fd_ == kInvalidFd || output_fd_ == kInvalidFd)
      return nullptr;
  if (!WriteToSymbolizer(command, std::strlen(command)))
      return nullptr;
  if (!ReadFromSymbolizer())
    return nullptr;
  return buffer_.data();
}

bool SymbolizerProcess::Restart() {
  // Kill anyway to prevent zombies
  if (IsRunning()) Kill();

  if (input_fd_ != kInvalidFd)
    CloseFile(input_fd_);
  if (output_fd_ != kInvalidFd)
    CloseFile(output_fd_);
  return StartSymbolizerSubprocess();
}

proc_id_t SymbolizerProcess::GetPID() { return active_pid_; }

bool SymbolizerProcess::ReadFromSymbolizer() {
  buffer_.clear();
  constexpr uptr max_length = 1024;
  bool ret = true;
  do {
    uptr just_read = 0;
    uptr size_before = buffer_.size();
    buffer_.resize(size_before + max_length);
    buffer_.resize(buffer_.capacity());
    bool ret = ReadFromFile(input_fd_, &buffer_[size_before],
                            buffer_.size() - size_before, &just_read);

    if (!ret)
      just_read = 0;

    buffer_.resize(size_before + just_read);

    // We can't read 0 bytes, as we don't expect external symbolizer to close
    // its stdout.
    if (just_read == 0) {
      SAYSTH("WARNING: Can't read from symbolizer");
      std::fprintf(stderr, "(at fd %d)\n", input_fd_);
      ret = false;
      break;
    }
  } while (!ReachedEndOfOutput(buffer_.data(), buffer_.size()));
  buffer_.push_back('\0');
  return ret;
}

bool SymbolizerProcess::WriteToSymbolizer(const char *buffer, uptr length) {
  if (length == 0)
    return true;
  uptr write_len = 0;
  bool success = WriteToFile(output_fd_, buffer, length, &write_len);
  if (!success || write_len != length) {
    SAYSTH("WARNING: Can't write to symbolizer");
    std::fprintf(stderr, "(at fd %d)\n", output_fd_);
    return false;
  }
  return true;
}

#if SANITIZER_POSIX
/* POSIX-specific implementation of symbolizer parts */
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

bool CreateTwoHighNumberedPipes(int *infd_, int *outfd_) {
  int *infd = NULL;
  int *outfd = NULL;
  // The client program may close its stdin and/or stdout and/or stderr
  // thus allowing socketpair to reuse file descriptors 0, 1 or 2.
  // In this case the communication between the forked processes may be
  // broken if either the parent or the child tries to close or duplicate
  // these descriptors. The loop below produces two pairs of file
  // descriptors, each greater than 2 (stderr).
  int sock_pair[5][2];
  for (int i = 0; i < 5; i++) {
    if (pipe(sock_pair[i]) == -1) {
      for (int j = 0; j < i; j++) {
        close(sock_pair[j][0]);
        close(sock_pair[j][1]);
      }
      return false;
    } else if (sock_pair[i][0] > 2 && sock_pair[i][1] > 2) {
      if (infd == NULL) {
        infd = sock_pair[i];
      } else {
        outfd = sock_pair[i];
        for (int j = 0; j < i; j++) {
          if (sock_pair[j] == infd) continue;
          close(sock_pair[j][0]);
          close(sock_pair[j][1]);
        }
        break;
      }
    }
  }
  CHECK(infd);
  CHECK(outfd);
  infd_[0] = infd[0];
  infd_[1] = infd[1];
  outfd_[0] = outfd[0];
  outfd_[1] = outfd[1];
  return true;
}

// Starts a subprocess and returs its pid.
// Originally declared in sanitizer_file.h.
// Here is its POSIX implementation with envs copied.
// If *_fd parameters are not kInvalidFd their corresponding input/output
// streams will be redirect to the file.
// The files will always be closed in parent process even in case of an error.
// The child process will close all fds after STDERR_FILENO
// before passing control to a program.
pid_t StartSubprocess(const char *program, const char *const argv[], 
                      fd_t stdin_fd  = kInvalidFd,
                      fd_t stdout_fd = kInvalidFd,
                      fd_t stderr_fd = kInvalidFd) {
  auto file_closer = at_scope_exit([&] {
    if (stdin_fd != kInvalidFd) {
      close(stdin_fd);
    }
    if (stdout_fd != kInvalidFd) {
      close(stdout_fd);
    }
    if (stderr_fd != kInvalidFd) {
      close(stderr_fd);
    }
  });

  int pid = fork();

  if (pid < 0) {
    SAYSTH("WARNING: failed to fork");
    std::fprintf(stderr, "(with errno %d)\n", errno);
    return pid;
  }

  if (pid == 0) {
    /** CHILD PROCESS **/

    // Let the child act according to their own ideas.
    // Many programs prefer to ignore SIGPIPE and deal
    // with EPIPE by themselves, such as AFL++.
    // But we shouldn't impose this on the child.
    // One more thing, SanSymTool library itself doesn't
    // handle SIGPIPE - once child process terminates, 
    // if the program using SanSymTool ignores or handles
    // SIGPIPE, a Restart of symbolizer may kick in, 
    // otherwise the program would die of it.
    struct sigaction sa;
    std::memset((char *)&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGPIPE, &sa, NULL);

    // Isolate child process.
    // Taking AFL++ as an example, it handles SIGHUP,
    // SIGINT and SIGTERM, ignores SIGPIPE.
    // If we use our SanSymTool inside it and a SIGINT
    // arrives, without setsid the symbolizer child
    // process will terminate but afl-fuzz still run as-is.
    // Then if SendCommand is called, EPIPE will be here
    // and Restart will kick in.
    setsid();

    if (stdin_fd != kInvalidFd) {
      dup2(stdin_fd, STDIN_FILENO);
      close(stdin_fd);
    }
    if (stdout_fd != kInvalidFd) {
      dup2(stdout_fd, STDOUT_FILENO);
      close(stdout_fd);
    }
    if (stderr_fd != kInvalidFd) {
      dup2(stderr_fd, STDERR_FILENO);
      close(stderr_fd);
    }

    for (int fd = sysconf(_SC_OPEN_MAX); fd > 2; fd--) close(fd);

    execv(program, const_cast<char **>(&argv[0]));

    std::exit(1);
  }

  return pid;
}

// Send SIGKILL to a child process and wait for it
bool KillChildProcess(pid_t pid) {
  // check its status first
  pid_t waitpid_status = waitpid(pid, 0, WNOHANG);
  if (waitpid_status == 0) { return true; }
  if (waitpid_status <  0) {
    if (errno == ECHILD) { return false; }
    else {
      SAYSTH("Waiting on child process failed");
      std::fprintf(stderr, "(with errno %d)\n", errno);
      return false;
    }
  }

  int kill_status = kill(pid, SIGKILL);
  if (kill_status < 0) {
    SAYSTH("Sending SIGKILL failed");
    std::fprintf(stderr, "(with errno %d)\n", errno);
    return false;
  } else {
    // Wait until it was killed.
    // May blocked here for a while.
    waitpid(pid, 0, 0);
    return true;
  }
}

// Checks if specified process is still running
// Originally declared in sanitizer_file.h.
bool IsProcessRunning(pid_t pid) {
  int process_status;
  pid_t waitpid_status = waitpid(pid, &process_status, WNOHANG);
  if (waitpid_status < 0) {
    SAYSTH("Waiting on the process failed");
    std::fprintf(stderr, "(with errno %d)\n", errno);
    return false;
  }
  return waitpid_status == 0;
}

// Waits for the process to finish and returns its exit code.
// Returns -1 in case of an error. Originally declared in sanitizer_file.h.
int WaitForProcess(pid_t pid) {
  int process_status;
  pid_t waitpid_status = waitpid(pid, &process_status, 0);
  if (waitpid_status < 0) {
    SAYSTH("Waiting on the process failed");
    std::fprintf(stderr, "(with errno %d)\n", errno);
    return -1;
  }
  return process_status;
}

bool SymbolizerProcess::StartSymbolizerSubprocess() {
  if (!FileExists(path_)) {
    if (!reported_invalid_path_) {
      SAYSTH("WARNING: invalid path to external symbolizer!\n");
      reported_invalid_path_ = true;
    }
    return false;
  }

  const char *argv[kArgVMax];
  GetArgV(path_, argv);
  pid_t pid;

  // Report how symbolizer is being launched for debugging purposes.
#if SANSYMTOOL_DBG_START_SUBPROCESS
  // Only use `SAYSTH` for first line
  SAYSTH("Launching Symbolizer process: \n");
  for (unsigned index = 0; index < kArgVMax && argv[index]; ++index)
    std::fprintf(stderr, "%s ", argv[index]);
  std::fprintf(stderr, "\n");
#endif

  if (use_posix_spawn_) {
    UNIMPLEMENTED(); // we don't support this since getting env is complex
  } else {
    fd_t infd[2] = {}, outfd[2] = {};
    if (!CreateTwoHighNumberedPipes(infd, outfd)) {
      SAYSTH("WARNING: Can't create a socket pair to start "
             "external symbolizer");
      std::fprintf(stderr, "(with errno: %d)\n", errno);
      return false;
    }

    pid = StartSubprocess(path_, argv, /* stdin */ outfd[0], /* stdout */ infd[1]);
    if (pid < 0) {
      close(infd[0]);
      close(outfd[1]);
      return false;
    }

    input_fd_ = infd[0];
    output_fd_ = outfd[1];
  }

  CHECK_GT(pid, 0);

  // Check that symbolizer subprocess started successfully.
  usleep((u64)kSymbolizerStartupTimeMillis * 1000);
  if (!IsProcessRunning(pid)) {
    // Either waitpid failed, or child has already exited.
    SAYSTH("WARNING: external symbolizer didn't start up correctly!\n");
    return false;
  }
  
  active_pid_ = pid;
  return true;
}

#else // SANITIZER_POSIX
#error ONLY SUPPORT POSIX NOW
#endif  // SANITIZER_POSIX

bool SymbolizerProcess::IsRunning() {
  if (-1 == active_pid_) //no active process
  { return false; }
  return IsProcessRunning(active_pid_);
}

bool SymbolizerProcess::Kill() {
  if (-1 == active_pid_) //no active process
  { return false; }
  if (!KillChildProcess(active_pid_)) {
    SAYSTH("WARNING: external symbolizer didn't killed correctly!\n");
    return false;
  } else {
    active_pid_ = -1;
    if (input_fd_ != kInvalidFd)
      CloseFile(input_fd_);
    if (output_fd_ != kInvalidFd)
      CloseFile(output_fd_);
    return true;
  }
}

const char *ExtractToken(const char *str, const char *delims, char **result) {
  std::size_t prefix_len = std::strcspn(str, delims);
  *result = (char*)std::malloc(prefix_len + 1);
  std::memcpy(*result, str, prefix_len);
  (*result)[prefix_len] = '\0';
  const char *prefix_end = str + prefix_len;
  if (*prefix_end != '\0') prefix_end++;
  return prefix_end;
}

const char *ExtractInt(const char *str, const char *delims, int *result) {
  char *buff = nullptr;
  const char *ret = ExtractToken(str, delims, &buff);
  if (buff) {
    *result = (int)std::atoll(buff);
  }
  std::free(buff);
  return ret;
}

const char *ExtractUptr(const char *str, const char *delims, uptr *result) {
  char *buff = nullptr;
  const char *ret = ExtractToken(str, delims, &buff);
  if (buff) {
    *result = (uptr)std::atoll(buff);
  }
  std::free(buff);
  return ret;
}

const char *ExtractSptr(const char *str, const char *delims, sptr *result) {
  char *buff = nullptr;
  const char *ret = ExtractToken(str, delims, &buff);
  if (buff) {
    *result = (sptr)std::atoll(buff);
  }
  std::free(buff);
  return ret;
}

const char *ExtractTokenUpToDelimiter(const char *str, const char *delimiter,
                                      char **result) {
  const char *found_delimiter = std::strstr(str, delimiter);
  uptr prefix_len =
      found_delimiter ? found_delimiter - str : std::strlen(str);
  *result = (char *)std::malloc(prefix_len + 1);
  std::memcpy(*result, str, prefix_len);
  (*result)[prefix_len] = '\0';
  const char *prefix_end = str + prefix_len;
  if (*prefix_end != '\0') prefix_end += std::strlen(delimiter);
  return prefix_end;
}

} // namespace SANSYMTOOL_NS