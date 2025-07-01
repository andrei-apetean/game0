
#include "core/os_defines.h"
#include "base.h"
#include "render/rdev_vulkan.h"
#include "vulkan/vulkan_core.h"

#if defined(OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>

typedef struct {
    HWND      hwnd;
    HINSTANCE instance;
} native_handle_win32;

VkBool32 vutl_present_supported_win32(VkPhysicalDevice d, uint32_t family) {
    debug_log("Checking win32 presentation support: %d\n", family);
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(d, family);
}

VkResult vcreate_surface_win32(vstate* v, void* wnd_native) {
  VkResult result = VK_ERROR_UNKNOWN;
  native_handle_win32* handle = (native_handle_win32*)wnd_native;

  VkWin32SurfaceCreateInfoKHR ci = {
      .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      .hwnd = handle->hwnd,
      .hinstance= handle->instance,
  };
  result = vkCreateWin32SurfaceKHR(v->instance, &ci, v->allocator, &v->surface);
  VCHECK(result);
  return result;
}

#else 

VkBool32 vutl_present_supported_win32(VkPhysicalDevice d, uint32_t family) { 
  unused(d);
  unused(family);
  unimplemented(vutl_present_supported_win32);
  return VK_FALSE;
}

VkResult vcreate_surface_win32(vstate* v, void* wnd_native) {
    unused(v);
    unused(wnd_native);
    unimplemented(vcreate_surface_win32);
    return VK_ERROR_UNKNOWN;
}
#endif // OS_WIN32
