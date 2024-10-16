#include "host-render.hpp"
#include <implus/host.hpp>
#include <implus/render-device.hpp>

#include <imgui_internal.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <tpcshrd.h>
#include <windows.h>

#include <backends/imgui_impl_win32.h>
#include <shellscalingapi.h>

#ifdef IMPLUS_RENDER_DX11
#include <backends/imgui_impl_dx11.h>
//
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif

#include <ShlObj.h>
#include <Shobjidl.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// GUID for ITipInvocation
const IID IID_ITipInvocation = {
    0x37c94e25, 0xd17b, 0x4e84, {0x87, 0xb8, 0x77, 0x79, 0x44, 0x0b, 0x97, 0xb5}};

const CLSID CLSID_UIHostNoLaunch = {
    0x4ce576fa, 0x83dc, 0x4f88, {0x95, 0x1c, 0x9d, 0x07, 0x82, 0x80, 0x0e, 0x7a}};

struct ITipInvocation : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Toggle(HWND wnd) = 0;
};

namespace ImPlus::Host {

extern Window* main_window_;
extern std::size_t window_counter_;

extern auto AllowRefresh() -> bool;

auto to_wstring(std::string_view s) -> std::wstring
{
    if (s.empty())
        return {};
    auto n = ::MultiByteToWideChar(CP_THREAD_ACP, 0, s.data(), int(s.size()), nullptr, 0);
    if (!n)
        return {};
    auto ret = std::wstring{};
    ret.resize(n);
    ::MultiByteToWideChar(CP_THREAD_ACP, 0, s.data(), int(s.size()), ret.data(), n);
    return ret;
}

auto to_string(std::wstring_view s) -> std::string
{
    if (s.empty())
        return {};
    auto n = ::WideCharToMultiByte(
        CP_THREAD_ACP, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    if (!n)
        return {};
    auto ret = std::string{};
    ret.resize(n);
    ::WideCharToMultiByte(
        CP_THREAD_ACP, 0, s.data(), int(s.size()), ret.data(), n, nullptr, nullptr);
    return ret;
}

inline auto native_wnd(void* h) -> HWND { return static_cast<HWND>(h); }

static auto const wnd_class_name = L"ImPlus Host Window";
static auto wnd_class_inited = false;
static auto inst = ::GetModuleHandleW(NULL);
static auto all_should_close = false;

void notifyMove(Window& w, Window::Pos const& xy)
{
    if (!w.suspendNotifications)
        w.handleMove(xy);
}
void notifyResize(Window& w, Window::Size const& wh)
{
    if (!w.suspendNotifications)
        w.handleResize(wh);
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {

    case WM_SIZE: {
        int width = short(LOWORD(lParam));
        int height = short(HIWORD(lParam));
        auto w = reinterpret_cast<Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (w) {
            if (w->OnFramebufferSize)
                w->OnFramebufferSize({width, height});
            if (w->OnRefresh && AllowRefresh())
                w->OnRefresh();
            notifyResize(*w, {width, height});
        }
    }
        return 0;

    case WM_MOVE:
        if (auto w = reinterpret_cast<Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA))) {
            int x = short(LOWORD(lParam));
            int y = short(HIWORD(lParam));
            notifyMove(*w, {x, y});
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;

    case WM_ERASEBKGND: return TRUE;

    case WM_PAINT: {
        auto w = reinterpret_cast<Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (w && w->OnRefresh && AllowRefresh())
            w->OnRefresh();
    } break;

    case WM_CLOSE: {
        // change should close instead of default DestroyWindow
        auto w = reinterpret_cast<Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (w)
            w->SetShouldClose(true);
        return 0;
    }

    case WM_DESTROY: ::PostQuitMessage(0); return 0;

    case WM_TABLET_QUERYSYSTEMGESTURESTATUS:
        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb969148(v=vs.85).aspx
        // return TABLET_DISABLE_PRESSANDHOLD;
        return TABLET_DISABLE_PRESSANDHOLD | TABLET_DISABLE_PENTAPFEEDBACK |
               TABLET_DISABLE_PENBARRELFEEDBACK | TABLET_DISABLE_TOUCHUIFORCEON |
               TABLET_DISABLE_TOUCHUIFORCEOFF | TABLET_DISABLE_TOUCHSWITCH | TABLET_DISABLE_FLICKS |
               TABLET_DISABLE_SMOOTHSCROLLING | TABLET_DISABLE_FLICKFALLBACKKEYS;

    case WM_DROPFILES: {
        auto w = reinterpret_cast<Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (!w || !w->OnDropFiles)
            return 0;

        auto drop = (HDROP)wParam;
        const int count = ::DragQueryFileW(drop, 0xffffffff, nullptr, 0);
        auto entries = std::vector<std::string>{};
        static constexpr size_t buflen = 1024;
        wchar_t wbuf[buflen];
        for (int i = 0; i < count; ++i)
            if (auto n = ::DragQueryFileW(drop, i, wbuf, buflen))
                entries.push_back(to_string(wbuf));

        auto ptrs = std::vector<char const*>{};
        ptrs.reserve(entries.size());
        for (auto& s : entries)
            ptrs.push_back(s.c_str());

        w->OnDropFiles(int(ptrs.size()), ptrs.data());

        return 0;
    }
    }

    if (::IsWindowUnicode(hWnd))
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    else
        return ::DefWindowProcA(hWnd, msg, wParam, lParam);
}

auto attrib_style_flags(WindowAttrib attr) -> unsigned
{
    auto ret = unsigned{WS_OVERLAPPED};
    auto resizable = contains(attr, WindowAttrib::Resizable);
    auto decorated = contains(attr, WindowAttrib::Decorated);
    if (resizable)
        ret |= WS_THICKFRAME;

    if (decorated) {
        ret |= WS_CAPTION | WS_SYSMENU;
        ret |= resizable ? WS_MINIMIZEBOX | WS_MAXIMIZEBOX : WS_MINIMIZEBOX;
    }

    return ret;
}

namespace osk {

static bool LaunchTabTip()
{
    DWORD pid = 0;
    auto su = STARTUPINFOW{sizeof(STARTUPINFOW)};
    su.dwFlags = STARTF_USESHOWWINDOW;
    su.wShowWindow = SW_HIDE;
    auto stat = PROCESS_INFORMATION{};
    const wchar_t* path = L"%CommonProgramW6432%\\microsoft shared\\ink\\TabTIP.EXE";
    wchar_t buf[512];
    ::SetEnvironmentVariableW(L"__compat_layer", L"RunAsInvoker");
    ::ExpandEnvironmentStringsW(path, buf, 512);

    if (::CreateProcessW(0, buf, 0, 0, 1, 0, 0, 0, &su, &stat)) {
        pid = stat.dwProcessId;
    }
    ::CloseHandle(stat.hProcess);
    ::CloseHandle(stat.hThread);
    return pid;
}

static bool GetKeyboardRect(RECT* r)
{
    IFrameworkInputPane* inputPane = NULL;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_FrameworkInputPane, NULL, CLSCTX_INPROC_SERVER,
            IID_IFrameworkInputPane, (LPVOID*)&inputPane);
        if (SUCCEEDED(hr)) {
            hr = inputPane->Location(r);
            if (!SUCCEEDED(hr)) {}
            inputPane->Release();
        }
    }
    CoUninitialize();
    return SUCCEEDED(hr);
}

static bool IsKeyboardVisible()
{
    RECT r;
    bool rect_ok = GetKeyboardRect(&r);
    return rect_ok && (r.right > r.left) && (r.bottom > r.top);
}

static void ToggleKeyboardVisibility()
{
    ITipInvocation* pTipInvocation = NULL;
    HRESULT hr;

    // Initialize COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        printf("Failed to initialize COM\n");
        return;
    }

    // Create an instance of the ITipInvocation interface
    hr = CoCreateInstance(CLSID_UIHostNoLaunch, 0, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_ITipInvocation, (void**)&pTipInvocation);

    if (hr == REGDB_E_CLASSNOTREG) {
        if (LaunchTabTip()) {
            hr = CoCreateInstance(CLSID_UIHostNoLaunch, 0,
                CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_ITipInvocation,
                (void**)&pTipInvocation);
        }
        else {
            printf("Failed to launch TabTip service\n");
        }
    }

    if (SUCCEEDED(hr)) {
        // Show the on-screen keyboard
        hr = pTipInvocation->Toggle(GetDesktopWindow());
        if (FAILED(hr)) {
            printf("Failed to show on-screen keyboard\n");
        }
        // Release the interface
        pTipInvocation->Release();
    }
    else {
        printf("Failed to create ITipInvocation instance\n");
    }

    // Uninitialize COM
    CoUninitialize();
}

} // namespace osk

static void Win32_PlatformSetImeData(
    ImGuiContext*, ImGuiViewport* viewport, ImGuiPlatformImeData* data)
{
    // Notify OS Input Method Editor of text input position
    HWND hwnd = (HWND)viewport->PlatformHandleRaw;
    if (hwnd == 0)
        return;

    //::ImmAssociateContextEx(hwnd, NULL, data->WantVisible ? IACE_DEFAULT : 0);
    if (HIMC himc = ::ImmGetContext(hwnd)) {
        COMPOSITIONFORM composition_form = {};
        composition_form.ptCurrentPos.x = (LONG)data->InputPos.x;
        composition_form.ptCurrentPos.y = (LONG)data->InputPos.y;
        composition_form.dwStyle = CFS_FORCE_POSITION;
        ::ImmSetCompositionWindow(himc, &composition_form);
        CANDIDATEFORM candidate_form = {};
        candidate_form.dwStyle = CFS_CANDIDATEPOS;
        candidate_form.ptCurrentPos.x = (LONG)data->InputPos.x;
        candidate_form.ptCurrentPos.y = (LONG)data->InputPos.y;
        ::ImmSetCandidateWindow(himc, &candidate_form);
        ::ImmReleaseContext(hwnd, himc);
    }

    //   if (osk::IsKeyboardVisible() != data->WantVisible)
    //       osk::ToggleKeyboardVisibility();
}

Window::Window(InitLocation const& loc, char const* title, Attrib attr)
    : regular_attr_{attr}
{
    ++window_counter_;

    if (!wnd_class_inited) {
        ImGui_ImplWin32_EnableDpiAwareness();
        wnd_class_inited = true;
        auto m = ::GetModuleHandleW(nullptr);
        auto wc = WNDCLASSEXW{
            .cbSize = sizeof(WNDCLASSEXW),
            .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc = WndProc,
            .cbClsExtra = 0L,
            .cbWndExtra = 0L,
            .hInstance = m,
            .hIcon = nullptr,
            .hCursor = ::LoadCursorW(NULL, IDC_ARROW),
            .hbrBackground = nullptr,
            .lpszClassName = wnd_class_name,
        };

        wc.hIcon =
            (HICON)::LoadImageW(m, L"GLFW_ICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
        if (!wc.hIcon)
            wc.hIcon = (HICON)::LoadImageW(
                nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

        ::RegisterClassExW(&wc);

        struct atexit {
            ~atexit() { Cleanup(); }
        };

        static auto _ = atexit{};
    }

    auto wtitle = to_wstring(title);

    auto wstyle = attrib_style_flags(regular_attr_);

    auto b = PrimaryMonitorWorkArea();
    b.inflate(-b.size.w / 10, -b.size.h / 10);
    if (auto size = std::get_if<Size>(&loc)) {
        if (size->w > 0)
            b.size.w = size->w;
        if (size->h > 0)
            b.size.h = size->h;
    }
    else if (auto bb = std::get_if<Bounds>(&loc)) {
        b = *bb;
    }

    b = ConstrainToMonitor(b);

    auto hwnd = ::CreateWindowExW(0l, wnd_class_name, wtitle.c_str(), wstyle, b.pos.x, b.pos.y,
        b.size.w, b.size.h, nullptr, nullptr, inst, nullptr);

    this->handle_ = hwnd;

    ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, LONG_PTR(this));

    if (window_counter_ == 1) {
        Render::SetupInstance(*this);
        if (Render::OnDeviceChange)
            Render::OnDeviceChange(Render::GetDeviceInfo());
    }

    context_ = ImGui::CreateContext();

    if (window_counter_ == 1) {
        IMGUI_CHECKVERSION();
        ImGui::StyleColorsDark();

        main_window_ = this;

        ImGui_ImplWin32_Init(hwnd);

        auto& platform_io = ImGui::GetPlatformIO();
        platform_io.Platform_SetImeDataFn = Win32_PlatformSetImeData;

        Render::SetupImplementation(*this);
    }
}

Window::~Window()
{
    --window_counter_;

    if (this == main_window_)
        main_window_ = nullptr;

    auto hwnd = native_wnd(this->handle_);
    if (window_counter_ == 0) {
        if (Render::OnDeviceChange)
            Render::OnDeviceChange({});
        Render::ShutdownImplementation();
        ImGui_ImplWin32_Shutdown();
    }
    if (context_)
        ImGui::DestroyContext(context_);
    if (window_counter_ == 0)
        Render::ShutdownInstance();
    if (hwnd) {
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        ::DestroyWindow(hwnd);
    }
}

void Window::Cleanup()
{
    if (wnd_class_inited) {
        wnd_class_inited = false;
        ::UnregisterClassW(wnd_class_name, inst);
    }
}

void Window::Show(bool do_show)
{
    auto hwnd = native_wnd(this->handle_);
    if (do_show) {
        ::ShowWindow(hwnd, SW_SHOWNA);
        ::BringWindowToTop(hwnd);
        ::SetForegroundWindow(hwnd);
        ::SetFocus(hwnd);
    }
    else
        ::ShowWindow(hwnd, SW_HIDE);
}

void Window::BringToFront()
{
    auto hwnd = native_wnd(this->handle_);
    ::ShowWindow(hwnd, SW_SHOWNA);
    ::BringWindowToTop(hwnd);
    ::SetForegroundWindow(hwnd);
    ::SetFocus(hwnd);
}

auto Window::ContentScale() const -> Scale
{
    // note:
    //
    // - screen dpi is relative to 96 dpi
    // - framebuffer matches screen resolution
    //
    auto hwnd = native_wnd(this->handle_);
    auto const s = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    return Scale{
        .dpi = 96.0f * s,
        .fb_scale = 1.0f,
    };
    /*auto hmon = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    UINT x, y;
    ::GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &x, &y);
    return static_cast<float>(y);*/
}

auto Window::ShouldClose() const -> bool { return should_close_ || all_should_close; }

void Window::SetShouldClose(bool close) { should_close_ = close; }

void Window::NewFrame(bool poll_events)
{
    auto hwnd = native_wnd(this->handle_);

    // todo: figure out how to handle current gl context switching
    // if (glfwGetCurrentContext() != handle_)
    //    glfwMakeContextCurrent(native_wnd(handle_));

    ImGui::SetCurrentContext(context_);
    if (poll_events) {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                all_should_close = true;
            }
            else {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
        }
    }

    Render::NewFrame(*this);
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Window::RenderFrame(bool swap_buffers)
{
    ImGui::Render();
    Render::PrepareViewport(*this);

    if (OnBeforeDraw)
        OnBeforeDraw();

    Render::RenderDrawData();

    if (OnAfterDraw)
        OnAfterDraw();

    Render::SwapBuffers(*this);

    if (pending_locate_) {
        perform_locate(*pending_locate_, pending_constrain_);
        pending_locate_.reset();
    }
}

auto Window::FramebufferSize() const -> Size
{
    auto hwnd = native_wnd(this->handle_);
    RECT r;
    ::GetClientRect(hwnd, &r);
    return {r.right, r.bottom};
}

void Window::handleMove(Pos const& xy)
{
    auto hwnd = native_wnd(this->handle_);
    if (!::IsWindow(hwnd))
        return;
    auto const sty = ::GetWindowLongW(hwnd, GWL_STYLE);
    auto const iconified = (sty & WS_ICONIC) != 0;
    auto const maximized = (sty & WS_MAXIMIZE) != 0;
    if (!fullscreen && !iconified && !maximized)
        regular_bounds.pos = xy;
}

void Window::handleResize(Size const& wh)
{
    auto hwnd = native_wnd(this->handle_);
    if (!::IsWindow(hwnd))
        return;
    auto const sty = ::GetWindowLongW(hwnd, GWL_STYLE);
    auto const iconified = (sty & WS_ICONIC) != 0;
    auto const maximized = (sty & WS_MAXIMIZE) != 0;
    if (!fullscreen && !iconified && !maximized)
        regular_bounds.size = wh;
}

auto Window::Locate() const -> Location
{
    auto hwnd = native_wnd(this->handle_);

    if (!::IsWindow(hwnd))
        return {};

    auto const sty = ::GetWindowLongW(hwnd, GWL_STYLE);
    auto const iconified = (sty & WS_ICONIC) != 0;
    auto const maximized = (sty & WS_MAXIMIZE) != 0;

    if (!fullscreen && !iconified && !maximized) {
        auto wp = WINDOWPLACEMENT{sizeof(WINDOWPLACEMENT)};
        ::GetWindowPlacement(hwnd, &wp);
        auto const& r = wp.rcNormalPosition;
        regular_bounds.pos = {r.left, r.top};
        regular_bounds.size = {r.right - r.left, r.bottom - r.top};
    }

    return {regular_bounds.pos, regular_bounds.size, maximized, fullscreen};
}

auto Window::IsMinimized() const -> bool
{
    auto hwnd = native_wnd(this->handle_);
    if (!::IsWindow(hwnd))
        return false;
    auto const sty = ::GetWindowLongW(hwnd, GWL_STYLE);
    return (sty & WS_MINIMIZE) != 0;
}

void Window::perform_locate(Location const& loc, bool constrain_to_monitor)
{
    auto hwnd = native_wnd(this->handle_);
    if (!::IsWindow(hwnd))
        return;

    auto sty = ::GetWindowLongW(hwnd, GWL_STYLE);
    auto exsty = ::GetWindowLongW(hwnd, GWL_EXSTYLE);
    auto const visible = (sty & WS_VISIBLE) != 0;
    auto const iconified = (sty & WS_ICONIC) != 0;
    auto const maximized = (sty & WS_MAXIMIZE) != 0;

    auto want_fullscreen = loc.FullScreen.value_or(fullscreen);
    auto want_maximized = loc.Maximized.value_or(maximized);

    if (!fullscreen || maximized) {
        auto wp = WINDOWPLACEMENT{sizeof(WINDOWPLACEMENT)};
        ::GetWindowPlacement(hwnd, &wp);
        auto const& r = wp.rcNormalPosition;
        regular_bounds.pos = {r.left, r.top};
        regular_bounds.size = Size{r.right - r.left, r.bottom - r.top};
    }

    if (loc.Pos)
        regular_bounds.pos = *loc.Pos;
    if (loc.Size)
        regular_bounds.size = *loc.Size;

    if (constrain_to_monitor)
        regular_bounds = ConstrainToMonitor(regular_bounds);

    auto const regular_rect = RECT{
        .left = regular_bounds.pos.x,
        .top = regular_bounds.pos.y,
        .right = regular_bounds.pos.x + regular_bounds.size.w,
        .bottom = regular_bounds.pos.y + regular_bounds.size.h,
    };

    auto get_monitor_rect = [&](bool workarea) {
        auto mon = ::MonitorFromRect(&regular_rect, MONITOR_DEFAULTTONEAREST);
        auto info = MONITORINFO{sizeof(MONITORINFO)};
        ::GetMonitorInfoW(mon, &info);
        return workarea ? info.rcWork : info.rcMonitor;
    };

    auto setup_normal_placement = [&](RECT const& r, UINT show_cmd) {
        auto wp = WINDOWPLACEMENT{sizeof(WINDOWPLACEMENT)};
        wp.rcNormalPosition = r;
        wp.showCmd = show_cmd;
        ::SetWindowPlacement(hwnd, &wp);
    };

    auto set_bounds = [&](RECT const& r) {
        ::SetWindowPos(hwnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    };

    suspendNotifications = true;

    if (!visible) {
        setup_normal_placement(regular_rect, SW_HIDE);
    };

    if (!fullscreen && want_fullscreen) {
        fullscreen = true;

        sty &= ~WS_BORDER;
        sty &= ~WS_DLGFRAME;
        sty &= ~WS_THICKFRAME;
        sty |= WS_POPUP;
        exsty &= ~WS_EX_WINDOWEDGE;
        exsty |= WS_EX_TOPMOST;

        if (want_maximized)
            sty |= WS_MAXIMIZE;
        else
            sty &= ~WS_MAXIMIZE;

        ::SetWindowLongW(hwnd, GWL_STYLE, sty);
        ::SetWindowLongW(hwnd, GWL_EXSTYLE, exsty);
        set_bounds(get_monitor_rect(false));
    }
    else if (fullscreen && !want_fullscreen) {
        fullscreen = false;

        sty |= WS_BORDER;
        sty |= WS_DLGFRAME;
        sty |= WS_THICKFRAME;
        sty &= ~WS_POPUP;
        exsty |= WS_EX_WINDOWEDGE;
        exsty &= ~WS_EX_TOPMOST;

        if (want_maximized)
            sty |= WS_MAXIMIZE;
        else
            sty &= ~WS_MAXIMIZE;

        ::SetWindowLongW(hwnd, GWL_STYLE, sty);
        ::SetWindowLongW(hwnd, GWL_EXSTYLE, exsty);

        auto r = want_maximized ? get_monitor_rect(true) : regular_rect;
        set_bounds(r);
    }
    else if (fullscreen) {
        if (want_maximized && !maximized)
            ::ShowWindow(hwnd, SW_MAXIMIZE);
        if (!want_maximized && maximized)
            ::ShowWindow(hwnd, SW_NORMAL);
    }
    else { // !fullscreen
        if (visible) {
            setup_normal_placement(regular_rect, want_maximized ? SW_MAXIMIZE : SW_NORMAL);
        }
        else {
            ::SetWindowLongW(
                hwnd, GWL_STYLE, want_maximized ? sty | WS_MAXIMIZE : sty & ~WS_MAXIMIZE);
            auto r = want_maximized ? get_monitor_rect(true) : regular_rect;
            set_bounds(r);
        }
    }

    suspendNotifications = false;
}

void Window::SetTitle(char const* s)
{
    auto hwnd = native_wnd(this->handle_);
    if (!::IsWindow(hwnd))
        return;

    auto ws = to_wstring(s);
    ::SetWindowTextW(hwnd, ws.c_str());
}

void Window::EnableDropFiles(bool allow)
{
    auto hwnd = native_wnd(this->handle_);
    ::DragAcceptFiles(hwnd, allow);
}

auto ConstrainToMonitor(Window::Bounds const& b) -> Window::Bounds
{
    auto const regular_rect = RECT{
        .left = b.pos.x,
        .top = b.pos.y,
        .right = b.pos.x + b.size.w,
        .bottom = b.pos.y + b.size.h,
    };
    auto ret = b;

    auto mon = ::MonitorFromRect(&regular_rect, MONITOR_DEFAULTTONEAREST);
    auto info = MONITORINFO{sizeof(MONITORINFO)};
    if (!::GetMonitorInfoW(mon, &info))
        return b;

    auto const& r = info.rcWork;
    if (r.right <= r.left || r.bottom <= r.top)
        return b;

    ret.size.w = std::min(ret.size.w, int(r.right - r.left));
    ret.size.h = std::min(ret.size.h, int(r.bottom - r.top));
    ret.pos.x = std::clamp(ret.pos.x, int(r.left), int(r.right - ret.size.w));
    ret.pos.y = std::clamp(ret.pos.y, int(r.top), int(r.bottom - ret.size.h));
    return ret;
}

auto PrimaryMonitorWorkArea() -> Window::Bounds
{
    auto mon = ::MonitorFromPoint(POINT(0, 0), MONITOR_DEFAULTTONEAREST);
    auto info = MONITORINFO{sizeof(MONITORINFO)};
    if (::GetMonitorInfoW(mon, &info)) {
        auto const& r = info.rcWork;
        return Window::Bounds{
            Window::Pos{r.left, r.top},
            Window::Size{r.right - r.left, r.bottom - r.top},
        };
    }
    return {{0, 0}, {640, 480}};
}

void InvalidateDeviceObjects() { ImGui_ImplDX11_InvalidateDeviceObjects(); }

} // namespace ImPlus::Host