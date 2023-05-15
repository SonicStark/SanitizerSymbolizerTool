#ifndef SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H
#define SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

int  SanSymTool_init(const char * external_symbolizer_path);
void SanSymTool_fini(void);

void SanSymTool_free(int free_target);

int SanSymTool_send(void);
int SanSymTool_recv(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif