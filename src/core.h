#pragma once

#include <stdint.h>

typedef int32_t (*pfn_module_get_size)();
typedef int32_t (*pfn_module_get_state_size)(void*);
typedef int32_t (*pfn_module_init)(void*);
typedef int32_t (*pfn_module_startup)(void*, void*);
typedef void (*pfn_module_teardown)(void*);

typedef struct {
    uint32_t                  module_id;
    const char*               module_name;
    pfn_module_get_size       get_size;
    pfn_module_get_state_size get_state_size;
    pfn_module_init           init;
    pfn_module_startup        startup;
    pfn_module_teardown       teardown;
} module_info;

int32_t core_load_modules(module_info* infos, uint32_t count);
void*   core_get_module(uint32_t module_id);
void    core_teardown();
