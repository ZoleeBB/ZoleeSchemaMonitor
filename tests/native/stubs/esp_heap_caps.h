#pragma once
#include <stdlib.h>
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_INTERNAL 4
static inline void *heap_caps_aligned_alloc(size_t alignment, size_t size, unsigned flags)
{
    (void)flags;
    return aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
}
#define MALLOC_CAP_DMA 8
size_t heap_caps_get_free_size(unsigned caps);
size_t heap_caps_get_total_size(unsigned caps);
size_t heap_caps_get_minimum_free_size(unsigned caps);
size_t heap_caps_get_largest_free_block(unsigned caps);
