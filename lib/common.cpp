//===-- common.cpp --------------------------------------------------------===//
//
// Based on the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file gives implementation of some basic functions,
// and some of them are platform specific.
//===----------------------------------------------------------------------===//

#include "common.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#if SANITIZER_POSIX

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#else // SANITIZER_POSIX
#error ONLY SUPPORT POSIX NOW
#endif // SANITIZER_POSIX

namespace SANSYMTOOL_NS
{

void NORETURN Die() { std::exit(SANSYMTOOL_EXITCODE); }

void NORETURN CheckFailed(const char *file, int line, const char *cond,
                          u64 v1, u64 v2) {
    std::fprintf(stderr, SANSYMTOOL_MYNAME ": CHECK failed: %s:%d \"%s\" (0x%zx, 0x%zx)\n",
            file, line, cond, v1, v2);
    Die();
}

void SaySth(const char *file, int line, const char *sth) {
    std::fprintf(stderr, SANSYMTOOL_MYNAME ": (%s:%d) \"%s\" ", file, line, sth);
}

const char *StripModuleName(const char *module) {
  if (!module)
    return nullptr;
  if (SANITIZER_WINDOWS) {
    // On Windows, both slash and backslash are possible.
    // Pick the one that goes last.
    if (const char *bslash_pos = std::strrchr(module, '\\'))
      return StripModuleName(bslash_pos + 1);
  }
  if (const char *slash_pos = std::strrchr(module, '/')) {
    return slash_pos + 1;
  }
  return module;
}

#if SANITIZER_POSIX

/* POSIX-specific implementation for file I/O */

fd_t OpenFile(const char *filename, FileAccessMode mode, error_t *errno_p) {
  int flags;
  switch (mode) {
    case RdOnly: flags = O_RDONLY; break;
    case WrOnly: flags = O_WRONLY | O_CREAT | O_TRUNC; break;
    case RdWr: flags = O_RDWR | O_CREAT; break;
  }
  fd_t res = open(filename, flags, 0660);
  if (res < 0) {
    *errno_p = errno;
    return kInvalidFd;
  } else {
    return res;
  }
}

void CloseFile(fd_t fd) {
  close(fd);
}

bool ReadFromFile(fd_t fd, void *buff, uptr buff_size, uptr *bytes_read,
                  error_t *error_p) {
  ssize_t res = read(fd, buff, buff_size);
  if (res < 0) {
    *error_p = errno;
    return false;
  } else {
    if (bytes_read) *bytes_read = res;
    return true;
  }
}

bool WriteToFile(fd_t fd, const void *buff, uptr buff_size, uptr *bytes_written,
                 error_t *error_p) {
  ssize_t res = write(fd, buff, buff_size);
  if (res < 0) {
    *error_p = errno;
    return false;
  } else {
    if (bytes_written) *bytes_written = res;
    return true;
  }
}

bool FileExists(const char *filename) {
  struct stat st;
  if (stat(filename, &st))
    return false;
  // Sanity check: filename is a regular file.
  return S_ISREG(st.st_mode);
}

bool DirExists(const char *path) {
  struct stat st;
  if (stat(path, &st))
    return false;
  return S_ISDIR(st.st_mode);
}

#else // SANITIZER_POSIX
#error ONLY SUPPORT POSIX NOW
#endif // SANITIZER_POSIX

} // namespace SANSYMTOOL_NS