#pragma once

#include <functional>

#if defined(IMPLUS_RENDER_DX11)
#include <d3d11.h>
#elif defined(IMPLUS_RENDER_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace ImPlus::Host::Render {

#if defined(IMPLUS_RENDER_DX11)
struct DeviceInfo {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
};
#elif defined(IMPLUS_RENDER_VULKAN)
struct DeviceInfo {
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkQueue graphics_queue;
    VkCommandPool command_pool;
};
#else 
struct DeviceInfo {
    int dummy = 0;
};
#endif

inline std::function<void(DeviceInfo const& info)> OnDeviceChange;

} // namespace ImPlus::Host::Render