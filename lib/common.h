//===-- common.h ----------------------------------------------------------===//
//
// Based on the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Necessary basic defs. Most of them are migrated from
// lib/sanitizer_common/sanitizer_internal_defs.h in
// https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.3/compiler-rt-16.0.3.src.tar.xz
// Others can also be found in lib/sanitizer_common/*.h
//===----------------------------------------------------------------------===//

#ifndef SANSYMTOOL_HEAD_COMMON_H
#define SANSYMTOOL_HEAD_COMMON_H

#include "sanitizer_platform.h"

/**
 * Exit code when calling std::exit
 * in SANSYMTOOL_NS::Die
*/
#define SANSYMTOOL_EXITCODE 1
/**
 * Name prefix when calling SANSYMTOOL_NS::CheckFailed
 * and SANSYMTOOL_NS::SaySth for abort or log
*/
#define SANSYMTOOL_MYNAME "SanitizerSymbolizerTool"
/**
 * Whether to print log msgs when starting
 * a subprocess for debugging
*/
#define SANSYMTOOL_DBG_START_SUBPROCESS 0

/**
 * Whether to make llvm-symbolizer print demangled
 * function names if the names are mangled.
 * @note e.g. the mangled name _Z3bazv becomes baz(), 
 * whilst the non-mangled name foz is printed as is.
*/
#define SANSYMTOOL_LLVMSYMBOLIZER_DEMANGLE 0
/**
 * Whether to make llvm-symbolizer print all
 * the inlined frames if a source code location 
 * is in an inlined function.
*/
#define SANSYMTOOL_LLVMSYMBOLIZER_INLINES 1

/**
 * Whether to make addr2line print demangled
 * function names. Just like what
 * SANSYMTOOL_LLVMSYMBOLIZER_DEMANGLE
 * does.
*/
#define SANSYMTOOL_ADDR2LINE_DEMANGLE 0
/**
 * Whether to make addr2line print 
 * all the inlined frames. Just like what
 * SANSYMTOOL_LLVMSYMBOLIZER_INLINES
 * does.
*/
#define SANSYMTOOL_ADDR2LINE_INLINES 1
/**
 * This macro indicates max process num.
 * @note One addr2line can only analyse 
 * one binary. In case multiple binaries 
 * are here we manage a process pool.
 * Once the max num is reached, we kill
 * all processes in the pool and then
 * start new ones.
*/
#define SANSYMTOOL_ADDR2LINE_POOLMAX 16

namespace SANSYMTOOL_NS
{
// Platform-specific defs.
#if defined(_MSC_VER)
# define ALWAYS_INLINE __forceinline
// FIXME(timurrrr): do we need this on Windows?
# define ALIAS(x)
# define ALIGNED(x) __declspec(align(x))
# define FORMAT(f, a)
# define NOINLINE __declspec(noinline)
# define NORETURN __declspec(noreturn)
# define THREADLOCAL   __declspec(thread)
# define LIKELY(x) (x)
# define UNLIKELY(x) (x)
# define PREFETCH(x) /* _mm_prefetch(x, _MM_HINT_NTA) */ (void)0
# define WARN_UNUSED_RESULT
#else  // _MSC_VER
# define ALWAYS_INLINE inline __attribute__((always_inline))
# define ALIAS(x) __attribute__((alias(x)))
// Please only use the ALIGNED macro before the type.
// Using ALIGNED after the variable declaration is not portable!
# define ALIGNED(x) __attribute__((aligned(x)))
# define FORMAT(f, a)  __attribute__((format(printf, f, a)))
# define NOINLINE __attribute__((noinline))
# define NORETURN  __attribute__((noreturn))
# define THREADLOCAL   __thread
# define LIKELY(x)     __builtin_expect(!!(x), 1)
# define UNLIKELY(x)   __builtin_expect(!!(x), 0)
# if defined(__i386__) || defined(__x86_64__)
// __builtin_prefetch(x) generates prefetchnt0 on x86
#  define PREFETCH(x) __asm__("prefetchnta (%0)" : : "r" (x))
# else
#  define PREFETCH(x) __builtin_prefetch(x)
# endif
# define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#endif  // _MSC_VER

#if defined(_WIN64)
// 64-bit Windows uses LLP64 data model.
typedef unsigned long long uptr;
typedef signed long long sptr;
#else
#  if (SANITIZER_WORDSIZE == 64) || SANITIZER_APPLE || SANITIZER_WINDOWS
typedef unsigned long uptr;
typedef signed long sptr;
#  else
typedef unsigned int uptr;
typedef signed int sptr;
#  endif
#endif  // defined(_WIN64)
#if defined(__x86_64__)
// Since x32 uses ILP32 data model in 64-bit hardware mode, we must use
// 64-bit pointer to unwind stack frame.
typedef unsigned long long uhwptr;
#else
typedef uptr uhwptr;
#endif

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;

void NORETURN Die();
void NORETURN CheckFailed(const char *file, int line, const char *cond,
                        u64 v1, u64 v2);

#define CHECK_IMPL(c1, op, c2) \
  do { \
    SANSYMTOOL_NS::u64 v1 = (SANSYMTOOL_NS::u64)(c1); \
    SANSYMTOOL_NS::u64 v2 = (SANSYMTOOL_NS::u64)(c2); \
    if (UNLIKELY(!(v1 op v2))) \
      SANSYMTOOL_NS::CheckFailed(__FILE__, __LINE__, \
        "(" #c1 ") " #op " (" #c2 ")", v1, v2); \
  } while (false) \
  /**/

#define CHECK(a)       CHECK_IMPL((a), !=, 0)
#define CHECK_EQ(a, b) CHECK_IMPL((a), ==, (b))
#define CHECK_NE(a, b) CHECK_IMPL((a), !=, (b))
#define CHECK_LT(a, b) CHECK_IMPL((a), <,  (b))
#define CHECK_LE(a, b) CHECK_IMPL((a), <=, (b))
#define CHECK_GT(a, b) CHECK_IMPL((a), >,  (b))
#define CHECK_GE(a, b) CHECK_IMPL((a), >=, (b))

#define UNREACHABLE(msg) do { \
  CHECK(0 && msg); \
  Die(); \
} while (0)

#define UNIMPLEMENTED() UNREACHABLE("unimplemented")

void SaySth(const char *file, int line, const char *sth);
#define SAYSTH(s)                                 \
  do {                                            \
    SANSYMTOOL_NS::SaySth(__FILE__, __LINE__, s); \
  } while (0)

enum ModuleArch {
  kModuleArchUnknown,
  kModuleArchI386,
  kModuleArchX86_64,
  kModuleArchX86_64H,
  kModuleArchARMV6,
  kModuleArchARMV7,
  kModuleArchARMV7S,
  kModuleArchARMV7K,
  kModuleArchARM64,
  kModuleArchLoongArch64,
  kModuleArchRISCV64,
  kModuleArchHexagon
};

// When adding a new architecture, don't forget to also update use_llvm_symbolizer.cpp.
inline const char *ModuleArchToString(ModuleArch arch) {
  switch (arch) {
    case kModuleArchUnknown:
      return "";
    case kModuleArchI386:
      return "i386";
    case kModuleArchX86_64:
      return "x86_64";
    case kModuleArchX86_64H:
      return "x86_64h";
    case kModuleArchARMV6:
      return "armv6";
    case kModuleArchARMV7:
      return "armv7";
    case kModuleArchARMV7S:
      return "armv7s";
    case kModuleArchARMV7K:
      return "armv7k";
    case kModuleArchARM64:
      return "arm64";
    case kModuleArchLoongArch64:
      return "loongarch64";
    case kModuleArchRISCV64:
      return "riscv64";
    case kModuleArchHexagon:
      return "hexagon";
  }
  CHECK(0 && "Invalid module arch");
  return "";
}

// Defined for cross-platform process control
// functions. Can't be defined as pid_t since
// we don't want to have a conflict with unistd.h
#if SANITIZER_SOLARIS && !defined(_LP64)
typedef long proc_id_t;
#else
typedef int proc_id_t;
#endif

#if SANITIZER_WINDOWS
// On Windows, files are HANDLE, which is a synonim of void*.
// Use void* to avoid including <windows.h> everywhere.
typedef void* fd_t;
typedef unsigned error_t;
#else
typedef int fd_t;
typedef int error_t;
#endif

#define kInvalidFd ((fd_t)-1)
#define kStdinFd   ((fd_t) 0)
#define kStdoutFd  ((fd_t) 1)
#define kStderrFd  ((fd_t) 2)

enum FileAccessMode {
  RdOnly,
  WrOnly,
  RdWr
};

// Returns kInvalidFd on error.
fd_t OpenFile(const char *filename, FileAccessMode mode,
              error_t *errno_p = nullptr);
void CloseFile(fd_t);

// Return true on success, false on error.
bool ReadFromFile(fd_t fd, void *buff, uptr buff_size,
                  uptr *bytes_read = nullptr, error_t *error_p = nullptr);
bool WriteToFile(fd_t fd, const void *buff, uptr buff_size,
                 uptr *bytes_written = nullptr, error_t *error_p = nullptr);

bool FileExists(const char *filename);
bool DirExists(const char *path);

template <typename Fn>
class RunOnDestruction {
 public:
  explicit RunOnDestruction(Fn fn) : fn_(fn) {}
  ~RunOnDestruction() { fn_(); }

 private:
  Fn fn_;
};

// A simple scope guard. Usage:
// auto cleanup = at_scope_exit([]{ do_cleanup; });
template <typename Fn>
RunOnDestruction<Fn> at_scope_exit(Fn fn) {
  return RunOnDestruction<Fn>(fn);
}

// Char handling
inline bool IsSpace(int c) {
  return (c == ' ') || (c == '\n') || (c == '\t') ||
         (c == '\f') || (c == '\r') || (c == '\v');
}
inline bool IsDigit(int c) {
  return (c >= '0') && (c <= '9');
}
inline int ToLower(int c) {
  return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c;
}

const char *StripModuleName(const char *module);

} // namespace SANSYMTOOL_NS

#endif // SANSYMTOOL_HEAD_COMMON_H