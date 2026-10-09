#include "allocator.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct Block {
    size_t size;
    bool available;
    struct Block *next;
} Block;

#define ALIGNMENT _Alignof(max_align_t)
#define HEADER_SIZE ((sizeof(Block) + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT)

static unsigned char *payload(Block *block) {
    return (unsigned char *)block + HEADER_SIZE;
}

bool allocator_init(Allocator *allocator, size_t capacity) {
    if (allocator == NULL || allocator->memory != NULL) {
        return false;
    }
    capacity -= capacity % ALIGNMENT;
    if (capacity < HEADER_SIZE + ALIGNMENT) {
        return false;
    }
    unsigned char *memory = malloc(capacity);
    if (memory == NULL) {
        return false;
    }
    allocator->memory = memory;
    allocator->capacity = capacity;
    Block *first = (Block *)memory;
    *first = (Block){capacity - HEADER_SIZE, true, NULL};
    return true;
}

void allocator_destroy(Allocator *allocator) {
    if (allocator != NULL) {
        free(allocator->memory);
        *allocator = (Allocator){0};
    }
}

void *allocator_alloc(Allocator *allocator, size_t size) {
    if (allocator == NULL || allocator->memory == NULL || size == 0 ||
        size > SIZE_MAX - (ALIGNMENT - 1)) {
        return NULL;
    }
    size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;
    for (Block *block = (Block *)allocator->memory; block != NULL; block = block->next) {
        if (!block->available || block->size < size) {
            continue;
        }
        /* Split only when the remainder can hold metadata and an aligned payload. */
        if (block->size - size >= HEADER_SIZE + ALIGNMENT) {
            Block *remainder = (Block *)(payload(block) + size);
            *remainder = (Block){block->size - size - HEADER_SIZE, true, block->next};
            block->next = remainder;
            block->size = size;
        }
        block->available = false;
        return payload(block);
    }
    return NULL;
}

bool allocator_free(Allocator *allocator, void *pointer) {
    if (pointer == NULL) {
        return true;
    }
    if (allocator == NULL || allocator->memory == NULL) {
        return false;
    }
    Block *found = NULL;
    /* Compare against known payload addresses without dereferencing the input. */
    for (Block *block = (Block *)allocator->memory; block != NULL; block = block->next) {
        if ((void *)payload(block) == pointer) {
            found = block;
            break;
        }
    }
    if (found == NULL || found->available) {
        return false;
    }
    found->available = true;
    /* The list is in physical order: adjacent free blocks can be coalesced. */
    for (Block *block = (Block *)allocator->memory; block != NULL;) {
        if (block->available && block->next != NULL && block->next->available) {
            Block *next = block->next;
            block->size += HEADER_SIZE + next->size;
            block->next = next->next;
        } else {
            block = block->next;
        }
    }
    return true;
}

AllocatorStats allocator_stats(const Allocator *allocator) {
    AllocatorStats stats = {0};
    if (allocator == NULL || allocator->memory == NULL) {
        return stats;
    }
    stats.capacity = allocator->capacity;
    for (Block *block = (Block *)allocator->memory; block != NULL; block = block->next) {
        if (block->available) {
            stats.free_blocks++;
            stats.free_bytes += block->size;
            if (block->size > stats.largest_free_block) {
                stats.largest_free_block = block->size;
            }
        } else {
            stats.allocated_blocks++;
        }
    }
    return stats;
}
