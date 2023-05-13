#include "symbolizer.h"

#include <string.h>

namespace SANSYMTOOL_NS
{
#if SANITIZER_POSIX
/* POSIX-specific implementation of symbolizer parts */

SymbolizerProcess::SymbolizerProcess(const char *path, bool use_posix_spawn)
    : path_(path),
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
  if (!WriteToSymbolizer(command, strlen(command)))
      return nullptr;
  if (!ReadFromSymbolizer())
    return nullptr;
  return buffer_.data();
}

bool SymbolizerProcess::Restart() {
  if (input_fd_ != kInvalidFd)
    CloseFile(input_fd_);
  if (output_fd_ != kInvalidFd)
    CloseFile(output_fd_);
  return StartSymbolizerSubprocess();
}

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
      SAYSTH("WARNING: Can't read from symbolizer at input_fd_\n");
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
    SAYSTH("WARNING: Can't write to symbolizer at output_fd_\n");
    return false;
  }
  return true;
}

#endif  // SANITIZER_POSIX

} // namespace SANSYMTOOL_NS