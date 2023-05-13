#include "common.h"

#include <stdlib.h>
#include <cstdio>

//Only standard C calls in this file

namespace SANSYMTOOL_NS
{

void NORETURN Die() { exit(SANSYMTOOL_EXITCODE); }

void NORETURN CheckFailed(const char *file, int line, const char *cond,
                          u64 v1, u64 v2) {
    fprintf(stderr, SANSYMTOOL_MYNAME ": CHECK failed: %s:%d \"%s\" (0x%zx, 0x%zx)\n",
            file, line, cond, v1, v2);
    Die();
}

void SaySth(const char *file, int line, const char *sth) {
    fprintf(stderr, SANSYMTOOL_MYNAME ": (%s:%d) \"%s\"\n", file, line, sth);
}

#if SANITIZER_POSIX

/* POSIX-specific implementation for file I/O */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

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

#else // SANITIZER_POSIX
#error ONLY SUPPORT POSIX NOW
#endif // SANITIZER_POSIX

} // namespace SANSYMTOOL_NS