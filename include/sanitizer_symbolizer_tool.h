#ifndef SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H
#define SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

int SanSymTool_init(void);
int SanSymTool_fini(void);

int SanSymTool_send(void);
int SanSymTool_recv(void);
int SanSymTool_free(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif