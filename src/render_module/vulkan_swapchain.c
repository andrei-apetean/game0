#include "render_module/vulkan_swapchain.h"

#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "render_module/vulkan_device.h"

int32_t vulkan_swapchain_create(vulkan_device* device, vulkan_swapchain* swapchain,
                                VkSurfaceKHR surface, uint32_t width,
                                uint32_t height) {
    swapchain->width = width;
    swapchain->height = height;
    VkSurfaceFormatKHR surface_fmt;
    VkFormat           depth_fmt;
    VkPresentModeKHR   present_mode;
    int32_t            result =
        vulkan_device_find_surface_format(device, surface, &surface_fmt);
    if (result) return result;

    result = vulkan_device_find_depth_format(device, &depth_fmt);
    if (result) return result;

    result = vulkan_device_find_present_mode(device, surface, &present_mode);
    if (result) return result;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, surface,
                                              &caps);
    uint32_t frame_count = caps.minImageCount + 1;
    if (frame_count > caps.maxImageCount) frame_count = caps.maxImageCount;
    swapchain->frame_count = frame_count;
    VkExtent2D size = (VkExtent2D){width, height};
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX || extent.height == UINT32_MAX) {
        extent.width = width;
        extent.height = height;
        extent.width = CLAMP_VK(size.width, caps.minImageExtent.width,
                                caps.maxImageExtent.width);
        extent.height = CLAMP_VK(size.height, caps.minImageExtent.height,
                                 caps.maxImageExtent.height);
    }

    VkSurfaceTransformFlagBitsKHR pretransform =
        (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : caps.currentTransform;
    VkSwapchainCreateInfoKHR sc_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = frame_count,
        .imageFormat = surface_fmt.format,
        .imageColorSpace = surface_fmt.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = pretransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
    };
    VK_CHECK(vkCreateSwapchainKHR(device->logical_device, &sc_ci, device->allocator,
                                  &swapchain->handle));
    printf("Swapchain created!\n");

    assert(frame_count <= MAX_FRAME_COUNT);
    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(device->logical_device, swapchain->handle, &img_count,
                            NULL);
    printf("Desired number of frames: %d\n", frame_count);
    printf("Retrieved swapchain img count: %d!\n", img_count);
    assert(img_count <= frame_count);
    vkGetSwapchainImagesKHR(device->logical_device, swapchain->handle, &img_count,
                            swapchain->color_images);

    for (uint32_t i = 0; i < frame_count; i++) {
        VkImageViewCreateInfo color_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain->color_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_fmt.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,

        };
        VK_CHECK(vkCreateImageView(device->logical_device, &color_view_info,
                                   device->allocator,
                                   &swapchain->color_image_views[i]));
        printf("VkImageView color %d created!\n", i);

        // depth attachments

        VkImageCreateInfo depth_img_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = depth_fmt,
            .extent = {width, height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VK_CHECK(vkCreateImage(device->logical_device, &depth_img_info,
                               device->allocator, &swapchain->depth_images[i]));

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(device->logical_device,
                                     swapchain->depth_images[i], &mem_reqs);

        // Allocate
        int32_t memory_type = vulkan_device_find_memory_type(
            device, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        assert(memory_type != -1);
        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = memory_type,
        };

        VK_CHECK(vkAllocateMemory(device->logical_device, &alloc_info,
                                  device->allocator, &swapchain->depth_memory[i]));
        printf("VkImage depth %d memory allocated!\n", i);
        VK_CHECK(vkBindImageMemory(device->logical_device,
                                   swapchain->depth_images[i],
                                   swapchain->depth_memory[i], 0));

        VkImageViewCreateInfo depth_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain->depth_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = depth_fmt,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        VK_CHECK(vkCreateImageView(device->logical_device, &depth_view_info,
                                   device->allocator,
                                   &swapchain->depth_image_views[i]));
        printf("VkImageView depth %d created!\n", i);
    }

    return 0;
}

void vulkan_swapchain_destroy(vulkan_device* device, vulkan_swapchain* swapchain) {
    VkImageView*    color_image_views = swapchain->color_image_views;
    VkImageView*    depth_image_views = swapchain->depth_image_views;
    VkImage*        depth_images = swapchain->depth_images;
    VkDeviceMemory* memories = swapchain->depth_memory;

    for (uint32_t i = 0; i < swapchain->frame_count; i++) {
        vkDestroyImage(device->logical_device, depth_images[i], device->allocator);
        vkDestroyImageView(device->logical_device, color_image_views[i],
                           device->allocator);
        vkDestroyImageView(device->logical_device, depth_image_views[i],
                           device->allocator);
        vkFreeMemory(device->logical_device, memories[i], device->allocator);
    }
    vkDestroySwapchainKHR(device->logical_device, swapchain->handle,
                          device->allocator);
}
