#include "../src/mymalloc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    mymalloc_init(STRATEGY_FIRST_FIT);

    char *a = mymalloc(64);
    assert(a != NULL);
    strcpy(a, "hello allocator");
    assert(strcmp(a, "hello allocator") == 0);

    assert(mymalloc(0) == NULL);
    myfree(NULL);

    int *arr = mycalloc(10, sizeof(int));
    for (int i = 0; i < 10; i++) assert(arr[i] == 0);

    char *b = mymalloc(8);
    strcpy(b, "abcdefg");
    b = myrealloc(b, 64);
    assert(strcmp(b, "abcdefg") == 0);

    myfree(a);
    myfree(arr);
    myfree(b);

    char *c = mymalloc(16);
    myfree(c);
    myfree(c);

    printf("PASS: all basic checks succeeded\n");
    return 0;
}
