#pragma once

#include <functional>

#if defined(IMPLUS_RENDER_DX11)
#include <d3d11.h>
#elif defined(IMPLUS_RENDER_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace ImPlus::Render {

#if defined(IMPLUS_RENDER_DX11)
struct DeviceInfo {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
};
struct FrameInfo {};
#elif defined(IMPLUS_RENDER_VULKAN)
struct DeviceInfo {
    VkAllocationCallbacks const* allocator;
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkQueue graphics_queue;
    VkDescriptorPool descriptor_pool;
};
struct FrameInfo {
    VkCommandPool command_pool;
};
#else
struct DeviceInfo {};
struct FrameInfo {};
#endif

auto GetDeviceInfo() -> Render::DeviceInfo;
auto GetFrameInfo() -> Render::FrameInfo;

inline std::function<void(DeviceInfo const& info)> OnDeviceChange;

// hints

enum U32Hint {
    Vulkan_CombinedImageSamplerCount,
    Vulkan_DescriptorPoolMaxSets,
};

void SetHint(U32Hint h, uint32_t value);
auto GetHint(U32Hint h) -> std::optional<uint32_t>;

} // namespace ImPlus::Render