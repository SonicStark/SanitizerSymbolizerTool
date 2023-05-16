# Based on compiler-rt/lib/sanitizer_common/CMakeLists.txt in
# https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/llvm-project-12.0.0.src.tar.xz

include(AddCompilerRT)

set(SANITIZER_SYMBOLIZER_TOOL_SOURCES
  common.cpp
  interface.cpp
  symbolizer.cpp
  use_addr2line.cpp
  use_llvm_symbolizer.cpp
  )

# Explicitly list all sanitizer_common headers. Not all of these are
# included in sanitizer_common source files, but we need to depend on
# headers when building our custom unit tests.
set(SANITIZER_IMPL_HEADERS
  sanitizer_platform.h
  common.h
  symbolizer.h
  use_addr2line.h
  use_llvm_symbolizer.h
  )

set(SANITIZER_COMMON_DEFINITIONS "")

set(SANITIZER_CFLAGS ${SANITIZER_COMMON_CFLAGS})

if(APPLE)
  set(OS_OPTION OS ${SANITIZER_COMMON_SUPPORTED_OS})
endif()

add_compiler_rt_object_libraries(SanitizerSymbolizerTool
  ${OS_OPTION}
  ARCHS ${SANITIZER_COMMON_SUPPORTED_ARCH}
  SOURCES ${SANITIZER_SYMBOLIZER_TOOL_SOURCES}
  ADDITIONAL_HEADERS ${SANITIZER_IMPL_HEADERS}
  CFLAGS ${SANITIZER_CFLAGS}
  DEFS ${SANITIZER_COMMON_DEFINITIONS})