#include <assert.h>
#include "render_module/vulkan/render_backend_vk.h"
#include "render_module/vulkan/render_context.h"
#include "render_module/vulkan/render_types_vk.h"

int32_t get_render_state_size_vk() {return sizeof(render_context_vk);}

int32_t connect_device_vk(render_state* state, render_device_params* params) {
    render_context_vk* ctx = (render_context_vk*)state;
    assert(ctx);
    *ctx = (render_context_vk){0};
    context_initialize_vk(ctx);

    create_surface_vk(ctx);
    create_device_vk(ctx);
    create_swapchain_vk(ctx);
    
return 0;  
}

void disconnect_device_vk(render_state* state) {
      render_context_vk* ctx = (render_context_vk*)state;
    assert(ctx);

    context_terminate_vk(ctx);
}
