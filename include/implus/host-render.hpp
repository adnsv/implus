#pragma once

#include <functional>

#ifdef IMPLUS_RENDER_DX11
#include <d3d11.h>
#endif

namespace ImPlus::Host::Render {

#if defined(IMPLUS_RENDER_DX11)
struct DeviceInfo {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
};
#else
struct DeviceInfo {
    int dummy = 0;
};
#endif

inline std::function<void(DeviceInfo const& info)> OnDeviceChange;

} // namespace ImPlus::Host::Render