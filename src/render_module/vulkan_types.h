#pragma once

#include <assert.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(x)                \
    {                              \
        assert((x) == VK_SUCCESS); \
    }

#define CLAMP_VK(x, min, max) (x < min ? min : x > max ? max : x)
#define MAX_FRAME_COUNT 3
typedef struct {
    VkSurfaceKHR     surface;
    VkShaderModule   vertex_shader;
    VkShaderModule   fragment_shader;
    VkPipelineLayout layout;

    VkFormat color_format;
    VkFormat depth_format;

    uint32_t enable_depth;
    uint32_t enable_blending;
} vulkan_pipeline_config;

typedef struct {
    VkPipeline       handle;
    VkPipelineLayout layout;
    VkRenderPass     renderpass;
} vulkan_pipeline;

typedef struct {
    VkDevice         logical_device;
    VkPhysicalDevice physical_device;

    VkPhysicalDeviceProperties props;

    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;

    VkCommandPool cmdpool;

    int32_t graphics_queue_idx;
    int32_t compute_queue_idx;
    int32_t transfer_queue_idx;

    VkAllocationCallbacks* allocator;
} vulkan_device;

typedef struct {
    VkSwapchainKHR  handle;
    VkDeviceMemory  depth_memory[MAX_FRAME_COUNT];
    VkImage         color_images[MAX_FRAME_COUNT];
    VkImage         depth_images[MAX_FRAME_COUNT];
    VkImageView     color_image_views[MAX_FRAME_COUNT];
    VkImageView     depth_image_views[MAX_FRAME_COUNT];
    uint32_t        frame_count;
    uint32_t        width;
    uint32_t        height;
} vulkan_swapchain;

typedef struct {
    VkInstance       instance;
    VkSurfaceKHR     surface;
    vulkan_device    device;
    vulkan_swapchain swapchain;

    VkFramebuffer   framebuffers[MAX_FRAME_COUNT];
    VkSemaphore     image_available_semaphores[MAX_FRAME_COUNT];
    VkSemaphore     render_finished_semaphores[MAX_FRAME_COUNT];
    VkFence         inflight_fences[MAX_FRAME_COUNT];
    VkCommandBuffer cmdbuffers[MAX_FRAME_COUNT];
    vulkan_pipeline pipeline;

    uint32_t current_frame;
    uint32_t image_index;
    VkAllocationCallbacks* allocator;
} vulkan_core;
