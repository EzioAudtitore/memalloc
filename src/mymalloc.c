#include "mymalloc.h"

#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define ARENA_SIZE      (1 << 16)
#define ALIGNMENT       16
#define MIN_SPLIT_LEFT  32

typedef struct header {
    size_t size;
    int is_free;
    struct header *next;
    struct header *phys_next;
} header_t;

typedef struct arena {
    void *base;
    size_t size;
    struct arena *next;
} arena_t;

static header_t *free_list = NULL;
static arena_t *arenas = NULL;
static strategy_t g_strategy = STRATEGY_FIRST_FIT;
static size_t g_arena_count = 0;
static size_t g_arena_bytes_total = 0;

static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static size_t align_up(size_t n, size_t a) {
    return (n + a - 1) & ~(a - 1);
}

static void free_list_insert(header_t *h) {
    h->is_free = 1;
    if (!free_list || h < free_list) {
        h->next = free_list;
        free_list = h;
        return;
    }
    header_t *cur = free_list;
    while (cur->next && cur->next < h) cur = cur->next;
    h->next = cur->next;
    cur->next = h;
}

static void free_list_remove(header_t *target) {
    if (free_list == target) {
        free_list = free_list->next;
        return;
    }
    header_t *cur = free_list;
    while (cur && cur->next != target) cur = cur->next;
    if (cur) cur->next = target->next;
}

static header_t *grow(size_t min_size) {
    size_t want = ARENA_SIZE;
    while (want < min_size + sizeof(header_t)) want *= 2;

    void *mem = mmap(NULL, want, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) return NULL;

    arena_t *a = (arena_t *)mmap(NULL, sizeof(arena_t), PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    a->base = mem;
    a->size = want;
    a->next = arenas;
    arenas = a;
    g_arena_count++;
    g_arena_bytes_total += want;

    header_t *h = (header_t *)mem;
    h->size = want - sizeof(header_t);
    h->is_free = 1;
    h->next = NULL;
    h->phys_next = NULL;

    free_list_insert(h);
    return h;
}

static void split_if_worthwhile(header_t *h, size_t size) {
    size_t leftover = h->size - size;
    if (leftover < sizeof(header_t) + MIN_SPLIT_LEFT) return;

    header_t *new_h = (header_t *)((char *)(h + 1) + size);
    new_h->size = leftover - sizeof(header_t);
    new_h->is_free = 1;
    new_h->phys_next = h->phys_next;

    h->phys_next = new_h;
    h->size = size;

    free_list_insert(new_h);
}

static void coalesce_forward(header_t *h) {
    header_t *nxt = h->phys_next;
    if (nxt && nxt->is_free) {
        free_list_remove(nxt);
        h->size += sizeof(header_t) + nxt->size;
        h->phys_next = nxt->phys_next;
    }
}

void mymalloc_init(strategy_t strategy) {
    pthread_mutex_lock(&alloc_mutex);
    g_strategy = strategy;
    free_list = NULL;
    arenas = NULL;
    g_arena_count = 0;
    g_arena_bytes_total = 0;
    pthread_mutex_unlock(&alloc_mutex);
}

static void *mymalloc_locked(size_t size) {
    if (size == 0) return NULL;
    size = align_up(size, ALIGNMENT);

    header_t *chosen = NULL;
    header_t *cur = free_list;

    while (cur) {
        if (cur->size >= size) {
            if (g_strategy == STRATEGY_FIRST_FIT) {
                chosen = cur;
                break;
            } else if (g_strategy == STRATEGY_BEST_FIT) {
                if (!chosen || cur->size < chosen->size) chosen = cur;
            } else {
                if (!chosen || cur->size > chosen->size) chosen = cur;
            }
        }
        cur = cur->next;
    }

    if (!chosen) {
        chosen = grow(size);
        if (!chosen) return NULL;
    }

    free_list_remove(chosen);
    split_if_worthwhile(chosen, size);
    chosen->is_free = 0;

    return (void *)(chosen + 1);
}

static void myfree_locked(void *ptr) {
    if (!ptr) return;
    header_t *h = ((header_t *)ptr) - 1;

    if (h->is_free) {
        fprintf(stderr, "mymalloc: double free detected at %p\n", ptr);
        return;
    }

    free_list_insert(h);
    coalesce_forward(h);
}

void *mymalloc(size_t size) {
    pthread_mutex_lock(&alloc_mutex);
    void *p = mymalloc_locked(size);
    pthread_mutex_unlock(&alloc_mutex);
    return p;
}

void myfree(void *ptr) {
    pthread_mutex_lock(&alloc_mutex);
    myfree_locked(ptr);
    pthread_mutex_unlock(&alloc_mutex);
}

void *mycalloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (nmemb != 0 && total / nmemb != size) return NULL;
    void *p = mymalloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *myrealloc(void *ptr, size_t size) {
    if (!ptr) return mymalloc(size);
    if (size == 0) { myfree(ptr); return NULL; }

    pthread_mutex_lock(&alloc_mutex);
    header_t *h = ((header_t *)ptr) - 1;
    if (h->size >= size) {
        pthread_mutex_unlock(&alloc_mutex);
        return ptr;
    }
    pthread_mutex_unlock(&alloc_mutex);

    void *new_ptr = mymalloc(size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, h->size);
    myfree(ptr);
    return new_ptr;
}

void mymalloc_dump_free_list(void) {
    pthread_mutex_lock(&alloc_mutex);
    printf("Free list:\n");
    for (header_t *cur = free_list; cur; cur = cur->next) {
        printf("  [%p] size=%zu\n", (void *)cur, cur->size);
    }
    pthread_mutex_unlock(&alloc_mutex);
}

size_t mymalloc_total_free(void) {
    pthread_mutex_lock(&alloc_mutex);
    size_t total = 0;
    for (header_t *cur = free_list; cur; cur = cur->next) total += cur->size;
    pthread_mutex_unlock(&alloc_mutex);
    return total;
}

void mymalloc_get_stats(mymalloc_stats_t *out) {
    if (!out) return;
    pthread_mutex_lock(&alloc_mutex);

    size_t total_free = 0, largest = 0, count = 0;
    for (header_t *cur = free_list; cur; cur = cur->next) {
        total_free += cur->size;
        if (cur->size > largest) largest = cur->size;
        count++;
    }

    out->total_free_bytes   = total_free;
    out->largest_free_block = largest;
    out->num_free_blocks    = count;
    out->num_arenas         = g_arena_count;
    out->arena_bytes_total  = g_arena_bytes_total;

    pthread_mutex_unlock(&alloc_mutex);
}
