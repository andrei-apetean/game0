#pragma once 

#include <vulkan/vulkan.h>

void create_vksurface_wl(VkInstance instance, VkAllocationCallbacks* alloc,
                              VkSurfaceKHR* surface, void* window_handle);

void create_vksurface_xcb(VkInstance instance, VkAllocationCallbacks* alloc,
                              VkSurfaceKHR* surface, void* window_handle);

void create_vksurface_win32(VkInstance instance, VkAllocationCallbacks* alloc,
                              VkSurfaceKHR* surface, void* window_handle);
