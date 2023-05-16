#include "sanitizer_symbolizer_tool.h"

#include <stdlib.h>
#include <stdio.h>

#define USE_PROG "./target_prog"
#define USE_LLVM "./llvm-symbolizer"
#define USE_AD2L "./addr2line"

long int bin_size = 0;

void update_bin_size() {
    FILE *fp = fopen(USE_PROG, "rb");
    if (fp) {
        fseek(fp, 0L, SEEK_END);
        bin_size = ftell(fp);
        fclose(fp);
        if (bin_size <= 0) {
            printf("Corrupted "USE_PROG"\n");
            exit(0);
        }
        printf("%ld bytes in total\n", bin_size);
    } else {
        printf("Missing "USE_PROG"\n");
        exit(0);
    }
}

void code_each_offest() {
    for (long int i=0; i<=(bin_size-1); ++i) {
        printf("CODE at offset 0x%lx\n", i);
        unsigned long N = 0;
        SanSymTool_addr_send(USE_PROG, (unsigned int)i, &N);
        if (N > 0) {
            for (unsigned long j=0; j<=(N-1); ++j) {
                char *pfile;
                char *pfunc;
                unsigned long lin;
                unsigned long col;
                SanSymTool_addr_read(j, &pfile, &pfunc, &lin, &col);
                if (!(pfile && pfunc)) { printf("NULL\n"); continue; }
                printf("%s in %s:%lu:%lu\n", pfunc, pfile, lin, col);
            }
        }
        SanSymTool_addr_free();
    }
}

void data_each_offest() {
    for (long int i=0; i<=(bin_size-1); ++i) {
        printf("DATA at offset 0x%lx\n", i);
        SanSymTool_data_send(USE_PROG, (unsigned int)i);
        char *pfile;
        char *pname;
        unsigned long lin;
        unsigned long stt;
        unsigned long siz;
        SanSymTool_data_read(&pfile, &pname, &lin, &stt, &siz);
        if (!(pfile && pname)) { printf("NULL\n"); continue; }
        printf("%s in %s:%lu, %lu at %lu\n", pname, pfile, lin, siz, stt);
        SanSymTool_data_free();
    }
}

int main() {
    int init_st;

    update_bin_size();

    printf("===== Using llvm-symbolizer =====\n");
    init_st = SanSymTool_init(USE_LLVM);
    if (init_st != 0) {
        printf("Init failed. RetCode=%d\n", init_st);
        exit(0);
    }
    code_each_offest();
    data_each_offest();
    SanSymTool_fini();

    printf("===== Using addr2line =====\n");
    init_st = SanSymTool_init(USE_LLVM);
    if (init_st != 0) { 
        printf("Init failed. RetCode=%d\n", init_st);
        exit(0);
    }
    SanSymTool_init(USE_AD2L);
    code_each_offest();
    data_each_offest();
    SanSymTool_fini();
}