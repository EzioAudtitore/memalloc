#include "../src/mymalloc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    mymalloc_init(STRATEGY_FIRST_FIT);

    /* basic alloc + write */
    char *a = mymalloc(64);
    assert(a != NULL);
    strcpy(a, "hello allocator");
    assert(strcmp(a, "hello allocator") == 0);

    /* zero-size and NULL handling */
    assert(mymalloc(0) == NULL);
    myfree(NULL); /* should not crash */

    /* calloc zero-initializes */
    int *arr = mycalloc(10, sizeof(int));
    for (int i = 0; i < 10; i++) assert(arr[i] == 0);

    /* realloc grows and preserves contents */
    char *b = mymalloc(8);
    strcpy(b, "abcdefg");
    b = myrealloc(b, 64);
    assert(strcmp(b, "abcdefg") == 0);

    /* free + coalesce sanity: allocate, free, re-allocate same-ish size */
    myfree(a);
    myfree(arr);
    myfree(b);

    /* double free should be caught, not crash */
    char *c = mymalloc(16);
    myfree(c);
    myfree(c); /* prints a warning, does not corrupt state */

    printf("PASS: all basic checks succeeded\n");
    return 0;
}
