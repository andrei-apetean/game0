#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "platform.h"
#include "render.h"

platform_state* p_state = NULL;
render_state*   r_state = NULL;

int main() {
    {
        size_t platform_state_size = platform_get_size();
        if (platform_state_size == 0) {
            printf("platform_size requirement is invalide!\n");
            return -1;
        }
        p_state = malloc(platform_state_size);
        if (p_state == NULL) {
            printf("Failed to allocate platform_state!\n");
            return -1;
        }
    }

    if (platform_startup(p_state) == false) {
        printf("Failed to startup platform!\n");
        return -1;
    }
    {
        size_t render_state_size = render_get_size();
        if (render_state_size == 0) {
            printf("platform_size requirement is invalide!\n");
            return -1;
        }
        r_state = malloc(render_state_size);
        if (r_state == NULL) {
            printf("Failed to allocate platform_state!\n");
            return -1;
        }
    }

    if (render_startup(r_state) == false) {
        printf("Failed to startup render!\n");
        return -1;
    }
    // while (platform_is_running(p_state)) {
    //     ret = platform_poll_events(p_state);
    //     if (ret == false) {
    //         break;
    //     }

    //     platform_update(p_state);
    // }
    render_shutdown(r_state);
    platform_shutdown(p_state);

    free(p_state);
    return 0;
}

#include "platform.c"
#include "render.c"
