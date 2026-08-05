/* Compares first/best/worst fit across several allocation patterns and
 * reports fragmentation stats. Output is CSV so it's easy to redirect
 * into a file and turn into a table/graph for a writeup. */

#include "../src/mymalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PTRS 300

typedef void (*pattern_fn)(void);

static void *g_ptrs[NUM_PTRS];

static const char *strategy_name(strategy_t s) {
    switch (s) {
        case STRATEGY_FIRST_FIT: return "first_fit";
        case STRATEGY_BEST_FIT:  return "best_fit";
        case STRATEGY_WORST_FIT: return "worst_fit";
    }
    return "?";
}

/* --- Patterns --- */

/* Random size, random alloc/free interleaving. Baseline case. */
static void pattern_random(void) {
    srand(42);
    for (int op = 0; op < 20000; op++) {
        int i = rand() % NUM_PTRS;
        if (g_ptrs[i] == NULL) {
            g_ptrs[i] = mymalloc(1 + (rand() % 512));
        } else {
            myfree(g_ptrs[i]);
            g_ptrs[i] = NULL;
        }
    }
}

/* Every allocation is the same size. Should fragment very little --
 * freed holes are always exactly the size of the next request. */
static void pattern_uniform(void) {
    srand(42);
    for (int op = 0; op < 20000; op++) {
        int i = rand() % NUM_PTRS;
        if (g_ptrs[i] == NULL) {
            g_ptrs[i] = mymalloc(64);
        } else {
            myfree(g_ptrs[i]);
            g_ptrs[i] = NULL;
        }
    }
}

/* Sizes steadily increase, then wrap back to small. Stresses whether
 * a strategy can reuse the small holes left behind as sizes grow. */
static void pattern_increasing(void) {
    int size = 16;
    for (int op = 0; op < 20000; op++) {
        int i = rand() % NUM_PTRS;
        if (g_ptrs[i] == NULL) {
            g_ptrs[i] = mymalloc(size);
            size = (size >= 2048) ? 16 : size + 16;
        } else {
            myfree(g_ptrs[i]);
            g_ptrs[i] = NULL;
        }
    }
}

/* Allocate everything up front with no interleaved frees, then free it
 * all at once. Worst case for fragmentation mid-run since nothing gets
 * a chance to coalesce until the very end. */
static void pattern_burst(void) {
    srand(42);
    for (int i = 0; i < NUM_PTRS; i++) {
        g_ptrs[i] = mymalloc(1 + (rand() % 512));
    }
    /* Free every other one, simulating some objects outliving others --
     * this is the point where we snapshot fragmentation, before the
     * final full cleanup. */
    for (int i = 0; i < NUM_PTRS; i += 2) {
        myfree(g_ptrs[i]);
        g_ptrs[i] = NULL;
    }
}

static void cleanup(void) {
    for (int i = 0; i < NUM_PTRS; i++) {
        if (g_ptrs[i]) { myfree(g_ptrs[i]); g_ptrs[i] = NULL; }
    }
}

static void run_case(const char *pattern_name, pattern_fn fn, strategy_t s) {
    memset(g_ptrs, 0, sizeof(g_ptrs));
    mymalloc_init(s);

    fn(); /* leaves the allocator in a "mid-run" state on purpose */

    mymalloc_stats_t stats;
    mymalloc_get_stats(&stats);

    double frag_ratio = (stats.total_free_bytes == 0)
        ? 0.0
        : 1.0 - ((double)stats.largest_free_block / (double)stats.total_free_bytes);

    printf("%s,%s,%zu,%zu,%zu,%zu,%zu,%.4f\n",
           pattern_name, strategy_name(s),
           stats.total_free_bytes, stats.largest_free_block,
           stats.num_free_blocks, stats.num_arenas, stats.arena_bytes_total,
           frag_ratio);

    cleanup();
}

int main(void) {
    printf("pattern,strategy,total_free_bytes,largest_free_block,num_free_blocks,"
           "num_arenas,arena_bytes_total,fragmentation_ratio\n");

    struct { const char *name; pattern_fn fn; } patterns[] = {
        { "random",     pattern_random },
        { "uniform",    pattern_uniform },
        { "increasing", pattern_increasing },
        { "burst",      pattern_burst },
    };
    strategy_t strategies[] = { STRATEGY_FIRST_FIT, STRATEGY_BEST_FIT, STRATEGY_WORST_FIT };

    for (size_t p = 0; p < sizeof(patterns)/sizeof(patterns[0]); p++) {
        for (size_t s = 0; s < sizeof(strategies)/sizeof(strategies[0]); s++) {
            run_case(patterns[p].name, patterns[p].fn, strategies[s]);
        }
    }

    return 0;
}
