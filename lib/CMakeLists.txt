# Based on compiler-rt/lib/asan/CMakeLists.txt in
# https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/llvm-project-12.0.0.src.tar.xz

set(SANSYMTOOL_SOURCES
  common.cpp
  interface.cpp
  symbolizer.cpp
  use_addr2line.cpp
  use_llvm_symbolizer.cpp
  )

SET(SANSYMTOOL_HEADERS
  sanitizer_platform.h
  sanitizer_symbolizer_tool.h
  common.h
  symbolizer.h
  use_addr2line.h
  use_llvm_symbolizer.h
  )

include(AddCompilerRT)

set(ASAN_CFLAGS ${SANITIZER_COMMON_CFLAGS})
set(ASAN_COMMON_DEFINITIONS ${COMPILER_RT_ASAN_SHADOW_SCALE_DEFINITION})

append_rtti_flag(OFF ASAN_CFLAGS)

set(ASAN_DYNAMIC_LINK_FLAGS ${SANITIZER_COMMON_LINK_FLAGS})

if(ANDROID)
# Put most Sanitizer shared libraries in the global group. For more details, see
# android-changes-for-ndk-developers.md#changes-to-library-search-order
  if (COMPILER_RT_HAS_Z_GLOBAL)
    list(APPEND ASAN_DYNAMIC_LINK_FLAGS -Wl,-z,global)
  endif()
endif()


set(ASAN_DYNAMIC_CFLAGS ${ASAN_CFLAGS})

append_list_if(MSVC /DEBUG ASAN_DYNAMIC_LINK_FLAGS)

set(ASAN_DYNAMIC_LIBS ${SANITIZER_CXX_ABI_LIBRARIES} ${SANITIZER_COMMON_LINK_LIBS})

append_list_if(COMPILER_RT_HAS_LIBDL dl ASAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBRT rt ASAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBM m ASAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBPTHREAD pthread ASAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBLOG log ASAN_DYNAMIC_LIBS)
append_list_if(MINGW "${MINGW_LIBRARIES}" ASAN_DYNAMIC_LIBS)

# Compile ASan sources into an object library.

add_compiler_rt_object_libraries(RTSanSymTool
  OS ${SANITIZER_COMMON_SUPPORTED_OS}
  ARCHS ${ASAN_SUPPORTED_ARCH}
  SOURCES ${SANSYMTOOL_SOURCES}
  ADDITIONAL_HEADERS ${SANSYMTOOL_HEADERS}
  CFLAGS ${ASAN_DYNAMIC_CFLAGS}
  DEFS ${ASAN_DYNAMIC_DEFINITIONS})

add_compiler_rt_component(SanSymTool)

if(APPLE)
  add_compiler_rt_runtime(sansymtool
    SHARED
    OS ${SANITIZER_COMMON_SUPPORTED_OS}
    ARCHS ${ASAN_SUPPORTED_ARCH}
    OBJECT_LIBS RTSanSymTool
    CFLAGS ${ASAN_DYNAMIC_CFLAGS}
    LINK_FLAGS ${WEAK_SYMBOL_LINK_FLAGS}
    DEFS ${ASAN_DYNAMIC_DEFINITIONS}
    PARENT_TARGET SanSymTool)
else()
  # Build separate libraries for each target.

  add_compiler_rt_runtime(sansymtool
    STATIC
    ARCHS ${ASAN_SUPPORTED_ARCH}
    OBJECT_LIBS RTSanSymTool
    CFLAGS ${ASAN_CFLAGS}
    DEFS ${ASAN_COMMON_DEFINITIONS}
    PARENT_TARGET SanSymTool)

  foreach(arch ${ASAN_SUPPORTED_ARCH})

    add_compiler_rt_runtime(sansymtool
      SHARED
      ARCHS ${arch}
      OBJECT_LIBS RTSanSymTool
      CFLAGS ${ASAN_DYNAMIC_CFLAGS}
      LINK_FLAGS ${ASAN_DYNAMIC_LINK_FLAGS}
      LINK_LIBS ${ASAN_DYNAMIC_LIBS}
      DEFS ${ASAN_DYNAMIC_DEFINITIONS}
      PARENT_TARGET SanSymTool)

    if (WIN32)
      message( FATAL_ERROR "Currently WIN32 is not supported!" )
##      add_compiler_rt_object_libraries(AsanDllThunk
##        ${SANITIZER_COMMON_SUPPORTED_OS}
##        ARCHS ${arch}
##        SOURCES asan_globals_win.cpp
##                asan_win_dll_thunk.cpp
##        CFLAGS ${ASAN_CFLAGS} -DSANITIZER_DLL_THUNK
##        DEFS ${ASAN_COMMON_DEFINITIONS})
##
##      add_compiler_rt_runtime(clang_rt.asan_dll_thunk
##        STATIC
##        ARCHS ${arch}
##        OBJECT_LIBS AsanDllThunk
##                    UbsanDllThunk
##                    SancovDllThunk
##                    SanitizerCommonDllThunk
##        SOURCES $<TARGET_OBJECTS:RTInterception.${arch}>
##        PARENT_TARGET asan)
    endif()
  endforeach()
endif()
