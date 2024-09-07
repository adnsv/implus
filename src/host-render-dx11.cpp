#include "host-render.hpp"

#include <backends/imgui_impl_dx11.h>
#include <implus/render-device.hpp>
#include <stdexcept>

namespace ImPlus::Render {

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ImPlus::Host::Window::Size g_FramebufferSize = {0, 0};
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

auto GetDeviceInfo() -> Render::DeviceInfo
{
    return Render::DeviceInfo{
        .device = g_pd3dDevice,
        .context = g_pd3dDeviceContext,
    };
}

auto GetFrameInfo() -> Render::FrameInfo { return {}; }

void SetHint(ImPlus::Render::U32Hint h, uint32_t v) {}

static void createRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

static void cleanupRenderTarget()
{
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

void SetupWindowHints() {}

void SetupInstance(ImPlus::Host::Window& wnd)
{
    auto hwnd = static_cast<HWND>(wnd.Handle());

    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
            &featureLevel, &g_pd3dDeviceContext) != S_OK)
        throw std::runtime_error("Failed to create device and swapchain.");

    createRenderTarget();
}

void ShutdownInstance()
{
    cleanupRenderTarget();

    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = NULL;
    }
    if (Render::OnDeviceChange && (g_pd3dDeviceContext || g_pd3dDevice))
        Render::OnDeviceChange({nullptr, nullptr});

    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = NULL;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
}

void SetupWindow(ImPlus::Host::Window&) {}

void SetupImplementation(ImPlus::Host::Window&)
{
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
}

void ShutdownImplementation() { ImGui_ImplDX11_Shutdown(); }

void NewFrame(ImPlus::Host::Window& wnd)
{
    auto sz = wnd.FramebufferSize();
    if (sz.w != g_FramebufferSize.w || sz.h != g_FramebufferSize.h) {
        g_FramebufferSize = sz;
        cleanupRenderTarget();
        g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        // g_pSwapChain->ResizeBuffers(
        //     0, (UINT)LOWORD(sz.w), (UINT)HIWORD(sz.h), DXGI_FORMAT_UNKNOWN, 0);
        createRenderTarget();
    }

    ImGui_ImplDX11_NewFrame();
}

void PrepareViewport(ImPlus::Host::Window& wnd)
{
    auto& clr = wnd.Background;
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clr);
}

void RenderDrawData() { ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); }

void SwapBuffers(ImPlus::Host::Window&)
{
    assert(g_pSwapChain);
    g_pSwapChain->Present(1, 0); // Present with vsync
}

} // namespace ImPlus::Render