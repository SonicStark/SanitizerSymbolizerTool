#include "common.h"
#include "stdlib.h"
#include  <cstdio>

namespace SANSYMTOOL_NS
{

void Die() { exit(SANSYMTOOL_EXITCODE); }

void CheckFailed(const char *file, int line, const char *cond,
                          u64 v1, u64 v2) {
    fprintf(stderr, SANSYMTOOL_MYNAME ": CHECK failed: %s:%d \"%s\" (0x%zx, 0x%zx)\n",
            file, line, cond, v1, v2);
    Die();
}

}