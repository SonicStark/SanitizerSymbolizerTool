#ifndef SANSYMTOOL_HEAD_COMMON_H
#define SANSYMTOOL_HEAD_COMMON_H

#include "sanitizer_platform.h"

#define SANSYMTOOL_EXITCODE 1
#define SANSYMTOOL_MYNAME "SanitizerSymbolizerTool"

# define LIKELY(x)    __builtin_expect(!!(x), 1)
# define UNLIKELY(x)  __builtin_expect(!!(x), 0)

namespace SANSYMTOOL_NS
{

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

#if SANITIZER_WINDOWS
// On Windows, files are HANDLE, which is a synonim of void*.
// Use void* to avoid including <windows.h> everywhere.
typedef void* fd_t;
typedef unsigned error_t;
#else
typedef int fd_t;
typedef int error_t;
#endif
#if SANITIZER_SOLARIS && !defined(_LP64)
typedef long pid_t;
#else
typedef int pid_t;
#endif

void Die();
void CheckFailed(const char *file, int line, const char *cond,
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

}

#endif