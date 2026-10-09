#include "allocator.h"

#include <stdio.h>
#include <string.h>

static void show(const char *label, const Allocator *allocator) {
    AllocatorStats stats = allocator_stats(allocator);
    printf("%-22s free=%4zu  largest=%4zu  free blocks=%zu  allocated=%zu\n",
           label, stats.free_bytes, stats.largest_free_block,
           stats.free_blocks, stats.allocated_blocks);
}

int main(void) {
    Allocator allocator = {0};
    if (!allocator_init(&allocator, 4096)) {
        return 1;
    }
    show("Initial arena", &allocator);
    char *text = allocator_alloc(&allocator, 64);
    int *numbers = allocator_alloc(&allocator, 32 * sizeof(int));
    if (text == NULL || numbers == NULL) {
        allocator_destroy(&allocator);
        return 1;
    }
    strcpy(text, "Hello from the custom allocator!");
    for (int index = 0; index < 32; index++) {
        numbers[index] = index * index;
    }
    puts(text);
    printf("numbers[7] = %d\n", numbers[7]);
    show("After two allocations", &allocator);
    if (!allocator_free(&allocator, text) || !allocator_free(&allocator, numbers)) {
        allocator_destroy(&allocator);
        return 1;
    }
    show("After free + coalesce", &allocator);
    allocator_destroy(&allocator);
    return 0;
}
