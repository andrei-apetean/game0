#pragma once

#include <vulkan/vulkan_core.h>

#include "render_module/vulkan_types.h"

int32_t vulkan_device_create(vulkan_device* device, VkInstance instance,
                             VkSurfaceKHR surface);

void vulkan_device_destroy(vulkan_device* device);

int32_t vulkan_device_find_surface_format(vulkan_device*      device,
                                          VkSurfaceKHR        surface,
                                          VkSurfaceFormatKHR* out_format);
int32_t vulkan_device_find_depth_format(vulkan_device* device,
                                        VkFormat*      out_format);

int32_t vulkan_device_find_present_mode(vulkan_device* device, VkSurfaceKHR surface,
                                        VkPresentModeKHR* out_mode);

int32_t vulkan_device_find_memory_type(vulkan_device* device,
                                       uint32_t       memory_type_bits,
                                       uint32_t       memory_type);

int32_t vulkan_device_create_buffer(vulkan_device* device, vulkan_buffer* buffer,
                                    buffer_config* config);

int32_t vulkan_device_upload_buffer(vulkan_device* device,
                                    vulkan_buffer* dst_buffer, void* data,
                                    uint32_t size);

void vulkan_device_destroy_buffer(vulkan_device* device, vulkan_buffer* buffer);

int32_t vulkan_device_create_shader_module(vulkan_device* device, uint32_t* code,
                                           size_t          code_size,
                                           VkShaderModule* module);

void vulkan_device_destroy_shader_module(vulkan_device* device,
                                         VkShaderModule module);

