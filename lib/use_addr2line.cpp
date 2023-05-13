#include "use_addr2line.h"

#if SANITIZER_POSIX

#include <string.h>
#include <cstdio>

namespace SANSYMTOOL_NS
{

Addr2LineProcess::Addr2LineProcess(const char *path, const char *module_name) 
  : SymbolizerProcess(path), module_name_(strdup(module_name)) {}

const char *Addr2LineProcess::module_name() const { return module_name_; }

void Addr2LineProcess::GetArgV(const char *path_to_binary,
               const char *(&argv)[kArgVMax]) const {
  int i = 0;
  argv[i++] = path_to_binary;

#if SANSYMTOOL_ADDR2LINE_DEMANGLE
  argv[i++] = "-C";
#endif
#if SANSYMTOOL_ADDR2LINE_INLINES
  argv[i++] = "-i";
#endif
  argv[i++] = "-fe";
  argv[i++] = module_name_;
  argv[i++] = nullptr;
  CHECK_LE(i, kArgVMax);
}

const char Addr2LineProcess::output_terminator_[] = "??\n??:0\n";

bool Addr2LineProcess::ReachedEndOfOutput(const char *buffer,
                                          uptr length) const {
  const size_t kTerminatorLen = sizeof(output_terminator_) - 1;
  // Skip, if we read just kTerminatorLen bytes, because Addr2Line output
  // should consist at least of two pairs of lines:
  // 1. First one, corresponding to given offset to be symbolized
  // (may be equal to output_terminator_, if offset is not valid).
  // 2. Second one for output_terminator_, itself to mark the end of output.
  if (length <= kTerminatorLen) return false;
  // Addr2Line output should end up with output_terminator_.
  return !memcmp(buffer + length - kTerminatorLen,
                          output_terminator_, kTerminatorLen);
}

bool Addr2LineProcess::ReadFromSymbolizer() {
  if (!SymbolizerProcess::ReadFromSymbolizer())
    return false;
  auto &buff = GetBuff();
  // We should cut out output_terminator_ at the end of given buffer,
  // appended by addr2line to mark the end of its meaningful output.
  // We cannot scan buffer from it's beginning, because it is legal for it
  // to start with output_terminator_ in case given offset is invalid. So,
  // scanning from second character.
  char *garbage = strstr(buff.data() + 1, output_terminator_);
  // This should never be NULL since buffer must end up with
  // output_terminator_.
  CHECK(garbage);

  // Trim the buffer.
  uintptr_t new_size = garbage - buff.data();
  GetBuff().resize(new_size);
  GetBuff().push_back('\0');
  return true;
}

Addr2LinePool::Addr2LinePool(const char *addr2line_path)
    : addr2line_path_(addr2line_path) {
    addr2line_pool_.reserve(16);
  }

bool Addr2LinePool::SymbolizeData(DataInfo *info) { return false; }

bool Addr2LinePool::SymbolizeAddr(AddrInfo *info) {
  if (const char *buf =
        SendCommand(info->module, info->module_offset)) {
    ParseSymbolizeAddrOutput(buf, info);
    return true;
  }
  return false;
}

const char *Addr2LinePool::SendCommand(const char *module_name, uptr module_offset) {
  Addr2LineProcess *addr2line = 0;
  for (uptr i = 0; i < addr2line_pool_.size(); ++i) {
    if (0 ==
        strcmp(module_name, addr2line_pool_[i]->module_name())) {
      addr2line = addr2line_pool_[i];
      break;
    }
  }
  if (!addr2line) {
    addr2line =
        new Addr2LineProcess(addr2line_path_, module_name);
    addr2line_pool_.push_back(addr2line);
  }
  CHECK_EQ(0, strcmp(module_name, addr2line->module_name()));
  char buffer[kBufferSize];
  std::snprintf(buffer, kBufferSize, "0x%zx\n0x%zx\n",
                    module_offset, dummy_address_);
  return addr2line->SendCommand(buffer);
}

} // namespace SANSYMTOOL_NS

#else // SANITIZER_POSIX
#warning addr2line is only available on POSIX platform
#endif  // SANITIZER_POSIX