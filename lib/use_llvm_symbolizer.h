//===-- use_llvm_symbolizer.h ---------------------------------------------===//
//
// Based on the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declare some classes for llvm-symbolizer.
// They are designed for cross-platform.
//===----------------------------------------------------------------------===//

#ifndef SANSYMTOOL_HEAD_USE_LLVM_SYMBOLIZER_H
#define SANSYMTOOL_HEAD_USE_LLVM_SYMBOLIZER_H

#include "symbolizer.h"

namespace SANSYMTOOL_NS
{

// For now we assume the following protocol:
// For each request of the form
//   <module_name> <module_offset>
// passed to STDIN, external symbolizer prints to STDOUT response:
//   <function_name>
//   <file_name>:<line_number>:<column_number>
//   <function_name>
//   <file_name>:<line_number>:<column_number>
//   ...
//   <empty line>

class LLVMSymbolizerProcess final : public SymbolizerProcess {
 public:
  explicit LLVMSymbolizerProcess(const char *path);

 private:
  bool ReachedEndOfOutput(const char *buffer, uptr length) const override;

  void GetArgV(const char *path_to_binary,
               const char *(&argv)[kArgVMax]) const override;
};

class LLVMSymbolizer final : public SymbolizerTool {
 public:
  explicit LLVMSymbolizer(const char *path);

  bool SymbolizeData(DataInfo *info) override;
  bool SymbolizeAddr(AddrInfo *info) override;

  void StopTheWorld() override;

 private:
  const char *FormatAndSendCommand(const char *command_prefix,
                                   const char *module_name, uptr module_offset,
                                   ModuleArch arch);

  LLVMSymbolizerProcess *symbolizer_process_;
  static const uptr kBufferSize = 16 * 1024;
  char buffer_[kBufferSize];
};

} // namespace SANSYMTOOL_NS

#endif // SANSYMTOOL_HEAD_USE_LLVM_SYMBOLIZER_H