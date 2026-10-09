#include "allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_edge_cases(void) {
    Allocator allocator = {0};
    assert(!allocator_init(NULL, 4096));
    assert(!allocator_init(&allocator, 1));
    assert(allocator_alloc(&allocator, 32) == NULL);
    assert(allocator_init(&allocator, 4096));
    assert(!allocator_init(&allocator, 4096));
    assert(allocator_alloc(&allocator, 0) == NULL);
    assert(allocator_alloc(&allocator, SIZE_MAX) == NULL);
    assert(allocator_alloc(&allocator, 8192) == NULL);
    assert(allocator_free(&allocator, NULL));
    int unrelated = 0;
    assert(!allocator_free(&allocator, &unrelated));
    unsigned char *pointer = allocator_alloc(&allocator, 17);
    assert(pointer != NULL);
    assert((uintptr_t)pointer % _Alignof(max_align_t) == 0);
    memset(pointer, 42, 17);
    assert(!allocator_free(&allocator, pointer + 1));
    assert(allocator_free(&allocator, pointer));
    assert(!allocator_free(&allocator, pointer));
    allocator_destroy(&allocator);
    allocator_destroy(&allocator);
    assert(allocator_stats(&allocator).capacity == 0);
    assert(allocator_init(&allocator, 4096));
    allocator_destroy(&allocator);
    puts("PASS: alignment, zero/overflow, invalid/double free, lifecycle");
}

static void test_split_reuse_coalesce(void) {
    Allocator allocator = {0};
    assert(allocator_init(&allocator, 4096));
    size_t original = allocator_stats(&allocator).free_bytes;
    void *first = allocator_alloc(&allocator, 1024);
    void *second = allocator_alloc(&allocator, 1024);
    unsigned char *third = allocator_alloc(&allocator, 1024);
    assert(first != NULL && second != NULL && third != NULL);
    memset(third, 73, 1024);
    assert(allocator_alloc(&allocator, 1800) == NULL);
    assert(allocator_free(&allocator, first));
    void *reused = allocator_alloc(&allocator, 1024);
    assert(reused == first);
    assert(allocator_free(&allocator, reused));
    assert(allocator_free(&allocator, second));
    void *merged = allocator_alloc(&allocator, 1800);
    assert(merged == first);
    for (size_t index = 0; index < 1024; index++) {
        assert(third[index] == 73);
    }
    assert(allocator_free(&allocator, third));
    assert(allocator_free(&allocator, merged));
    AllocatorStats final = allocator_stats(&allocator);
    assert(final.free_blocks == 1 && final.allocated_blocks == 0);
    assert(final.free_bytes == original && final.largest_free_block == original);
    void *whole_arena = allocator_alloc(&allocator, original);
    assert(whole_arena != NULL);
    assert(allocator_alloc(&allocator, 1) == NULL);
    assert(allocator_free(&allocator, whole_arena));
    allocator_destroy(&allocator);
    puts("PASS: splitting, first-fit reuse, exhaustion, adjacent coalescing");
}

typedef struct {
    unsigned char *pointer;
    size_t size;
    unsigned char pattern;
} Allocation;

static uint32_t random_state = 2026;

static uint32_t next_random(void) {
    random_state = random_state * UINT32_C(1664525) + UINT32_C(1013904223);
    return random_state;
}

static void verify(const Allocation slots[64]) {
    for (size_t index = 0; index < 64; index++) {
        if (slots[index].pointer == NULL) {
            continue;
        }
        assert((uintptr_t)slots[index].pointer % _Alignof(max_align_t) == 0);
        for (size_t byte = 0; byte < slots[index].size; byte++) {
            assert(slots[index].pointer[byte] == slots[index].pattern);
        }
        for (size_t other = index + 1; other < 64; other++) {
            if (slots[other].pointer != NULL) {
                uintptr_t a = (uintptr_t)slots[index].pointer;
                uintptr_t b = (uintptr_t)slots[other].pointer;
                assert(a + slots[index].size <= b || b + slots[other].size <= a);
            }
        }
    }
}

static void test_stress(void) {
    Allocator allocator = {0};
    Allocation slots[64] = {0};
    assert(allocator_init(&allocator, 16 * 1024));
    size_t original = allocator_stats(&allocator).free_bytes;
    for (size_t iteration = 0; iteration < 10000; iteration++) {
        verify(slots);
        /* Use high PRNG bits to avoid a repeating slot pattern. */
        size_t index = (next_random() >> 16) % 64;
        if (slots[index].pointer != NULL) {
            assert(allocator_free(&allocator, slots[index].pointer));
            slots[index] = (Allocation){0};
        } else {
            size_t size = 1 + (next_random() >> 16) % 512;
            unsigned char *pointer = allocator_alloc(&allocator, size);
            if (pointer != NULL) {
                unsigned char pattern = (unsigned char)(next_random() >> 16);
                memset(pointer, pattern, size);
                slots[index] = (Allocation){pointer, size, pattern};
            }
        }
    }
    verify(slots);
    for (size_t index = 0; index < 64; index++) {
        assert(allocator_free(&allocator, slots[index].pointer));
    }
    assert(allocator_stats(&allocator).free_blocks == 1);
    assert(allocator_stats(&allocator).free_bytes == original);
    allocator_destroy(&allocator);
    puts("PASS: 10,000 deterministic operations, contents and non-overlap");
}

int main(void) {
    test_edge_cases();
    test_split_reuse_coalesce();
    test_stress();
    puts("All allocator tests passed.");
    return 0;
}
