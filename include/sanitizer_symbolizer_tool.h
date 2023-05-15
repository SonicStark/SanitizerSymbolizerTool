//===-- sanitizer_symbolizer_tool.h ---------------------------------------===//
//
// Based on the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of https://github.com/SonicStark/SanitizerSymbolizerTool.
//
// Public interface header.
//===----------------------------------------------------------------------===//

#ifndef SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H
#define SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init all stuffs.
 * Will init all global buf and object pointers.
 * If not succeed, they will be all nullptr.
 * 
 * @param external_symbolizer_path
 * Path to the symbolizer. Must be valid and executable.
 * It's strongly recommended that using absolute path.
 * Currently only llvm-symbolizer and addr2line
 * (both on POSIX) are supported.
 * @return Defined by enum RetCode in lib/interface.cpp
*/
int SanSymTool_init(const char * external_symbolizer_path);

/**
 * Destroy all stuffs to clean up.
 * Will stop symbolizer subprocess, call
 * SanSymTool_*_free, free all allocated
 * global pointers and set them to nullptr.
 * If some of them are already nullptr,
 * it will just jmp over.
*/
void SanSymTool_fini(void);

/**
 * Send a request to symbolize the address
 * as executable code.
 * 
 * @param module The name/path of target binary.
 * @param offset Offset relative to start of the binary.
 * Please remember that we do symbolize the executable code
 * at which is an offset in target binary, not VMA!
 * @param n_frames If a source code location is in an 
 * inlined function, all the inlined frames will be symbolized.
 * Use n_frames to receive total number of the frames. If the
 * return value indicates it's failed, value of this pointer
 * will not be touched.
 * @return Defined by enum RetCode in lib/interface.cpp
*/
int SanSymTool_addr_send(char *module, unsigned int offset, unsigned long *n_frames);

/**
 * Read symbolizing result of executable code 
 * after the last request sent out.
 * 
 * @param idx Index of the frame which you want to read.
 * Can't be greater than (n_frames-1).
 * @param file Receive the pointer of an internal allocated
 * memory which stores file path.
 * @param function Receive the pointer of an internal allocated
 * memory which stores function name.
 * @param line Receive the line number.
 * @param column Receive the column number.
*/
int SanSymTool_addr_read(unsigned long idx, char **file, char **function, unsigned long *line, unsigned long *column);

/**
 * Free the internal allocated memory because of
 * symbolizing executable code.
 * 
 * @attention After calling this, the pointers returned
 * by SanSymTool_addr_read should not be touched anymore.
 * Otherwise it's *Use-After-Free*. 
 * 
 * @attention If you call SanSymTool_addr_read after 
 * calling this, dirty data will be got. Please don't do this.
 * 
 * @warning This can only free the *latest* allocated memory.
 * The allocation happens when you call SanSymTool_addr_send, and
 * the allocated ones then can be accessed by SanSymTool_addr_read.
 * That is to say, if you don't use SanSymTool_addr_free 
 * between two calls of SanSymTool_addr_read, the pointers 
 * received from SanSymTool_addr_read between the two call
 * will be the ONLY way to touch the memory. If the pointers
 * are not held, there will be *Memory-Leaks*.
 * Besides, calling free on these pointers manually is a dirty idea.
 * So you must:
 * (1) Make a safe copy of strings from the allocated memory in your way.
 * (2) Call this before the next call of SanSymTool_addr_send.
*/
void SanSymTool_addr_free(void);

/**
 * Send a request to symbolize the address
 * as as data.
 * 
 * @attention Currently only llvm-symbolizer is supported.
 * Calling this for addr2line will always return as fail.
 * 
 * @note What's more, LLVMSymbolizer added support for 
 * symbolizing the third line in https://reviews.llvm.org/D123538 , 
 * but we support the older two-line information as well.
 * If the third line isn't present, *file* will be "\0"
 * and *line* will be 0.
 * 
 * @param module The name/path of target binary.
 * @param offset Offset relative to start of the binary.
 * Please remember that we do symbolize the data
 * at which is an offset in target binary, not VMA!
 * @return Defined by enum RetCode in lib/interface.cpp
*/
int SanSymTool_data_send(char *module, unsigned int offset);

/**
 * Read symbolizing result of data
 * after the last request sent out.
 * 
 * @param file Receive the pointer of an internal allocated
 * memory which stores file name.
 * @param name Receive the pointer of an internal allocated
 * memory which stores data name.
 * @param line Receive the line number.
 * @param start Receive start address of the data. It seems that
 * offset just equals start. Just keep it here for compatibility :-)
 * @param size Receive the data size.
*/
int SanSymTool_data_read(char **file, char **name, unsigned long *line, unsigned long *start, unsigned long *size);

/**
 * Free the internal allocated memory
 * because of symbolizing data.
 * 
 * @attention After calling this, the pointers returned
 * by SanSymTool_data_read should not be touched anymore.
 * Otherwise it's *Use-After-Free*. 
 * 
 * @attention If you call SanSymTool_data_read after 
 * calling this, dirty data will be got. Please don't do this.
 * 
 * @warning This can only free the *latest* allocated memory.
 * The allocation happens when you call SanSymTool_data_send, and
 * the allocated ones then can be accessed by SanSymTool_data_read.
 * That is to say, if you don't use SanSymTool_data_free 
 * between two calls of SanSymTool_data_read, the pointers 
 * received from SanSymTool_data_read between the two call
 * will be the ONLY way to touch the memory. If the pointers
 * are not held, there will be *Memory-Leaks*.
 * Besides, calling free on these pointers manually is a dirty idea.
 * So you must:
 * (1) Make a safe copy of strings from the allocated memory in your way.
 * (2) Call this before the next call of SanSymTool_data_send.
*/
void SanSymTool_data_free(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif