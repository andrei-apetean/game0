#pragma once
#include <vulkan/vulkan.h>
#include "render/rtypes.h"

#define VSWAPCHAIN_MAX_IMG 3
#define VBUF_MAX_COUNT 128
#define VPASS_MAX_COUNT 16
#define VPIPE_MAX_COUNT 16

#define VCHECK(x) debug_assert((x) == VK_SUCCESS);
#define VCLAMP(x, min, max)  (x < min ? min : x > max ? max : x)

struct rcmd {
    VkCommandBuffer handle;
};

typedef struct {
    VkBuffer       handle;
    VkDeviceMemory memory;
    VkDeviceSize   size;
    uint32_t       id;
} vbuf;

typedef struct {
    VkRenderPass handle;
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
    VkExtent2D         extent;
    VkPresentModeKHR   present_mode;
    VkSurfaceFormatKHR surface_fmt;
    vpass              rpass;
    uint32_t           image_count;
} vswapchain;

typedef struct {
    VkAllocationCallbacks* allocator;
    VkInstance             instance;
    VkSurfaceKHR           surface;
    vswapchain             swapchain;
    vdev                   dev;
    vbuf                   buffers[VBUF_MAX_COUNT];
    VkCommandBuffer        cmd_buffers[VSWAPCHAIN_MAX_IMG];
    rdev_wnd               window_api;
} rdev_vulkan;

VkResult rdev_vulkan_create_instance(rdev_vulkan* rdev);

VkResult rdev_vulkan_create_device(rdev_vulkan* rdev);
void     rdev_vulkan_destroy_device(rdev_vulkan* rdev);

VkResult rdev_vulkan_create_surface_wl(rdev_vulkan* rdev, void* wnd_native);
VkResult rdev_vulkan_create_surface_xcb(rdev_vulkan* rdev, void* wnd_native);
VkResult rdev_vulkan_create_surface_win32(rdev_vulkan* rdev, void* wnd_native);

VkResult rdev_vulkan_create_swapchain(rdev_vulkan* rdev, vswapchain* sc,
                                      VkExtent2D extent);
void     rdev_vulkan_destroy_swapchain(rdev_vulkan* rdev, vswapchain* sc);

VkResult rdev_vulkan_create_semaphores(rdev_vulkan* rdev, uint32_t count);
void     rdev_vulkan_destroy_semaphores(rdev_vulkan* rdev, uint32_t count);

VkResult rdev_vulkan_create_pipeline(rdev_vulkan* rdev, vpipe* pipe);
void     rdev_vulkan_destroy_pipeline(rdev_vulkan* rdev, vpipe* pipe);

VkResult rdev_vulkan_create_framebuffers(rdev_vulkan* rdev, vpass* pass,
                                         uint32_t count, VkExtent2D extent);
void     rdev_vulkan_destroy_framebuffers(rdev_vulkan* rdev, uint32_t count);

VkResult rdev_vulkan_create_fences(rdev_vulkan* rdev, uint32_t count);
void     rdev_vulkan_destroy_fences(rdev_vulkan* rdev, uint32_t count);

#ifdef _DEBUG
VkResult rdev_vulkan_create_dbg_msgr(rdev_vulkan*              rdev,
                                     VkDebugUtilsMessengerEXT* m);
void rdev_vulkan_destroy_dbg_msgr(rdev_vulkan* rdev, VkDebugUtilsMessengerEXT m);
#endif  // _DEBUG
