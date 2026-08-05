#include "../src/mymalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NUM_PTRS 200
#define NUM_OPS  20000

int main(void) {
    mymalloc_init(STRATEGY_FIRST_FIT);
    srand(42);

    void   *ptrs[NUM_PTRS] = {0};
    size_t  sizes[NUM_PTRS] = {0};

    for (int op = 0; op < NUM_OPS; op++) {
        int i = rand() % NUM_PTRS;

        if (ptrs[i] == NULL) {
            size_t sz = 1 + (rand() % 512);
            ptrs[i] = mymalloc(sz);
            assert(ptrs[i] != NULL);
            sizes[i] = sz;
            memset(ptrs[i], (i % 256), sz);   /* canary fill */
        } else {
            /* verify canary wasn't corrupted by a neighbouring block */
            unsigned char *p = (unsigned char *)ptrs[i];
            for (size_t k = 0; k < sizes[i]; k++) {
                if (p[k] != (unsigned char)(i % 256)) {
                    fprintf(stderr, "CORRUPTION at ptr %d, byte %zu\n", i, k);
                    return 1;
                }
            }
            myfree(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    for (int i = 0; i < NUM_PTRS; i++) {
        if (ptrs[i]) myfree(ptrs[i]);
    }

    printf("PASS: %d random alloc/free ops, no corruption detected\n", NUM_OPS);
    printf("Free bytes remaining after full cleanup: %zu\n", mymalloc_total_free());
    return 0;
}
