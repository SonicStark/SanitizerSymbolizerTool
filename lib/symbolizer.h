//===-- symbolizer.h ------------------------------------------------------===//
//
// Based on the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file delcares some common basic classes, structs and functions
// for launching and using a symbolizer, which shared between all other files.
//===----------------------------------------------------------------------===//

#ifndef SANSYMTOOL_HEAD_SYMBOLIZER_H
#define SANSYMTOOL_HEAD_SYMBOLIZER_H

#include "common.h"
#include <vector>

namespace SANSYMTOOL_NS
{

// Advanced symbolizer can symbolize an address
// as data or executable code respectively.
// For now, DataInfo is used to describe global variable.
struct DataInfo {
  char      *module;
  uptr       module_offset;
  ModuleArch module_arch;

  char *file;
  uptr  line;
  char *name;
  uptr  start;
  uptr  size;
};

// Advanced symbolizer can deal with inlined functions.
// So use a vector here. For example llvm-symbolizer,
// if a source code location is in an inlined function,
// it can print all the inlined frames.
struct FrameDat {
  char *func;
  char *file;
  uptr lin;
  uptr col;
};
struct AddrInfo {
  char      *module;
  uptr       module_offset;
  ModuleArch module_arch;

  std::vector<FrameDat> frames;
};

// Base class for a symbolizer tool
class SymbolizerTool {
public:
  // We may implement a "fallback chain" of symbolizer tools.
  //In a request to symbolize an address, if one tool returns false,
  // the next tool in the chain will be tried.
  SymbolizerTool *next;

  SymbolizerTool() : next(nullptr) {}

  // The |info| parameter is inout. 
  // It is pre-filled with the module base, module offset and arch (if any),
  // then we use the following methods to fill the remained fields.
  virtual bool SymbolizeData(DataInfo *info) { UNIMPLEMENTED(); }
  virtual bool SymbolizeAddr(AddrInfo *info) { UNIMPLEMENTED(); }
  
  // Destroy all SymbolizerProcess related stuffs.
  // IT IS IRREVERSIBLE !!!
  // Used for object destruction. Memory allocated inside
  // SymbolizerProcess then can only be touched by pointers
  // inside XXXXInfo struct passed to SymbolizeXXXX.
  // Managing memory is the user's responsibility.
  virtual void StopTheWorld() { UNIMPLEMENTED(); }
};

// SymbolizerProcess encapsulates communication between the tool and
// external symbolizer program, running in a different subprocess.
// SymbolizerProcess may not be used from two threads simultaneously.
class SymbolizerProcess {
public:
  explicit SymbolizerProcess(const char *path, bool use_posix_spawn = false);
  const char *SendCommand(const char *command);

  /* Methods for controlling the subprocess */
  bool      Restart();
  proc_id_t GetPID();
  bool      IsRunning();
  bool      Kill();

protected:
  // The maximum number of arguments required to invoke a tool process.
  static const unsigned kArgVMax = 16;

  // Customizable by subclasses.
  virtual bool StartSymbolizerSubprocess();
  virtual bool ReadFromSymbolizer();

  std::vector<char> &GetBuff() { return buffer_; }

private:
  virtual bool ReachedEndOfOutput(const char *buffer, uptr length) const {
    UNIMPLEMENTED();
  }

  /// Fill in an argv array to invoke the child process.
  virtual void GetArgV(const char *path_to_binary,
                       const char *(&argv)[kArgVMax]) const {
    UNIMPLEMENTED();
  }

  const char *SendCommandImpl(const char *command);
  bool WriteToSymbolizer(const char *buffer, uptr length);

  //PID of current active symbolizer process, -1 for non
  proc_id_t active_pid_;

  const char *path_;
  fd_t input_fd_;
  fd_t output_fd_;

  std::vector<char> buffer_;

  static const uptr kMaxTimesRestarted = 5;
  static const int kSymbolizerStartupTimeMillis = 10;
  uptr times_restarted_;
  bool failed_to_start_;
  bool reported_invalid_path_;
  bool use_posix_spawn_;
};

// Used by LLVMSymbolizer, Addr2LinePool.
// Although declared here for common usage,
// it is defined in use_llvm_symbolizer.cpp
void ParseSymbolizeAddrOutput(const char *str, AddrInfo *res);

// Parsing helpers, 'str' is searched for delimiter(s) and a string or uptr
// is extracted. When extracting a string, a newly allocated (using std::malloc)
// and null-terminated buffer is returned. They return a pointer
// to the next characted after the found delimiter.
const char *ExtractToken             (const char *str, const char *delims,    char **result);
const char *ExtractInt               (const char *str, const char *delims,    int   *result);
const char *ExtractUptr              (const char *str, const char *delims,    uptr  *result);
const char *ExtractTokenUpToDelimiter(const char *str, const char *delimiter, char **result);

} // namespace SANSYMTOOL_NS

#endif // SANSYMTOOL_HEAD_SYMBOLIZER_H 