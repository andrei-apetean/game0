#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct scratch_allocator scratch_allocator;
typedef struct allocator allocator;

void mem_init(void);
void mem_term(void);
allocator* mem_allocator();
scratch_allocator* mem_scratch_allocator();

scratch_allocator* scratch_init(size_t bytes);
void*  scratch_alloc(scratch_allocator* ator, size_t bytes, size_t align);
size_t scratch_mark(scratch_allocator* ator);
void   scratch_restore(scratch_allocator* ator, size_t marker);
void   scratch_reset(scratch_allocator* ator);

allocator* allocator_init(size_t bytes);
void* allocator_alloc(allocator* allocator, size_t bytes, size_t align);
void allocator_free(allocator* allocator, void* ptr);

