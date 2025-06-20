#pragma once

#include <stddef.h>

void* mem_alloc(size_t sz);
void* mem_realloc(void* ptr, size_t sz);
void  mem_free(void* ptr);

void* temp_alloc(size_t sz);
void  temp_free();
