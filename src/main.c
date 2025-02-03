#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "platform.h"
// #include "render.h"

platform_state* p_state = NULL;
platform_interface platform;

int main() {
    bool ret = false;

    ret = get_platform_interface(&platform);
    if (ret == false) {
        printf("Failed to get platform interface!\n");
        return -1;
    }

    p_state = malloc(platform.platform_state_size);
    if (p_state == NULL) {
        printf("Failed to allocate platform state!\n");
        return -1;
    }

    ret = platform.startup(p_state);

    if (ret == false) {
        printf("Failed to startup platform!\n");
        return -1;
    }
    while (platform.is_running(p_state)) {
        ret = platform.poll_events(p_state);
        if (ret == false) {
            break;
        }

        platform.update(p_state);
    }
    platform.shutdown(p_state);

    free(p_state);
    return 0;
}

#include "platform.c"
// #include "render_vulkan.c"
// #include "vulkan_platform_wl.c"
