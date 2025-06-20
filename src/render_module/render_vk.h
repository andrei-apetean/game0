#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    VkDevice         handle;
    VkPhysicalDevice gpu;
    VkQueue          gfx_queue;
    uint32_t         gfx_queue_family;
} device_vk;

typedef struct {
    VkImage        color_img;
    VkImageView    color_img_view;
    VkImage        depth_img;
    VkImageView    depth_img_view;
    VkDeviceMemory depth_memory;
    VkFramebuffer  framebuffer;
    VkSemaphore    image_available;
    VkSemaphore    render_finished;
    VkFence        inflight;
} swapchain_frame;

typedef struct {
    VkSwapchainKHR     handle;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR   present_mode;
    VkFormat           depth_format;
    uint32_t           frame_count;
    swapchain_frame*   frames;
} swapchain_vk;

typedef struct {
    int32_t        id;
    VkBuffer       handle;
    VkDeviceMemory memory;
    VkDeviceSize   allocation_size;
} buffer_vk;

typedef struct {
    VkInstance             instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR           surface;
    VkRenderPass           renderpass;
    VkCommandPool          cmdpool;
    VkCommandBuffer*       cmdbuffers;
    VkPipeline             pipeline;
    VkPipelineLayout       pipeline_layout;
    device_vk              device;
    swapchain_vk           swapchain;
    uint32_t               current_frame;
    uint32_t               width;
    uint32_t               height;
    uint32_t               image_index;
    buffer_vk              buffers[32];  // todo
} render_vk;

typedef struct {
    VkInstance       instance;
    VkSurfaceKHR     surface;
    VkPhysicalDevice device;
    int32_t          queue_family_idx;
} device_selection_params;

typedef struct {
    VkInstance             instance;
    VkAllocationCallbacks* allocator;
    VkPhysicalDevice       phys_device;
    VkDevice*              device;
    VkQueue*               queue;
    uint32_t               queue_family_idx;
} device_creation_params;

typedef struct {
    VkAllocationCallbacks*        allocator;
    VkSurfaceCapabilitiesKHR      capabilities;
    VkExtent2D                    extent;
    VkSurfaceTransformFlagBitsKHR pretransform;
    swapchain_vk*                 swapchain;
    device_vk*                    device;
    VkSurfaceKHR                  surface;
    VkRenderPass                  renderpass;
    uint32_t                      width;
    uint32_t                      height;
} swapchain_creation_params;

void init_instance_vk(VkInstance* inst, VkAllocationCallbacks* allocator,
                      uint32_t window_api);

void    init_physical_device_vk(device_selection_params* params);
void    init_logical_device_vk(device_creation_params* params);
int32_t find_memory_type(VkPhysicalDevice dev, uint32_t memory_type_bits,
                         uint32_t memory_type);
void    create_buffer_vk(render_vk state, VkMemoryPropertyFlags properties,
                         buffer_vk* buffer, uint32_t size,
                         VkBufferUsageFlags usage_flags);
void    upload_to_gpu_vk(render_vk state, buffer_vk* buffer, void* data,
                         uint32_t size);

void init_renderpass_vk(VkDevice device, VkAllocationCallbacks* allocator,
                        VkRenderPass* renderpass, VkFormat format,
                        VkFormat depth_format);

void init_swapchain_vk(swapchain_creation_params* params);
void destroy_swapchain_vk(VkDevice dev, VkAllocationCallbacks* alloc,
                          swapchain_vk swapchain, VkCommandPool pool);
// todo recreate_swapchain

void create_graphics_pipeline(VkDevice device, VkRenderPass renderpass,
                              VkAllocationCallbacks* allocator,
                              VkPipelineLayout       pipeline_layout,
                              VkShaderModule         vert_shader_module,
                              VkShaderModule         frag_shader_module,
                              VkPipeline* pipeline, uint32_t width,
                              uint32_t height);

VkShaderModule     load_shader_module(VkDevice device, const char* filename,
                                      VkAllocationCallbacks* allocator);
VkSurfaceFormatKHR choose_surface_fmt(VkPhysicalDevice dev,
                                      VkSurfaceKHR     surface);

VkFormat         choose_depth_format(VkPhysicalDevice dev);
VkPresentModeKHR choose_present_mode(VkPhysicalDevice dev,
                                     VkSurfaceKHR     surface);

#if _DEBUG
void init_debug_msgr(VkInstance instance, VkAllocationCallbacks* allocator,
                     VkDebugUtilsMessengerEXT* msgr);
void destroy_debug_msgr(VkInstance instance, VkAllocationCallbacks* allocator,
                        VkDebugUtilsMessengerEXT msgr);
#endif  // _DEBUG
