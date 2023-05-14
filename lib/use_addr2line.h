#ifndef SANSYMTOOL_HEAD_USE_ADDR2LINE_H
#define SANSYMTOOL_HEAD_USE_ADDR2LINE_H

#include "symbolizer.h"

#if SANITIZER_POSIX

#include <vector>
#include <cstdint>

namespace SANSYMTOOL_NS
{

class Addr2LineProcess final : public SymbolizerProcess {
 public:
  Addr2LineProcess(const char *path, const char *module_name);

  char *module_name() const;
  // Free the strdup result with pointer set to 0 in case needed
  void module_name_free();

 private:
  void GetArgV(const char *path_to_binary,
               const char *(&argv)[kArgVMax]) const override;

  bool ReachedEndOfOutput(const char *buffer, uptr length) const override;

  bool ReadFromSymbolizer() override;

  char *module_name_;  // Owned, leaked. Unless free with module_name_free
  static const char output_terminator_[];
};

class Addr2LinePool final : public SymbolizerTool {
 public:
  explicit Addr2LinePool(const char *addr2line_path);

  bool SymbolizeData(DataInfo *info) override;
  bool SymbolizeAddr(AddrInfo *info) override;

  void StopTheWorld() override;

 private:
  const char *SendCommand(const char *module_name, uptr module_offset);

  static const uptr kBufferSize = 64;
  const char *addr2line_path_;

  // If there are many different module names,
  // we'll get many subprocesses running addr2line.
  // There should be some garbage collection.
  void FlushPool();
  std::vector<Addr2LineProcess*> addr2line_pool_;

  static const uptr dummy_address_ =
      FIRST_32_SECOND_64(UINT32_MAX, UINT64_MAX);
};

} // namespace SANSYMTOOL_NS

#else // SANITIZER_POSIX
#warning addr2line is only available on POSIX platform
#endif  // SANITIZER_POSIX

#endif // SANSYMTOOL_HEAD_USE_ADDR2LINE_H