#pragma once

#include <stddef.h>
#include <stdint.h>

#define mkilo(bytes) (bytes) * 1024 
#define mmega(bytes) (bytes) * 1024 * 1024
#define mgiga(bytes) (bytes) * 1024 * 1024 * 1024

typedef enum  {
    ALLOCATOR_TYPE_STACK,
    ALLOCATOR_TYPE_HEAP,
    ALLOCATOR_TYPE_MAX_ENUM,
} allocator_type;

typedef enum {
    MEM_TAG_CORE = 0,
    MEM_TAG_RENDERER,
    MEM_TAG_MAX_ENUM
} mem_tag;

typedef struct allocator allocator;

typedef struct {
    void* (*alloc)(allocator* allocator, size_t size, size_t align);
    void (*free)(allocator* allocator, void* ptr);
    void (*reset)(allocator* allocator);
    void (*get_info)(allocator* allocator, size_t* used, size_t* total);
    void (*destroy)(allocator* allocator);
} allocfn;

struct allocator {
    allocfn*       fn;
    allocator_type type;
    mem_tag        tag;
    void*          data;
};

#define aalloc(ator, size) ((ator)->fn->alloc((ator), (size), 8))
#define aalloc_algn(ator, size, align) ((ator)->fn->alloc((ator), (size), (align))))
#define afree(ator, ptr) ((ator)->fn->free((ator), (ptr)))
#define areset(ator, size) ((ator)->fn->reset((ator)))
#define aget_info(ator, used, total) ((ator)->fn->get_info((ator), (used), (total)))
#define adestroy(ator) ((ator)->fn->destroy((ator)))

allocator* allocator_create(allocator_type type, mem_tag tag, size_t size);
