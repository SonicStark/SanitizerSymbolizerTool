# Based on compiler-rt/include/CMakeLists.txt in
# https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/llvm-project-12.0.0.src.tar.xz

set(COMPILER_RT_HEADERS sanitizer_symbolizer_tool.h)

set(output_dir ${COMPILER_RT_OUTPUT_DIR}/include)

# Copy compiler-rt headers to the build tree.
set(out_files)
foreach( f ${COMPILER_RT_HEADERS} )
  set( src ${CMAKE_CURRENT_SOURCE_DIR}/${f} )
  set( dst ${output_dir}/${f} )
  add_custom_command(OUTPUT ${dst}
    DEPENDS ${src}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src} ${dst}
    COMMENT "Copying compiler-rt's ${f}...")
  list(APPEND out_files ${dst})
endforeach( f )

add_custom_target(compiler-rt-headers ALL DEPENDS ${out_files})
add_dependencies(compiler-rt compiler-rt-headers)
set_target_properties(compiler-rt-headers PROPERTIES FOLDER "Compiler-RT Misc")

# Install the headers.
install(FILES ${COMPILER_RT_HEADERS}
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${COMPILER_RT_INSTALL_PATH}/include)

if (NOT CMAKE_CONFIGURATION_TYPES) # don't add this for IDEs.
  add_custom_target(install-compiler-rt-headers
                    DEPENDS compiler-rt-headers
                    COMMAND "${CMAKE_COMMAND}"
                            -DCMAKE_INSTALL_COMPONENT="compiler-rt-headers"
                            -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
                    USES_TERMINAL)
  add_custom_target(install-compiler-rt-headers-stripped
                    DEPENDS compiler-rt-headers
                    COMMAND "${CMAKE_COMMAND}"
                            -DCMAKE_INSTALL_COMPONENT="compiler-rt-headers"
                            -DCMAKE_INSTALL_DO_STRIP=1
                            -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
                    USES_TERMINAL)
endif()
