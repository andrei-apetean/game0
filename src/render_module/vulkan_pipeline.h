#pragma once
#include "render_module/vulkan_types.h"

int32_t vulkan_pipeline_create(vulkan_device* device, vulkan_pipeline* pipeline,
                               vulkan_pipeline_config* config);
void    vulkan_pipeline_destroy(vulkan_device* device, vulkan_pipeline* pipeline);
