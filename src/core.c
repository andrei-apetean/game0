#include "core.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    module_info info;
    void*       module_data;
    void*       module_state;
} module;

typedef struct {
    module*  modules;
    uint32_t module_count;
} core_state;

static core_state core = {0};

// todo, single allocation

int32_t core_load_modules(module_info* infos, uint32_t count) {
    core.modules = malloc(count * sizeof(module));
    core.module_count = count;
    memset(core.modules, 0, sizeof(module) * count);
    assert(core.modules && "Failed to allocate module array");

    for (uint32_t i = 0; i < core.module_count; i++) {
        module_info info = infos[i];
        uint32_t    module_size = info.get_size();
        void*       module_data = malloc(module_size);
        assert(module_data && "Failed to allocate module data");

        info.init(module_data);

        uint32_t state_size = info.get_state_size(module_data);
        void*    module_state = state_size > 0 ? malloc(state_size) : NULL;

        assert(module_state && "Failed to allocate module state");

        printf("Starting up %s...\n", info.module_name);
        if (info.startup(module_data, module_state) == 0) {
            printf("%s startup successful...\n", info.module_name);
        } else {
            // todo, leaking memory
            return -1;
        }
        core.modules[i] = (module){
            .info = info,
            .module_data = module_data,
            .module_state = module_state,
        };
    }
    return 0;
}

void* core_get_module(uint32_t module_id) {
    assert(module_id < core.module_count && "Unregistered module!");
    for (uint32_t i = 0; i < core.module_count; i++) {
        if (core.modules[i].info.module_id == module_id) {
            return core.modules[i].module_data;
        }
    }
    assert(0 && "Unregistered module");
}

void core_teardown() {
    for (uint32_t i = core.module_count - 1; i > 0; i--) {
        module m = core.modules[i];
        m.info.teardown(core.modules[i].module_data);
        free(m.module_state);
        free(m.module_data);
    }
    free(core.modules);
}
