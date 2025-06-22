#include <vulkan/vulkan_core.h>
#include "vulkan_types.h"

int32_t vkcore_create_instance(vulkan_core* core, uint32_t win_api);

int32_t vkcore_create_surface_wl(vulkan_core* core, void* win);
int32_t vkcore_create_surface_xcb(vulkan_core* core, void* win);
int32_t vkcore_create_surface_win32(vulkan_core* core, void* win);
int32_t vkcore_create_semaphores(vulkan_core* core, uint32_t count);
int32_t vkcore_create_fences(vulkan_core* core, uint32_t count);
int32_t vkcore_create_framebuffers(vulkan_core* core, VkRenderPass renderpass, uint32_t count,
                                   uint32_t width, uint32_t height);
void vkcore_destroy_instance(vulkan_core* core);
void vkcore_destroy_surface(vulkan_core* core);
void vkcore_destroy_semaphores(vulkan_core* core, uint32_t count);
void vkcore_destroy_fences(vulkan_core* core, uint32_t count);
void vkcore_destroy_framebuffers(vulkan_core* core, uint32_t count);


#ifdef _DEBUG
int32_t vkcore_create_dbg_msgr(vulkan_core* core, VkDebugUtilsMessengerEXT* m);
void    vkcore_destroy_dbg_msgr(vulkan_core* core, VkDebugUtilsMessengerEXT m);
#endif  // _DEBUG
