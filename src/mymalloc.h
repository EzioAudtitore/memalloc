#ifndef MYMALLOC_H
#define MYMALLOC_H

#include <stddef.h>

typedef enum {
    STRATEGY_FIRST_FIT,
    STRATEGY_BEST_FIT,
    STRATEGY_WORST_FIT
} strategy_t;

void mymalloc_init(strategy_t strategy);
void *mymalloc(size_t size);
void myfree(void *ptr);
void *mycalloc(size_t nmemb, size_t size);
void *myrealloc(void *ptr, size_t size);

void mymalloc_dump_free_list(void);
size_t mymalloc_total_free(void);

typedef struct {
    size_t total_free_bytes;
    size_t largest_free_block;
    size_t num_free_blocks;
    size_t num_arenas;
    size_t arena_bytes_total;
} mymalloc_stats_t;

void mymalloc_get_stats(mymalloc_stats_t *out);

#endif
