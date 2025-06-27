#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "render/rtypes.h"

#define VSWAPCHAIN_MAX_IMG 3
#define VBUF_MAX_COUNT 128
#define VPASS_MAX_COUNT 16
#define VPIPE_MAX_COUNT 16

#define VCHECK(x) debug_assert((x) == VK_SUCCESS);
#define VCLAMP(x, min, max) (x < min ? min : x > max ? max : x)

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
    VkBuffer       handle;
    VkDeviceMemory memory;
    VkDeviceSize   size;
    rbuf_id        id;
} vbuf;

typedef struct {
    VkRenderPass handle;
    rpass_id     id;
} vpass;

typedef struct {
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
    rdev_wnd               window_api;
    uint32_t               image_index;
    uint32_t               current_frame;
} rdev_vulkan;

//=========================================================
//
// context
//
//=========================================================

VkResult rdev_vulkan_create_instance(rdev_vulkan* rdev);

VkResult rdev_vulkan_create_device(rdev_vulkan* rdev);
void     rdev_vulkan_destroy_device(rdev_vulkan* rdev);

//=========================================================
//
// presentation resources
//
//=========================================================

VkResult rdev_vulkan_create_surface_wl(rdev_vulkan* rdev, void* wnd_native);
VkResult rdev_vulkan_create_surface_xcb(rdev_vulkan* rdev, void* wnd_native);
VkResult rdev_vulkan_create_surface_win32(rdev_vulkan* rdev, void* wnd_native);

VkResult rdev_vulkan_create_swapchain(rdev_vulkan* rdev, vswapchain* sc,
                                      VkExtent2D extent);
void     rdev_vulkan_destroy_swapchain(rdev_vulkan* rdev, vswapchain* sc);

VkResult rdev_vulkan_create_framebuffers(rdev_vulkan* rdev, vswapchain* sc);
void     rdev_vulkan_destroy_framebuffers(rdev_vulkan* rdev, vswapchain* sc);

//=========================================================
//
// rendering resources
//
//=========================================================

VkResult rdev_vulkan_create_swapchainpass(rdev_vulkan* rdev, vswapchain* sc);
void     rdev_vulkan_destroy_swapchainpass(rdev_vulkan* rdev, vswapchain* sc);

VkResult rdev_vulkan_create_semaphores(rdev_vulkan* rdev, uint32_t count);
void     rdev_vulkan_destroy_semaphores(rdev_vulkan* rdev, uint32_t count);

VkResult rdev_vulkan_create_fences(rdev_vulkan* rdev, uint32_t count);
void     rdev_vulkan_destroy_fences(rdev_vulkan* rdev, uint32_t count);

VkResult rdev_vulkan_create_pipeline(rdev_vulkan* rdev, vpipe* pipe);
void     rdev_vulkan_destroy_pipeline(rdev_vulkan* rdev, vpipe* pipe);

VkResult rdev_vulkan_create_fences(rdev_vulkan* rdev, uint32_t count);
void     rdev_vulkan_destroy_fences(rdev_vulkan* rdev, uint32_t count);

//=========================================================
//
// graphics resources
//
//=========================================================

#ifdef _DEBUG
VkResult rdev_vulkan_create_dbg_msgr(rdev_vulkan*              rdev,
                                     VkDebugUtilsMessengerEXT* m);
void rdev_vulkan_destroy_dbg_msgr(rdev_vulkan* rdev, VkDebugUtilsMessengerEXT m);
#endif  // _DEBUG
