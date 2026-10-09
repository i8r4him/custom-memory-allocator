#ifndef CUSTOM_ALLOCATOR_H
#define CUSTOM_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

/* Initialize with {0}. One allocator owns one fixed-capacity arena. */
typedef struct {
    unsigned char *memory;
    size_t capacity;
} Allocator;

typedef struct {
    size_t capacity;
    size_t free_bytes;
    size_t largest_free_block;
    size_t free_blocks;
    size_t allocated_blocks;
} AllocatorStats;

/* Reserves an arena once using system malloc; allocations then use this arena. */
bool allocator_init(Allocator *allocator, size_t capacity);
void allocator_destroy(Allocator *allocator);

/* Returns max_align_t-aligned memory, or NULL for zero/overflow/exhaustion. */
void *allocator_alloc(Allocator *allocator, size_t size);

/* NULL is a no-op. Unknown, interior and already-free pointers return false. */
bool allocator_free(Allocator *allocator, void *pointer);
AllocatorStats allocator_stats(const Allocator *allocator);

#endif
