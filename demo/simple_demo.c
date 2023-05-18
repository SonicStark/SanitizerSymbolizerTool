#include "sanitizer_symbolizer_tool.h"

#include <stdlib.h>
#include <stdio.h>

#define USE_PROG "./bug-san0-dbg0-64.bin"
#define USE_LLVM "./llvm-symbolizer"
#define USE_AD2L "./addr2line"

#define SEC_HEAD_TEXT 0x1320U
#define SEC_TAIL_TEXT 0x1C65U

#define SEC_HEAD_DATA 0x4000U
#define SEC_TAIL_DATA 0x41B8U

#define SEC_HEAD_BSS 0x41C0U
#define SEC_TAIL_BSS 0x4460U


void code_each(unsigned int head, unsigned int tail) {
    for (unsigned int i=head; i<=tail; ++i) {
        printf("CODE at damn offset 0x%x\n", i);
        unsigned long N = 0;
        SanSymTool_addr_send(USE_PROG, i, &N);
        if (N > 0) {
            for (unsigned long j=0; j<=(N-1); ++j) {
                char *pfile;
                char *pfunc;
                unsigned long lin;
                unsigned long col;
                SanSymTool_addr_read(j, &pfile, &pfunc, &lin, &col);
                if (!pfile) { pfile = "??"; }
                if (!pfunc) { pfunc = "??"; }
                printf("%s in %s:%lu:%lu\n", pfunc, pfile, lin, col);
            }
        }
        SanSymTool_addr_free();
    }
}

void data_each(unsigned int head, unsigned int tail) {
    for (unsigned int i=head; i<=tail; ++i) {
        printf("DATA at damn offset 0x%x\n", i);
        SanSymTool_data_send(USE_PROG, (unsigned int)i);
        char *pfile;
        char *pname;
        unsigned long lin;
        unsigned long stt;
        unsigned long siz;
        SanSymTool_data_read(&pfile, &pname, &lin, &stt, &siz);
        if (!pfile) { 
            pfile = "FAIL";
        } else if (*pfile == '\n') {
            pfile = "NULL";
        } else {}
        if (!pname) { 
            pname = "FAIL";
        } else {}
        printf("%s in %s:%lu, %lu at %lu\n", pname, pfile, lin, siz, stt);
        SanSymTool_data_free();
    }
}

int main() {
    int init_st;

    printf("===== Using llvm-symbolizer =====\n");
    init_st = SanSymTool_init(USE_LLVM);
    if (init_st != 0) {
        printf("Init failed. RetCode=%d\n", init_st);
        exit(0);
    }
    code_each(SEC_HEAD_TEXT, SEC_TAIL_TEXT);
    data_each(SEC_HEAD_DATA, SEC_TAIL_DATA);
    data_each(SEC_HEAD_BSS , SEC_TAIL_BSS );
    SanSymTool_fini();

    printf("===== Using addr2line =====\n");
    init_st = SanSymTool_init(USE_LLVM);
    if (init_st != 0) { 
        printf("Init failed. RetCode=%d\n", init_st);
        exit(0);
    }
    SanSymTool_init(USE_AD2L);
    code_each(SEC_HEAD_TEXT, SEC_TAIL_TEXT);
    data_each(SEC_HEAD_DATA, SEC_TAIL_DATA);
    data_each(SEC_HEAD_BSS , SEC_TAIL_BSS );
    SanSymTool_fini();
}