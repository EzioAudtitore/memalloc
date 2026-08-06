#include "../src/mymalloc.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NUM_THREADS 8
#define PTRS_PER_THREAD 64
#define OPS_PER_THREAD 5000

typedef struct {
    int id;
} thread_arg_t;

static void *worker(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    unsigned int seed = 1000 + targ->id;

    void *ptrs[PTRS_PER_THREAD] = {0};
    size_t sizes[PTRS_PER_THREAD] = {0};
    unsigned char tag = (unsigned char)(targ->id % 256);

    for (int op = 0; op < OPS_PER_THREAD; op++) {
        int i = rand_r(&seed) % PTRS_PER_THREAD;

        if (ptrs[i] == NULL) {
            size_t sz = 1 + (rand_r(&seed) % 256);
            ptrs[i] = mymalloc(sz);
            assert(ptrs[i] != NULL);
            sizes[i] = sz;
            memset(ptrs[i], tag, sz);
        } else {
            unsigned char *p = (unsigned char *)ptrs[i];
            for (size_t k = 0; k < sizes[i]; k++) {
                if (p[k] != tag) {
                    fprintf(stderr, "thread %d: CORRUPTION at slot %d byte %zu\n",
                            targ->id, i, k);
                    exit(1);
                }
            }
            myfree(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    for (int i = 0; i < PTRS_PER_THREAD; i++) {
        if (ptrs[i]) myfree(ptrs[i]);
    }
    return NULL;
}

int main(void) {
    mymalloc_init(STRATEGY_FIRST_FIT);

    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].id = i;
        int rc = pthread_create(&threads[i], NULL, worker, &args[i]);
        assert(rc == 0);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("PASS: %d threads x %d ops each, no corruption or crashes\n",
           NUM_THREADS, OPS_PER_THREAD);
    return 0;
}
