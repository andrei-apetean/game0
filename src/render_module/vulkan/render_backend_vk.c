
#include "render_module/vulkan/render_backend_vk.h"

void load_render_backend_vk(render_backend* backend) {
    //dvice
    backend->get_render_device_state_size = get_render_device_state_size_vk;
    backend->connect_device = connect_device_vk;
    backend->disconnect_device = disconnect_device_vk;

    // backend->startup = startup_vk;
    // backend->teardown = teardown_vk;
    // backend->get_backend_state_size = get_backend_state_size_vk;
    // backend->render_begin = render_begin_vk;
    // backend->render_end = render_end_vk;
    // backend->resize = resize_vk;
}