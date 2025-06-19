#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "memory.h"
#include "render_module/render_types.h"
#include "render_module/vulkan/render_surface_vk.h"
#include "render_module/vulkan/render_types_vk.h"

#define WL_SURFACE_EXTENSION_NAME "VK_KHR_wayland_surface"
#define WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
#define XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"

static VkBool32 debug_utils_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data) {
    printf(" MessageID: %s %i\nMessage: %s\n\n", callback_data->pMessageIdName,
           callback_data->messageIdNumber, callback_data->pMessage);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        // ASSERT(0); // todo
    }

    return VK_FALSE;
}

static int32_t create_instance(render_device_vk* device, uint32_t window_api);
static int32_t pick_gpu(render_device_vk* device);
static int32_t make_device(render_device_vk* device);
static int32_t get_family_queue(render_device_vk* state, VkPhysicalDevice dev);

int32_t get_render_device_state_size_vk() { return sizeof(render_device_vk); }

int32_t connect_device_vk(render_device* device, render_device_params* params) {
    render_device_vk* dev = (render_device_vk*)device;
    assert(dev);
    memset(dev, 0, sizeof(render_device_vk));

    create_instance(dev, params->window_api);
    switch (params->window_api) {
        case 0:
            create_vksurface_wl(dev->instance, dev->allocator, &dev->surface,
                                params->window_handle);
            break;
        default:
            assert(0 && "Unimplemented surface");
    }

    if (pick_gpu(dev) != 0)  return -1;
        if (make_device(dev) != 0) return -1;
    return 0;
}

void disconnect_device_vk(render_device* device) {
    render_device_vk* dev = (render_device_vk*)device;
    assert(dev);
    vkDestroySurfaceKHR(dev->instance, dev->surface, dev->allocator);
#ifdef _DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_msgr =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            dev->instance, "vkDestroyDebugUtilsMessengerEXT");
    destroy_msgr(dev->instance, dev->dbg_msgr, dev->allocator);
#endif
    vkDestroyInstance(dev->instance, dev->allocator);
}

static int32_t create_instance(render_device_vk* device, uint32_t window_api) {
    {
        const VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "game0",
            .pEngineName = "game0",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_MAKE_VERSION(1, 3, 0),
        };

#ifdef _DEBUG
        const uint32_t ext_count = 4;
#else
        const uint32_t ext_count = 2;
#endif  //_DEBUG
        const char* requested_extensions[ext_count];
        requested_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        // todo, window api
        switch (window_api) {
            case 0: {
                requested_extensions[1] = WL_SURFACE_EXTENSION_NAME;
                break;
            }
            case 1: {
                requested_extensions[1] = XCB_SURFACE_EXTENSION_NAME;
                break;
            }
            case 2: {
                requested_extensions[1] = WIN32_SURFACE_EXTENSION_NAME;
                break;
            }
            default:
                assert(0 && "Unimplemented surface extension");
        }

#ifdef _DEBUG
        requested_extensions[ext_count - 2] =
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        requested_extensions[ext_count - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        for (size_t i = 0; i < ext_count; i++) {
            printf("Requested ext: %s\n", requested_extensions[i]);
        }
        const char* requested_layers[] = {
            "VK_LAYER_KHRONOS_validation",
        };
#else
        const char* requested_layers[requested_layer_count] = {
            "",
        };
#endif  // _DEBUG
        const uint32_t requested_layer_count =
            sizeof(requested_layers) / sizeof(requested_layers[0]);

        const VkInstanceCreateInfo inst_ci = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
            .enabledExtensionCount = ext_count,
            .ppEnabledExtensionNames = requested_extensions,
            .enabledLayerCount = requested_layer_count,
            .ppEnabledLayerNames = requested_layers,
        };
        VkResult result =
            vkCreateInstance(&inst_ci, device->allocator, &device->instance);
        check(result, "vkCreateInstance", __FILE__, __LINE__);
    }
// debug messenger
#if _DEBUG
    {
        uint32_t ext_count = 0;
        int32_t  debug_extension_present = 0;
        vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
        VkExtensionProperties* extensions =
            temp_alloc(ext_count * sizeof(VkExtensionProperties));
        vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions);
        const char* dbg_ext_name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        for (size_t i = 0; i < ext_count; i++) {
            if (!strcmp(extensions[i].extensionName, dbg_ext_name)) {
                debug_extension_present = 1;
                printf("Debug extension present!\n");
                break;
            }
        }
        temp_free();
        if (debug_extension_present) {
            const VkDebugUtilsMessengerCreateInfoEXT dbg_ci = {
                .sType =
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                .pfnUserCallback = debug_utils_callback,
            };
            PFN_vkCreateDebugUtilsMessengerEXT create_debug_msgr =
                (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                    device->instance, "vkCreateDebugUtilsMessengerEXT");
            if (create_debug_msgr != NULL) {
                VkResult result =
                    create_debug_msgr(device->instance, &dbg_ci,
                                      device->allocator, &device->dbg_msgr);
                check(result, "create_debug_msgr", __FILE__, __LINE__);
            }

        } else {
            printf("Debug utils messenger extension not present!\n");
        }
    }
    return 0;
#endif
}

int32_t pick_gpu(render_device_vk* device) {
    VkResult result;
    uint32_t dev_count = 0;
    result = vkEnumeratePhysicalDevices(device->instance, &dev_count, NULL);
    check(result, "VkEnumeratePhysicalDevices", __FILE__, __LINE__);
    VkPhysicalDevice* devs = temp_alloc(dev_count * sizeof(VkPhysicalDevice));
    result = vkEnumeratePhysicalDevices(device->instance, &dev_count, devs);
    check(result, "VkEnumeratePhysicalDevices", __FILE__, __LINE__);

    VkPhysicalDevice discrete_gpu = VK_NULL_HANDLE;
    VkPhysicalDevice integrated_gpu = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties dev_props;
    for (size_t i = 0; i < dev_count; i++) {
        vkGetPhysicalDeviceProperties(devs[i], &dev_props);
        if (dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            if (get_family_queue(device, devs[i])) {
                discrete_gpu = devs[i];
                break;
            }
        }
        if (dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            if (get_family_queue(device, devs[i])) {
                integrated_gpu = devs[i];
                break;
            }
        }
    }

    temp_free();
    if (discrete_gpu != VK_NULL_HANDLE) {
        device->gpu = discrete_gpu;
    } else if (integrated_gpu != VK_NULL_HANDLE) {
        device->gpu = integrated_gpu;
    } else {
        return -1;
    }
    printf("Selected GPU: %s\n", dev_props.deviceName);
    return 0;
}

static int32_t make_device(render_device_vk* device) {
    const char* dev_exts[] = {
        "VK_KHR_swapchain",
    };
    const float             queue_priority[] = {1.0f};
    VkDeviceQueueCreateInfo queue_ci[1] = {0};
    queue_ci[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci[0].queueFamilyIndex = device->queue_family_idx;
    queue_ci[0].queueCount = 1;
    queue_ci[0].pQueuePriorities = queue_priority;

    VkPhysicalDeviceFeatures2 features = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };

    vkGetPhysicalDeviceFeatures2(device->gpu, &features);

    VkDeviceCreateInfo dev_ci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = sizeof(queue_ci) / sizeof(*queue_ci),
        .pQueueCreateInfos = queue_ci,
        .enabledExtensionCount = sizeof(dev_exts) / sizeof(*dev_exts),
        .ppEnabledExtensionNames = dev_exts,
        .pNext = &features,
    };
    VkResult result = vkCreateDevice(device->gpu, &dev_ci, device->allocator,
                                     &device->handle);
    check(result, "vkCreateDevice", __FILE__, __LINE__);

    vkGetDeviceQueue(device->handle, device->queue_family_idx, 0,
                     &device->queue);
    return 0;
}

static int32_t get_family_queue(render_device_vk* device,
                                VkPhysicalDevice  dev) {
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, NULL);

    VkQueueFamilyProperties* properties =
        temp_alloc(sizeof(VkQueueFamilyProperties) * family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, properties);
    VkBool32 supported;
    for (size_t idx = 0; idx < family_count; idx++) {
        VkQueueFamilyProperties queue_family = properties[idx];
        if (queue_family.queueCount > 0 &&
            queue_family.queueFlags &
                (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, idx, device->surface,
                                                 &supported);

            if (supported) {
                device->queue_family_idx = idx;
                break;
            }
        }
    }
    temp_free();

    return supported;
}

void check(VkResult result, char* msg, char* file, int32_t line) {
    if (result == VK_SUCCESS) {
        printf("Success %s; %s:%d\n", msg, file, line);
        return;
    }
    printf("Failure %s; %s:%d\n", msg, file, line);
    assert(0);
}
