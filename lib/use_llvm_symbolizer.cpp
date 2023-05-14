#include "use_llvm_symbolizer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace SANSYMTOOL_NS
{

// Parse a <file>:<line>[:<column>] buffer. The file path may contain colons on
// Windows, so extract tokens from the right hand side first. The column info is
// also optional.
static const char *ParseFileLineInfo(FrameDat *info, const char *str) {
  char *file_line_info = nullptr;
  str = ExtractToken(str, "\n", &file_line_info);
  CHECK(file_line_info);

  if (uptr size = std::strlen(file_line_info)) {
    char *back = file_line_info + size - 1;
    for (int i = 0; i < 2; ++i) {
      while (back > file_line_info && IsDigit(*back)) --back;
      if (*back != ':' || !IsDigit(back[1])) break;
      info->col = info->lin;
      info->lin = std::atoll(back + 1);
      // Truncate the string at the colon to keep only filename.
      *back = '\0';
      --back;
    }
    ExtractToken(file_line_info, "", &info->file);
  }

  std::free(file_line_info);
  return str;
}

// Parses one or more two-line strings in the following format:
//   <function_name>
//   <file_name>:<line_number>[:<column_number>]
// Used by LLVMSymbolizer, Addr2LinePool, since all of them 
// use the same output format.
void ParseSymbolizeAddrOutput(const char *str, AddrInfo *res) {
  while (true) {
    char *function_name = nullptr;
    str = ExtractToken(str, "\n", &function_name);
    CHECK(function_name);
    if (function_name[0] == '\0') {
      // There are no more frames.
      free(function_name);
      break;
    }
    struct FrameDat ThisFrame;
    ThisFrame.func = function_name;
    str = ParseFileLineInfo(&ThisFrame, str);

    // Functions and filenames can be "??", in which case we write 0
    // to address info to mark that names are unknown.
    if (0 == std::strcmp(ThisFrame.func, "??")) {
      std::free(ThisFrame.func);
      ThisFrame.func = 0;
    }
    if (ThisFrame.file && 0 == std::strcmp(ThisFrame.file, "??")) {
      std::free(ThisFrame.file);
      ThisFrame.file = 0;
    }
    res->frames.push_back(ThisFrame);
  }
}

// Parses a two- or three-line string in the following format:
//   <symbol_name>
//   <start_address> <size>
//   <filename>:<column>
// Used by LLVMSymbolizer and InternalSymbolizer. LLVMSymbolizer added support
// for symbolizing the third line in D123538, but we support the older two-line
// information as well.
void ParseSymbolizeDataOutput(const char *str, DataInfo *info) {
  str = ExtractToken(str, "\n", &info->name);
  str = ExtractUptr(str, " ", &info->start);
  str = ExtractUptr(str, "\n", &info->size);
  // Note: If the third line isn't present, these calls will set info.{file,
  // line} to empty strings.
  str = ExtractToken(str, ":", &info->file);
  str = ExtractUptr(str, "\n", &info->line);
}

LLVMSymbolizerProcess::LLVMSymbolizerProcess(const char *path)
    : SymbolizerProcess(path, /*use_posix_spawn=*/SANITIZER_APPLE) {}

bool LLVMSymbolizerProcess::ReachedEndOfOutput(const char *buffer, uptr length) const {
  // Empty line marks the end of llvm-symbolizer output.
  return length >= 2 && buffer[length - 1] == '\n' &&
           buffer[length - 2] == '\n';
}

void LLVMSymbolizerProcess::GetArgV(const char *path_to_binary,
               const char *(&argv)[kArgVMax]) const {
// When adding a new architecture, don't forget to also update common.h.
#if defined(__x86_64h__)
  const char* const kSymbolizerArch = "--default-arch=x86_64h";
#elif defined(__x86_64__)
  const char* const kSymbolizerArch = "--default-arch=x86_64";
#elif defined(__i386__)
  const char* const kSymbolizerArch = "--default-arch=i386";
#elif SANITIZER_LOONGARCH64
  const char *const kSymbolizerArch = "--default-arch=loongarch64";
#elif SANITIZER_RISCV64
  const char *const kSymbolizerArch = "--default-arch=riscv64";
#elif defined(__aarch64__)
  const char* const kSymbolizerArch = "--default-arch=arm64";
#elif defined(__arm__)
  const char* const kSymbolizerArch = "--default-arch=arm";
#elif defined(__powerpc64__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  const char* const kSymbolizerArch = "--default-arch=powerpc64";
#elif defined(__powerpc64__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  const char* const kSymbolizerArch = "--default-arch=powerpc64le";
#elif defined(__s390x__)
  const char* const kSymbolizerArch = "--default-arch=s390x";
#elif defined(__s390__)
  const char* const kSymbolizerArch = "--default-arch=s390";
#else
  const char* const kSymbolizerArch = "--default-arch=unknown";
#endif

#if SANSYMTOOL_LLVMSYMBOLIZER_DEMANGLE
  const char *const demangle_flag = "--demangle";
#else
  const char *const demangle_flag = "--no-demangle";
#endif

#if SANSYMTOOL_LLVMSYMBOLIZER_INLINES
  const char *const inline_flag = "--inlines";
#else
  const char *const inline_flag = "--no-inlines";
#endif

  int i = 0;
  argv[i++] = path_to_binary;
  argv[i++] = demangle_flag;
  argv[i++] = inline_flag;
  argv[i++] = kSymbolizerArch;
  argv[i++] = nullptr;
  CHECK_LE(i, kArgVMax);
}

LLVMSymbolizer::LLVMSymbolizer(const char *path)
    : symbolizer_process_(new LLVMSymbolizerProcess(path)) {}

bool LLVMSymbolizer::SymbolizeAddr(AddrInfo *info) {
  const char *buf = FormatAndSendCommand(
      "CODE", info->module, info->module_offset, info->module_arch);
  if (!buf)
    return false;
  ParseSymbolizeAddrOutput(buf, info);
  return true;
}

bool LLVMSymbolizer::SymbolizeData(DataInfo *info) {
  const char *buf = FormatAndSendCommand(
      "DATA", info->module, info->module_offset, info->module_arch);
  if (!buf)
    return false;
  ParseSymbolizeDataOutput(buf, info);
  return true;
}

const char *LLVMSymbolizer::FormatAndSendCommand(const char *command_prefix,
                                                 const char *module_name,
                                                 uptr module_offset,
                                                 ModuleArch arch) {
  CHECK(module_name);
  int size_needed = 0;
  if (arch == kModuleArchUnknown)
    size_needed = std::snprintf(buffer_, kBufferSize, "%s \"%s\" 0x%zx\n",
                                    command_prefix, module_name, module_offset);
  else
    size_needed = std::snprintf(buffer_, kBufferSize,
                                    "%s \"%s:%s\" 0x%zx\n", command_prefix,
                                    module_name, ModuleArchToString(arch),
                                    module_offset);

  if (size_needed >= static_cast<int>(kBufferSize)) {
    SAYSTH("WARNING: Command buffer too small");
    return nullptr;
  }

  return symbolizer_process_->SendCommand(buffer_);
}

}