#pragma once
#include "render_module/vulkan/render_types_vk.h"

void context_initialize_vk(vk_context* ctx);
void context_terminate_vk(vk_context* ctx);

void create_surface_vk(vk_context* ctx);
void create_device_vk(vk_context* ctx);
void create_swapchain_vk(vk_context* ctx);
