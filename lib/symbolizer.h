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
  int lin;
  int col;
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

protected:
  ~SymbolizerTool() {}
};

// SymbolizerProcess encapsulates communication between the tool and
// external symbolizer program, running in a different subprocess.
// SymbolizerProcess may not be used from two threads simultaneously.
class SymbolizerProcess {
public:
  explicit SymbolizerProcess(const char *path, bool use_posix_spawn = false);
  const char *SendCommand(const char *command);

protected:
  ~SymbolizerProcess() {}

  /// The maximum number of arguments required to invoke a tool process.
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

  bool Restart();
  const char *SendCommandImpl(const char *command);
  bool WriteToSymbolizer(const char *buffer, uptr length);

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

// Parses one or more two-line strings in the following format:
//   <function_name>
//   <file_name>:<line_number>[:<column_number>]
// Used by LLVMSymbolizer, Addr2LinePool, since all of them 
// use the same output format.
void ParseSymbolizeOutputNorm(const char *str, AddrInfo *res);

}

#endif