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
#elif SANITIZER_GO
# define SANITIZER_INTERFACE_ATTRIBUTE
#else
# define SANITIZER_INTERFACE_ATTRIBUTE __attribute__((visibility("default")))
#endif


typedef enum
{
  run_nothing,
  run_llvm_symbolizer,
  run_addr2line
} ToolCode;

typedef enum
{
  yes_init_done,
  yes_fini_done,
  yes_send_done,
  yes_read_done,
  err_path_not_executable,
  err_path_corrupted,
  err_unsupported_tool,
  err_symbolize_failed,
  err_has_nullptr,
  err_outofbound
} RetCode;

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

void SanSymToolFini(void) {
  RunningThisTool = run_nothing;

  if (pSanSymTool) {
    pSanSymTool->StopTheWorld();
    delete pSanSymTool;
    pSanSymTool = nullptr;
  }

  SanSymToolFreeDataRes();
  if (pDataInfoBuf) {
    delete pDataInfoBuf;
    pDataInfoBuf = nullptr;
  }

  SanSymToolFreeAddrRes();
  if (pAddrInfoBuf) {
    delete pAddrInfoBuf;
    pAddrInfoBuf = nullptr;
  }
}

int SanSymToolSendAddrDat(char *module, unsigned int offset, unsigned long *n_frames) {
  if (!(pSanSymTool && pAddrInfoBuf)) { return (int) err_has_nullptr; }

  pAddrInfoBuf->module        = module;
  pAddrInfoBuf->module_offset = offset;
  pAddrInfoBuf->module_arch   = SANSYMTOOL_NS::kModuleArchUnknown;
  
  if (pSanSymTool->SymbolizeAddr(pAddrInfoBuf)) {
    *n_frames = (pAddrInfoBuf->frames).size();
    return (int) yes_send_done;
  } else {
    return (int) err_symbolize_failed;
  }
}

int SanSymToolReadAddrDat(unsigned long idx, char **file, char **function, unsigned long *line, unsigned long *column) {
  if (!(pAddrInfoBuf)) { return (int) err_has_nullptr; }

  if (idx >= (pAddrInfoBuf->frames).size()) { return (int) err_outofbound; }

  struct SANSYMTOOL_NS::FrameDat * pframe = &(pAddrInfoBuf->frames[idx]);
  *file     = pframe->file;
  *function = pframe->func;
  *line     = pframe->lin;
  *column   = pframe->col;

  return (int) yes_read_done;
}

int SanSymToolSendDataDat(char *module, unsigned int offset) {
  if (!(pSanSymTool && pDataInfoBuf)) { return (int) err_has_nullptr; }

  pDataInfoBuf->module        = module;
  pDataInfoBuf->module_offset = offset;
  pDataInfoBuf->module_arch   = SANSYMTOOL_NS::kModuleArchUnknown;

  if (pSanSymTool->SymbolizeData(pDataInfoBuf)) {
    return (int) yes_send_done;
  } else {
    return (int) err_symbolize_failed;
  }
}

int SanSymToolReadDataDat(char **file, char **name, unsigned long *line, unsigned long *start, unsigned long *size) {
  if (!(pDataInfoBuf)) { return (int) err_has_nullptr; }

  *file  = pDataInfoBuf->file;
  *name  = pDataInfoBuf->name;
  *line  = pDataInfoBuf->line;
  *start = pDataInfoBuf->start;
  *size  = pDataInfoBuf->size;

  return (int) yes_read_done;
}


extern "C" {

SANITIZER_INTERFACE_ATTRIBUTE
int SanSymTool_init(const char * external_symbolizer_path) {
  return SanSymToolInit(external_symbolizer_path);
}

SANITIZER_INTERFACE_ATTRIBUTE
void SanSymTool_fini(void) {
  SanSymToolFini();
}

SANITIZER_INTERFACE_ATTRIBUTE
int SanSymTool_addr_send(char *module, unsigned int offset, unsigned long *n_frames) {
  return SanSymToolSendAddrDat(module, offset, n_frames);
}

SANITIZER_INTERFACE_ATTRIBUTE
int SanSymTool_addr_read(unsigned long idx, char **file, char **function, unsigned long *line, unsigned long *column) {
  return SanSymToolReadAddrDat(idx, file, function, line, column);
}

SANITIZER_INTERFACE_ATTRIBUTE
void SanSymTool_addr_free(void) {
  SanSymToolFreeAddrRes();
}

SANITIZER_INTERFACE_ATTRIBUTE
int SanSymTool_data_send(char *module, unsigned int offset) {
  return SanSymToolSendDataDat(module, offset);
}

SANITIZER_INTERFACE_ATTRIBUTE
int SanSymTool_data_read(char **file, char **name, unsigned long *line, unsigned long *start, unsigned long *size) {
  return SanSymToolReadDataDat(file, name, line, start, size);
}

SANITIZER_INTERFACE_ATTRIBUTE
void SanSymTool_data_free(void) {
  SanSymToolFreeDataRes();
}

} // extern "C"