#include "memory.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define TEMP_BUFFER_SIZE 1024 * 1024

static uint8_t buf[TEMP_BUFFER_SIZE] = {0};
static size_t  offset;

void* mem_alloc(size_t sz) { return malloc(sz); }
void* mem_realloc(void* ptr, size_t sz) { return realloc(ptr, sz); }
void  mem_free(void* ptr) { free(ptr); }

void* temp_alloc(size_t sz) {
    if (offset + sz > TEMP_BUFFER_SIZE) {
        assert(0 && "Ran out of temporary storage");
        return NULL;
    }
    void* ptr = &buf[offset];
    offset += sz;
    return ptr;
}
void temp_free() {
    offset = 0;
}
