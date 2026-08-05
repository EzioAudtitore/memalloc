#ifndef MYMALLOC_H
#define MYMALLOC_H

#include <stddef.h>

/* Allocation strategies you can switch between at init time */
typedef enum {
    STRATEGY_FIRST_FIT,
    STRATEGY_BEST_FIT,
    STRATEGY_WORST_FIT
} strategy_t;

/* Must be called once before any mymalloc/myfree calls. */
void  mymalloc_init(strategy_t strategy);

/* Same signatures as the real libc functions, on purpose:
 * this lets test programs #define malloc mymalloc and just work. */
void *mymalloc(size_t size);
void  myfree(void *ptr);
void *mycalloc(size_t nmemb, size_t size);
void *myrealloc(void *ptr, size_t size);

/* Debug/inspection helpers */
void  mymalloc_dump_free_list(void);
size_t mymalloc_total_free(void);

/* Fragmentation / usage snapshot, taken atomically under the internal lock. */
typedef struct {
    size_t total_free_bytes;   /* sum of all free block sizes */
    size_t largest_free_block; /* size of the single biggest free block */
    size_t num_free_blocks;    /* how many free blocks exist right now */
    size_t num_arenas;         /* how many mmap() arenas have been requested */
    size_t arena_bytes_total;  /* total bytes ever obtained from the OS */
} mymalloc_stats_t;

void mymalloc_get_stats(mymalloc_stats_t *out);

/* All public functions are internally synchronized with a mutex, so this
 * allocator is safe to call from multiple threads concurrently. */

#endif
