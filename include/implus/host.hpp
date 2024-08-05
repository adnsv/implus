#pragma once

#include <functional>
#include <imgui.h>
#include <optional>
#include <variant>

namespace ImPlus::Host {

struct pixel_pos {
    int x = 0;
    int y = 0;
    pixel_pos() noexcept = default;
    pixel_pos(int x, int y)
        : x{x}
        , y{y}
    {
    }
};

struct pixel_size {
    int w = 0;
    int h = 0;
    pixel_size() noexcept = default;
    pixel_size(int w, int h)
        : w{w}
        , h{h}
    {
    }
};

struct pixel_bounds {
    pixel_pos pos;
    pixel_size size;
    pixel_bounds() noexcept = default;
    pixel_bounds(pixel_bounds const&) noexcept = default;
    pixel_bounds(pixel_pos const& p, pixel_size const& s)
        : pos{p}
        , size{s}
    {
    }
    auto min() const -> pixel_pos const& { return pos; }
    auto max() const -> pixel_pos { return pixel_pos{pos.x + size.w, pos.y + size.h}; }
    void inflate(int dw, int dh)
    {
        pos.x -= dw;
        pos.y -= dh;
        size.w += dw * 2;
        size.h += dh * 2;
    }
    auto empty() const { return size.w <= 0 || size.h <= 0; }
};

struct pixel_scale {
    float dpi = 96.0f;           // nominal dpi
    float fb_scale = 1.0f; // framebuffer pixel scale
};

enum class WindowAttrib {
    None = 0b0000,
    Resizable = 1 << 0,
    Decorated = 1 << 1,
};

inline auto operator|(WindowAttrib a, WindowAttrib b) -> WindowAttrib
{
    return WindowAttrib(unsigned(a) | unsigned(b));
}

inline auto contains(WindowAttrib a, WindowAttrib b) -> bool { return unsigned(a) & unsigned(b); }

struct Window {
    using handle_t = void*;
    using Attrib = WindowAttrib;
    using Pos = pixel_pos;
    using Size = pixel_size;
    using Scale = pixel_scale;
    using Bounds = pixel_bounds;

    struct Location {
        std::optional<Window::Pos> Pos;
        std::optional<Window::Size> Size;
        std::optional<bool> Maximized;
        std::optional<bool> FullScreen;
    };

    using InitLocation = std::variant<Size, Bounds>;

    Window(InitLocation const& loc, char const* title,
        Attrib attr = Attrib::Resizable | Attrib::Decorated);
    Window(Window const&) = delete;
    ~Window();

    operator bool() const { return handle_ != nullptr; }

    auto ContentScale() const -> Scale;

    auto Handle() const -> void* { return handle_; }

    void Show(bool do_show = true);

    auto ShouldClose() const -> bool;
    void SetShouldClose(bool close = true);

    void NewFrame(bool poll_events);
    void RenderFrame(bool swap_buffers = true);
    auto FramebufferSize() const -> Size;

    auto Locate() const -> Location;
    void Locate(Location const& loc, bool constrain_to_monitor = true);

    auto IsFullscreen() const -> bool { return fullscreen; }
    void SetFullscreen(bool on = true);
    void ToggleFullscreen() { SetFullscreen(!fullscreen); }
    auto IsMinimized() const -> bool;
    void BringToFront();

    void SetTitle(char const* s);
    void EnableDropFiles(bool allow);

    ImVec4 Background = ImVec4{0.45f, 0.55f, 0.60f, 1.00f};

    std::function<void()> OnRefresh;
    std::function<void(Size const& sz)> OnFramebufferSize;
    std::function<void()> OnBeforeDraw; // called before ImGui calls RenderDrawData
    std::function<void()> OnAfterDraw;  // called before swapbuffers
    std::function<void(int count, char const* paths[])> OnDropFiles;

private:
    handle_t handle_ = nullptr;
    Attrib regular_attr_ = {};
    ImGuiContext* context_ = nullptr;
    bool should_close_ = false; // may not be used by all backends
    std::optional<Location> pending_locate_;
    bool pending_constrain_ = false;

    mutable Bounds regular_bounds = {};
    bool fullscreen = false;

    static void Cleanup();
    void perform_locate(Location const& loc, bool constrain_to_monitor);

    friend void notifyMove(Window&, Pos const&);
    friend void notifyResize(Window&, Size const&);
    bool suspendNotifications = false;
    void handleMove(Pos const& xy);
    void handleResize(Size const& wh);
};

auto MainWindow() -> Window&;
inline std::function<void()> OnEnterBackground;
    inline std::function<void()> OnEnterForeground;

auto ConstrainToMonitor(Window::Bounds const& b) -> Window::Bounds;

inline auto ConstrainToMonitor(Window::Size& size) -> Window::Size
{
    auto p = ConstrainToMonitor(Window::Bounds{Window::Pos{0, 0}, size});
    return p.size;
}

auto PrimaryMonitorWorkArea() -> Window::Bounds;

//namespace Interprocess {
//auto GetIPWMHandle(Window* w) -> void*;
//void RestoreInterprocessWindowHandle(void* iph);
//
//}; // namespace Interprocess

void InvalidateDeviceObjects();

// WithinFrame returns true while the aplication is within
// NewFrame...RenderFrame scope
auto WithinFrame() -> bool;

enum class NativeHandleType {
    UNKNOWN_HANDLE_TYPE,
    WIN32_HWND,      // Win32 HWND
    COCOA_ID,        // Cocoa id
    X11_WINDOW_XID,  // X11 Window, also XID
    WAYLAND_SURFACE, // Wayland wl_surface*
    GDK_WINDOW,      // GDKWindow*
    ANDROID_WINDOW,  // ANativeWindow*
};

struct NativeWindowHandle {
    void* value = nullptr;
    NativeHandleType type = NativeHandleType::UNKNOWN_HANDLE_TYPE;
};

// GetBackendNativeHandle
//
auto GetBackendNativeHandle(Window const&) -> NativeWindowHandle;

} // namespace ImPlus::Host