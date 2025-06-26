#include "mem.h"

#include <stdalign.h>
#include <stdlib.h>

#include "base.h"

struct scratch_allocator {
    uint8_t* start;
    size_t   cursor;
    size_t   capacity;
};

typedef struct {
    scratch_allocator* scratch;
} mem_t;

static mem_t mem = {0};

//=======================================================
//
// forward declarations
//
//=======================================================

size_t round_to_align(size_t size, size_t align);

//=======================================================
//
// implementation
//
//=======================================================

void mem_init(void) {
    scratch_init(mkilo(1));
}

void mem_term(void) {
    free(mem.scratch->start);
    free(mem.scratch);
}

scratch_allocator* mem_scratch_allocator() { return mem.scratch; }

scratch_allocator* scratch_init(size_t bytes) {
    scratch_allocator* result = malloc(sizeof(scratch_allocator));
    debug_assert(result);
    if (!result) return 0;
    result->start = malloc(bytes);
    debug_assert(result->start);
    if (!result->start) {
        free(result);
        return 0;
    }
    result->cursor = 0;
    result->capacity = bytes;
    return result;
}

void* scratch_alloc(scratch_allocator* ator, size_t size, size_t align) {
    size_t next_pos = round_to_align(ator->cursor, align);
    debug_assert(next_pos < ator->capacity);
    const size_t new_size = next_pos + size;
    if (new_size > ator->capacity) {
        debug_assert(0 && "scratch allocator out of memory");
        return 0;
    }
    void* ret = ator->cursor + ator->start;
    ator->cursor = new_size;
    return ret;
}

size_t scratch_mark(scratch_allocator* ator) {
    return ator->cursor;
}
void scratch_restore(scratch_allocator* ator, size_t marker) {
    debug_assert(marker < ator->capacity);
    ator->cursor = marker;
}

void scratch_reset(scratch_allocator* ator) {
    ator->cursor = 0;
}


allocator* allocator_init(size_t bytes) {
    unused(bytes);
    unimplemented("allocator_init");
    return 0;
}
void* allocator_alloc(allocator* allocator, size_t bytes, size_t align) {
    unused(allocator);
    unused(bytes);
    unused(align);
    unimplemented("allocator_free");
    return 0;
}

void allocator_free(allocator* allocator, void* ptr) {
    unused(allocator);
    unused(ptr);
    unimplemented("allocator_free");
}

size_t round_to_align(size_t size, size_t alignment) {
    if (alignment == 0) return 0;
    size_t mask = alignment - 1;
    return (size + mask) & ~mask;
}
