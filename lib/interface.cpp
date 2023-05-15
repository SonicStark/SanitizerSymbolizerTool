#include "sanitizer_symbolizer_tool.h"

#include "use_llvm_symbolizer.h"
#include "use_addr2line.h"

#include <cstring>
#include <cstdlib>

#if SANITIZER_POSIX

#include <unistd.h>

#else // SANITIZER_POSIX
#error ONLY SUPPORT POSIX NOW
#endif // SANITIZER_POSIX

// Only use SANITIZER_*ATTRIBUTE* before the function return type!
#if SANITIZER_WINDOWS
#if SANITIZER_IMPORT_INTERFACE
# define SANITIZER_INTERFACE_ATTRIBUTE __declspec(dllimport)
#else
# define SANITIZER_INTERFACE_ATTRIBUTE __declspec(dllexport)
#endif
# define SANITIZER_WEAK_ATTRIBUTE
#elif SANITIZER_GO
# define SANITIZER_INTERFACE_ATTRIBUTE
# define SANITIZER_WEAK_ATTRIBUTE
#else
# define SANITIZER_INTERFACE_ATTRIBUTE __attribute__((visibility("default")))
# define SANITIZER_WEAK_ATTRIBUTE  __attribute__((weak))
#endif


typedef enum
{
  run_nothing,
  run_llvm_symbolizer,
  run_addr2line
} ToolCode ;

typedef enum
{
  yes_init_done,
  yes_fini_done,
  err_path_not_executable,
  err_path_corrupted,
  err_unsupported_tool
} RetCode ;

static struct SANSYMTOOL_NS::DataInfo * pDataInfoBuf = nullptr;
static struct SANSYMTOOL_NS::AddrInfo * pAddrInfoBuf = nullptr;

static struct SANSYMTOOL_NS::SymbolizerTool * pSanSymTool = nullptr;
static ToolCode RunningThisTool = run_nothing;


int SanSymToolInit(const char * path) {

#if SANITIZER_POSIX
  if (access(path, X_OK)) { return (int) err_path_not_executable; }

  const char *binary_name = path ? SANSYMTOOL_NS::StripModuleName(path) : "";

  static const char kLLVMSymbolizerPrefix[] = "llvm-symbolizer";
  if (path && path[0] == '\0') {
    return (int) err_path_corrupted;

  } else if (!std::strncmp(binary_name, kLLVMSymbolizerPrefix, 
                            std::strlen(kLLVMSymbolizerPrefix))) {
    RunningThisTool = run_llvm_symbolizer;
    pSanSymTool = new SANSYMTOOL_NS::LLVMSymbolizer(path);

  } else if (!std::strcmp(binary_name, "addr2line")) {
    RunningThisTool = run_addr2line;
    pSanSymTool = new SANSYMTOOL_NS::Addr2LinePool(path);

  } else if (path) {
    return (int) err_unsupported_tool;
  }
#else // SANITIZER_POSIX
# if SANITIZER_WINDOWS
#  error Will support Windows in future! (Only "llvm-symbolizer.exe" is available there)
# else // SANITIZER_WINDOWS
#  error ONLY SUPPORT POSIX NOW
# endif // SANITIZER_WINDOWS
#endif // SANITIZER_POSIX

  pDataInfoBuf = new SANSYMTOOL_NS::DataInfo();
  pAddrInfoBuf = new SANSYMTOOL_NS::AddrInfo();
  return (int) yes_init_done;
}


void SanSymToolFini(void) {
  RunningThisTool = run_nothing;

  if (pSanSymTool) {
    pSanSymTool->StopTheWorld();
    delete pSanSymTool;
    pSanSymTool = nullptr;
  }

  if (pDataInfoBuf) {
    if (pDataInfoBuf->file) { std::free(pDataInfoBuf->file); }
    if (pDataInfoBuf->name) { std::free(pDataInfoBuf->name); }
    delete pDataInfoBuf;
    pDataInfoBuf = nullptr;
  }

  if (pAddrInfoBuf) {
    for (size_t i = 0; i < pAddrInfoBuf->frames.size(); ++i) {
      struct SANSYMTOOL_NS::FrameDat * pframe = &(pAddrInfoBuf->frames[i]);
      if (pframe->func) { std::free(pframe->func); }
      if (pframe->file) { std::free(pframe->file); }
    }
    pAddrInfoBuf->frames.clear();
    delete pAddrInfoBuf;
    pAddrInfoBuf = nullptr;
  }
}


void SanSymToolFreeDataRes(void) {
  if (pDataInfoBuf) {
    if (pDataInfoBuf->file) { 
      std::free(pDataInfoBuf->file);
      pDataInfoBuf->file = nullptr;
    }
    if (pDataInfoBuf->name) {
      std::free(pDataInfoBuf->name);
      pDataInfoBuf->name = nullptr;
    }
  }
}
void SanSymToolFreeAddrRes(void) {
  if (pAddrInfoBuf) {
    for (size_t i = 0; i < pAddrInfoBuf->frames.size(); ++i) {
      struct SANSYMTOOL_NS::FrameDat * pframe = &(pAddrInfoBuf->frames[i]);
      if (pframe->func) { std::free(pframe->func); }
      if (pframe->file) { std::free(pframe->file); }
    }
    pAddrInfoBuf->frames.clear();
  }
}
void SanSymToolFreeRes(int free_target) {
  switch (free_target)
  {
  case 1:
    SanSymToolFreeAddrRes();
    break;
  case 2:
    SanSymToolFreeDataRes();
    break;
  default:
    SanSymToolFreeAddrRes();
    SanSymToolFreeDataRes();
    break;
  }
}


