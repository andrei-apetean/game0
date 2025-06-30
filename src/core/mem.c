#include "mem.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../base.h"

#define round_align(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

typedef struct free_block free_block;

typedef struct {
    uint8_t* mem;
    size_t   size;
    size_t   cursor;
    size_t   prev_cursor;
} stack_allocator;

struct free_block {
    size_t      size;
    free_block* next;
};

typedef struct {
    uint8_t*    mem;
    size_t      size;
    free_block* list;
} heap_allocator;

static void* stack_alloc_impl(allocator* ator, size_t size, size_t align) {
    debug_assert(ator);
    stack_allocator* data = (stack_allocator*)ator->data;
    debug_assert(data);

    size_t aligned_size = round_align(size, align);

    if (data->cursor + aligned_size > data->size) {
        return NULL;
    }

    void* ptr = data->mem + data->cursor;
    data->prev_cursor = data->cursor;
    data->cursor += aligned_size;
    return ptr;
}

static void stack_free_impl(allocator* ator, void* ptr) {
    unused(ator);
    unused(ptr);
    debug_log(
        "allocator free called on the stack allocator - this should not occur");
}

static void stack_reset_impl(allocator* ator) {
    debug_assert(ator);
    stack_allocator* data = (stack_allocator*)ator->data;
    debug_assert(data);
    data->cursor = 0;
    data->prev_cursor = 0;
}

static void stack_get_info_impl(allocator* ator, size_t* used, size_t* total) {
    debug_assert(ator);
    stack_allocator* data = (stack_allocator*)ator->data;
    debug_assert(data);
    *used = data->cursor;
    *total = data->size;
}

static void stack_destroy_impl(allocator* ator) {
    debug_assert(ator);
    stack_allocator* data = (stack_allocator*)ator->data;
    debug_assert(data);
    if (data) {
        free(data->mem);
        free(data);
    }
    free(ator);
}

static allocfn stack_allocfn = {
    .alloc = stack_alloc_impl,
    .free = stack_free_impl,
    .reset = stack_reset_impl,
    .get_info = stack_get_info_impl,
    .destroy = stack_destroy_impl,
};

static allocator* stack_create_impl(mem_tag tag, size_t size) {
    allocator* ator = malloc(sizeof(*ator));
    if (!ator) return NULL;
    stack_allocator* data = malloc(sizeof(*data));
    if (!data) {
        free(ator);
        return NULL;
    }
    data->mem = malloc(size);
    if (!data->mem) {
        free(ator);
        free(data);
        return NULL;
    }
    data->size = size;
    data->cursor = 0;
    data->prev_cursor = 0;
    ator->data = data;
    ator->tag = tag;
    ator->fn = &stack_allocfn;
    return ator;
}

static void* heap_alloc_impl(allocator* ator, size_t size, size_t align) {
    debug_assert(ator);
    heap_allocator* data = (heap_allocator*)ator->data;
    debug_assert(data);

    size_t       aligned_size = round_align(size, align);
    free_block** current = &data->list;
    size_t block_sz = sizeof(free_block);
    while (*current) {
        free_block* block = *current;
        if (block->size >= aligned_size) {
            if (block->size > aligned_size + sizeof(free_block)) {
                *current = block->next;
                uint8_t* b = (uint8_t*)block;
                free_block* new_block = (free_block*)(b + block_sz + aligned_size);

                new_block->size = block->size - aligned_size - block_sz;
                new_block->next = block->next;
                block->size = aligned_size;
                *current = new_block;
            } else {
                *current = block->next;
            }
            return (uint8_t*)block + sizeof(free_block);
        }
        current = &block->next;
    }
    return NULL;
}

static void heap_free_impl(allocator* ator, void* ptr) {
    debug_assert(ator);
    heap_allocator* data = (heap_allocator*)ator->data;
    debug_assert(data);

    free_block* block_to_free = (free_block*)((uint8_t*)ptr - sizeof(free_block));
    free_block** current = &data->list;
    free_block* prev = NULL;

    while(*current && *current < block_to_free) {
        prev = *current;
        current = &(*current)->next;
    }
    if (prev && ((uint8_t*)prev + sizeof(free_block) + prev->size) == (uint8_t*)block_to_free) {
        prev->size += sizeof(free_block) + block_to_free->size;
        block_to_free = prev;
    } else {
        block_to_free->next = *current;
        *current = block_to_free;
    }

    free_block* next_block = block_to_free->next;
    if (next_block && ((uint8_t*)block_to_free + sizeof(free_block) + block_to_free->size == (uint8_t*)next_block)) {
        block_to_free->size += sizeof(free_block) + next_block->size;
        block_to_free->next = next_block->next;
    }
}

static void heap_reset_impl(allocator* ator) {
    debug_assert(ator);
    heap_allocator* data = (heap_allocator*)ator->data;
    debug_assert(data);

    data->list = (free_block*)data->mem;
    data->list->size = data->size;
    data->list->next = NULL;
}

static void heap_get_info_impl(allocator* ator, size_t* used, size_t* total) {
    debug_assert(ator);
    heap_allocator* data = (heap_allocator*)ator->data;
    debug_assert(data);

    *total = data->size;
    *used = 0;
    size_t free_bytes = 0;
    free_block* current = data->list;
    while (current) {
        free_bytes += current->size + sizeof(free_block);
        current = current->next;
    }
    *used = *total - free_bytes;
}

static void heap_destroy_impl(allocator* ator) {
    debug_assert(ator);
    heap_allocator* data = (heap_allocator*)ator->data;
    debug_assert(data);
    free(data->mem);
    free(data);
    free(ator);
}

static allocfn heap_allocfn = {
    .alloc = heap_alloc_impl,
    .free = heap_free_impl,
    .reset = heap_reset_impl,
    .get_info = heap_get_info_impl,
    .destroy = heap_destroy_impl,
};

static allocator* heap_create_impl(mem_tag tag, size_t size) {
    allocator* ator = malloc(sizeof(*ator));
    if (!ator) return NULL;
    heap_allocator* data = malloc(sizeof(*data));
    if (!data) {
        free(ator);
        return NULL;
    }
    data->mem = malloc(size);
    if(!data->mem) {
        free(data);
        free(ator);
        return NULL;
    }
    ator->tag = tag;
    ator->fn = &heap_allocfn;
    data->size = size - sizeof(free_block);
    data->list = (free_block*)data->mem;
    data->list->next = NULL;
    return ator;
}

allocator* allocator_create(allocator_type type, mem_tag tag, size_t size) {
    switch (type) {
        case ALLOCATOR_TYPE_STACK:
            return stack_create_impl(tag, size);
        case ALLOCATOR_TYPE_HEAP:
            return heap_create_impl(tag, size);
        default:
            return NULL;
    }
}
