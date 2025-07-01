#pragma once
#include <vulkan/vulkan.h>

#include "rtypes.h"

#define VSWAPCHAIN_MAX_IMG 3
#define VBUF_MAX_COUNT 128
#define VPASS_MAX_COUNT 16
#define VPIPE_MAX_COUNT 16

#define VCHECK(x) debug_assert((x) == VK_SUCCESS);
#define VCLAMP(x, min, max) (x < min ? min : x > max ? max : x)

typedef struct {
    rshader_type   type;
    VkShaderModule handle;
    uint32_t*      code;
    uint32_t       code_size;
} vshader;

struct rcmd {
    VkCommandBuffer handle;
};

typedef struct {
    VkImage        handle;
    VkImageView    view;
    VkDeviceMemory memory;
    VkFormat       format;
    uint32_t       width;
    uint32_t       height;
} vimage;

typedef struct {
    rbuffer_id               id;
    VkBuffer              handle;
    VkDeviceMemory        memory;
    VkDeviceSize          size;
    VkBufferUsageFlags    usage;
    VkMemoryPropertyFlags mem_props;
    void*                 mapped_ptr;
    uint32_t              is_mapped;
} vbuf;

typedef struct {
    rpass_id     id;
    VkRenderPass handle;
} vpass;

typedef struct {
    rpipe_id         id;
    VkPipeline       handle;
    VkPipelineLayout layout;
} vpipe;

typedef struct {
    VkDevice                         handle;
    VkPhysicalDevice                 physical;
    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkQueue                          graphics_queue;
    VkQueue                          compute_queue;
    VkQueue                          transfer_queue;
    VkCommandPool                    cmd_pool;
    uint32_t                         graphics_family;
    uint32_t                         compute_family;
    uint32_t                         transfer_family;
    uint32_t                         present_family;
} vdev;

typedef struct {
    VkSwapchainKHR     handle;
    VkDeviceMemory     depth_mem[VSWAPCHAIN_MAX_IMG];
    VkImage            color_imgs[VSWAPCHAIN_MAX_IMG];
    VkImage            depth_imgs[VSWAPCHAIN_MAX_IMG];
    VkImageView        color_views[VSWAPCHAIN_MAX_IMG];
    VkImageView        depth_views[VSWAPCHAIN_MAX_IMG];
    VkFramebuffer      framebuffers[VSWAPCHAIN_MAX_IMG];
    VkPresentModeKHR   present_mode;
    VkSurfaceFormatKHR surface_fmt;
    vpass              rpass;
    VkExtent2D         extent;
    VkFormat           depth_fmt;
    uint32_t           image_count;
} vswapchain;

typedef struct {
    VkAllocationCallbacks* allocator;
    VkInstance             instance;
    VkSurfaceKHR           surface;
    vswapchain             swapchain;
    vdev                   dev;
    vbuf                   buffers[VBUF_MAX_COUNT];
    rcmd                   rcmds[VSWAPCHAIN_MAX_IMG];
    VkFence                inflight_fences[VSWAPCHAIN_MAX_IMG];
    VkSemaphore            image_available_semaphores[VSWAPCHAIN_MAX_IMG];
    VkSemaphore            render_finished_semaphores[VSWAPCHAIN_MAX_IMG];
    vpipe                  pipes[VPIPE_MAX_COUNT];
    rdev_wnd               window_api;
    uint32_t               image_index;
    uint32_t               current_frame;
    uint32_t               swapchain_needs_resize;
} vstate;

//=========================================================
//
// context
//
//=========================================================

VkResult vcreate_instance(vstate* rdev);

VkResult vcreate_device(vstate* rdev);
void     vdestroy_device(vstate* rdev);

//=========================================================
//
// presentation resources
//
//=========================================================

VkResult vcreate_surface_wl(vstate* v, void* wnd_native);
VkResult vcreate_surface_xcb(vstate* v, void* wnd_native);
VkResult vcreate_surface_win32(vstate* v, void* wnd_native);

VkResult vcreate_swapchain(vstate* v, vswapchain* sc, VkExtent2D extent);
void     vdestroy_swapchain(vstate* v, vswapchain* sc);

VkResult vcreate_framebuffers(vstate* v, vswapchain* sc);
void     vdestroy_framebuffers(vstate* v, vswapchain* sc);

//=========================================================
//
// graphics resources
//
//=========================================================

VkResult vcreate_swapchainpass(vstate* v, vswapchain* sc);
void     vdestroy_swapchainpass(vstate* v, vswapchain* sc);

VkResult vcreate_semaphores(vstate* v, uint32_t count);
void     vdestroy_semaphores(vstate* v, uint32_t count);

VkResult vcreate_fences(vstate* v, uint32_t count);
void     vdestroy_fences(vstate* v, uint32_t count);

VkResult vcreate_pipeline(vstate* v, vpipe* pipe, rpipe_params* params,
                          vshader* modules);
void     vdestroy_pipeline(vstate* v, vpipe* pipe);

VkResult vcreate_shader_modules(vstate* v, vshader* shaders, uint32_t count);
void     vdestroy_shader_modules(vstate* v, vshader* shaders, uint32_t count);

VkRenderPass vrenderpass_from_id(vstate* v, rpass_id id);
VkResult     vcreate_fences(vstate* v, uint32_t count);
void         vdestroy_fences(vstate* v, uint32_t count);

VkCommandBuffer vbegin_transfer_cmd(vstate* v);
void            vend_transfer_cmd(vstate* v, VkCommandBuffer cmd);



VkResult vcreate_buffer(vstate* v, rbuf_params* params, vbuf* buf);
void vdestroy_buffer(vstate* v, vbuf* buf);

VkResult vupload_buffer(vstate* v, vbuf* buf, void* data, uint32_t size, uint32_t offset);
VkResult vdownload_buffer(vstate* v, vbuf* buf, void* data, uint32_t size, uint32_t offset);

void* vbuffer_map(vstate* v, vbuf* buf);
void vbuffer_unmap(vstate* v, vbuf* buf);

#ifdef _DEBUG
VkResult vcreate_dbg_msgr(vstate* v, VkDebugUtilsMessengerEXT* m);
void     vdestroy_dbg_msgr(vstate* v, VkDebugUtilsMessengerEXT m);
#endif  // _DEBUG
