#ifndef SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H
#define SANSYMTOOL_HEAD_PUBLIC_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

int SanSymTool_init(const char * external_symbolizer_path);
void SanSymTool_fini(void);

unsigned long SanSymTool_addr_send(char *module, unsigned int offset);
int SanSymTool_addr_read(unsigned long idx, char **file, char **function, unsigned long *line, unsigned long *column);
void SanSymTool_addr_free(void);

int SanSymTool_data_send(char *module, unsigned int offset);
int SanSymTool_data_read(char **file, char **name, unsigned long *line, unsigned long *start, unsigned long *size);
void SanSymTool_data_free(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif