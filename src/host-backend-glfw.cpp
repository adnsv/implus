#include <implus/host.hpp>
#include <implus/render-device.hpp>

#include "host-render.hpp"
#include <imgui_internal.h>

#include <GLFW/glfw3.h>
#ifdef GLFW_BACKEND_SUPPORTS_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <backends/imgui_impl_glfw.h>

#include <algorithm>
#include <cstdio>

namespace ImPlus::Host {

static auto glfw_inited = false;
extern Window* main_window_;
extern std::size_t window_counter_;

extern auto AllowRefresh() -> bool;

inline auto native_wnd(void* h) -> GLFWwindow* { return reinterpret_cast<GLFWwindow*>(h); }

inline auto window_from_native(GLFWwindow* native)
{
    return native ? reinterpret_cast<Window*>(glfwGetWindowUserPointer(native)) : nullptr;
}

static void handleRefreshCallback(GLFWwindow* native)
{
    if (auto w = window_from_native(native))
        if (w->OnRefresh && AllowRefresh())
            w->OnRefresh();
}

static void handleFramebufferSizeCallback(GLFWwindow* native, int width, int height)
{
    if (auto w = window_from_native(native))
        if (w->OnFramebufferSize)
            w->OnFramebufferSize({width, height});
}

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

void handleWindowPosCallback(GLFWwindow* native, int xpos, int ypos)
{
    if (auto w = window_from_native(native))
        notifyMove(*w, {xpos, ypos});
}

void handleWindowSizeCallback(GLFWwindow* native, int width, int height)
{
    if (auto w = window_from_native(native))
        notifyResize(*w, {width, height});
}

Window::Window(InitLocation const& loc, char const* title, Attrib attr)
    : regular_attr_{attr}
{
    ++window_counter_;

    if (!glfw_inited) {
        glfwSetErrorCallback([](int error, char const* description) {
            std::fprintf(stderr, "GLFW Error: %s\n", description);
        });

#ifndef __EMSCRIPTEN__
        // if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND))
        //    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
#endif

        auto success = glfwInit();
        if (!success)
            std::fprintf(stderr, "GLFW initialization failure\n");

        glfw_inited = true;

        struct atexit {
            ~atexit() { Cleanup(); }
        };

        static auto _ = atexit{};
    }

    Render::SetupWindowHints();

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    glfwWindowHint(
        GLFW_DECORATED, contains(regular_attr_, Attrib::Decorated) ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(
        GLFW_RESIZABLE, contains(regular_attr_, Attrib::Resizable) ? GLFW_TRUE : GLFW_FALSE);

    auto sz = Size{0, 0};

    auto native = glfwCreateWindow(256, 256, title, nullptr, nullptr);
    glfwSetWindowUserPointer(native, this);

    auto b = PrimaryMonitorWorkArea();
    b.inflate(-b.size.w / 10, -b.size.h / 10);
    if (auto sz = std::get_if<Size>(&loc)) {
        if (sz->w > 0)
            b.size.w = sz->w;
        if (sz->h > 0)
            b.size.h = sz->h;
    }
    else if (auto bb = std::get_if<Bounds>(&loc)) {
        b = *bb;
    }

    b = ConstrainToMonitor(b);

    handle_ = native;

    if (!handle_)
        return;

    glfwSetWindowPos(native, b.pos.x, b.pos.y);
    glfwSetWindowSize(native, b.size.w, b.size.h);

    int x, y, w, h;
    glfwGetWindowPos(native, &x, &y);
    glfwGetWindowSize(native, &w, &h);
    regular_bounds.pos = {x, y};
    regular_bounds.size = {w, h};

    if (window_counter_ == 1) {
        Render::SetupInstance(*this);
        if (Render::OnDeviceChange)
            Render::OnDeviceChange(Render::GetDeviceInfo());
    }

    Render::SetupWindow(*this);

    context_ = ImGui::CreateContext();

    if (window_counter_ == 1) {
        IMGUI_CHECKVERSION();
        ImGui::StyleColorsDark();

        main_window_ = this;
        Render::SetupImplementation(*this);
    }

    glfwSetWindowRefreshCallback(native, handleRefreshCallback);
    glfwSetFramebufferSizeCallback(native, handleFramebufferSizeCallback);

    glfwSetWindowPosCallback(native, handleWindowPosCallback);
    glfwSetWindowSizeCallback(native, handleWindowSizeCallback);
}

Window::~Window()
{
    --window_counter_;

    if (this == main_window_)
        main_window_ = nullptr;

    auto native = native_wnd(handle_);
    if (window_counter_ == 0) {
        if (Render::OnDeviceChange)
            Render::OnDeviceChange({});
        Render::ShutdownImplementation();
        ImGui_ImplGlfw_Shutdown();
    }
    if (context_)
        ImGui::DestroyContext(context_);
    if (window_counter_ == 0)
        Render::ShutdownInstance();
    if (handle_)
        glfwDestroyWindow(native);
}

void Window::Show(bool do_show)
{
    if (do_show)
        glfwShowWindow(native_wnd(handle_));
    else
        glfwHideWindow(native_wnd(handle_));
}

void Window::BringToFront()
{
    auto h = native_wnd(handle_);
    if (!glfwGetWindowAttrib(h, GLFW_VISIBLE))
        glfwShowWindow(h);
    if (glfwGetWindowAttrib(h, GLFW_ICONIFIED))
        glfwRestoreWindow(h);
    glfwFocusWindow(h);
}

auto Window::ContentScale() const -> Scale
{
    float s = 1.0f;
    glfwGetWindowContentScale(native_wnd(handle_), &s, nullptr);

#ifdef IMPLUS_HOST_GLFW_COCOA
    // macos is different
    return {
        .dpi = 72.0f,
        .fb_scale = s,
    };
#endif

    return Scale{
        .dpi = 96.0f * s,
        .fb_scale = 1.0f,
    };
}

auto Window::ShouldClose() const -> bool
{
    return static_cast<bool>(glfwWindowShouldClose(native_wnd(handle_)));
}

void Window::SetShouldClose(bool close) { glfwSetWindowShouldClose(native_wnd(handle_), close); }

void Window::NewFrame(bool poll_events)
{
    if (glfwGetCurrentContext() != handle_)
        glfwMakeContextCurrent(native_wnd(handle_));
    ImGui::SetCurrentContext(context_);
    if (poll_events)
        glfwPollEvents();
    Render::NewFrame(*this);
    ImGui_ImplGlfw_NewFrame();
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

    if (swap_buffers)
        Render::SwapBuffers(*this);

    if (pending_locate_) {
        perform_locate(*pending_locate_, pending_constrain_);
        pending_locate_.reset();
    }
}

auto Window::FramebufferSize() const -> Size
{
    int w, h;
    glfwGetFramebufferSize(native_wnd(handle_), &w, &h);
    return {w, h};
}

auto Window::Locate() const -> Location
{
    auto w = native_wnd(handle_);
    auto iconified = bool(glfwGetWindowAttrib(w, GLFW_ICONIFIED));
    auto maximized = bool(glfwGetWindowAttrib(w, GLFW_MAXIMIZED));

    if (!fullscreen && !iconified && !maximized) {
        glfwGetWindowPos(w, &regular_bounds.pos.x, &regular_bounds.pos.y);
        glfwGetWindowSize(w, &regular_bounds.size.w, &regular_bounds.size.h);
    }
    return {regular_bounds.pos, regular_bounds.size, maximized, fullscreen};
}

auto Window::IsMinimized() const -> bool
{
    auto w = native_wnd(handle_);
    return bool(glfwGetWindowAttrib(w, GLFW_ICONIFIED));
}

auto overlap(Window::Bounds const& a, Window::Bounds const& b) -> int
{
    auto xl = std::max(a.min().x, b.min().x);
    auto yl = std::max(a.min().y, b.min().y);
    auto xh = std::min(a.max().x, b.max().x);
    auto yh = std::min(a.max().y, b.max().y);
    return (xh > xl && yh > yl) ? (xh - xl) * (yh - yl) : 0;
}

auto distance(Window::Bounds const& a, Window::Bounds const& b) -> int
{
    int dx = 0;
    int dy = 0;
    if (a.min().x >= b.max().x)
        dx = a.min().x - b.max().x;
    else if (a.max().x <= b.min().x)
        dx = a.max().x - b.min().x;
    if (a.min().y >= b.max().y)
        dy = a.min().y - b.max().y;
    else if (a.max().y <= b.min().y)
        dy = a.max().y - b.min().y;
    return dx * dx + dy * dy;
}

auto getMonitorArea(GLFWmonitor* mon) -> Window::Bounds
{
    auto p = Window::Pos{0, 0};
    glfwGetMonitorPos(mon, &p.x, &p.y);
    auto mode = glfwGetVideoMode(mon);
    return {p, {mode->width, mode->height}};
}

auto findClosestMonitor(Window::Bounds const& r) -> GLFWmonitor*
{
    GLFWmonitor* best_mon = nullptr;
    auto best_overlap = 0;
    auto best_distance = 0;
    auto count = int{};
    auto monitors = glfwGetMonitors(&count);
    for (auto i = 0; i < count; ++i) {
        auto mon = monitors[i];
        auto b = getMonitorArea(mon);
        if (auto o = overlap(b, r)) {
            if (o > best_overlap) {
                best_overlap = o;
                best_mon = mon;
            }
        }
        else if (!best_overlap) {
            auto d = distance(b, r);
            if (!best_mon || d < best_distance) {
                best_distance = d;
                best_mon = mon;
            }
        }
    }
    return best_mon ? best_mon : glfwGetPrimaryMonitor();
}

void Window::perform_locate(Location const& loc, bool constrain_to_monitor)
{
    auto w = native_wnd(handle_);
    auto iconified = bool(glfwGetWindowAttrib(w, GLFW_ICONIFIED));
    auto maximized = bool(glfwGetWindowAttrib(w, GLFW_MAXIMIZED));
    auto visible = bool(glfwGetWindowAttrib(w, GLFW_VISIBLE));

    if (!fullscreen && !iconified && !maximized) {
        glfwGetWindowPos(w, &regular_bounds.pos.x, &regular_bounds.pos.y);
        glfwGetWindowSize(w, &regular_bounds.size.w, &regular_bounds.size.h);
    }

    auto want_fullscreen = loc.FullScreen.value_or(fullscreen);
    auto want_maximized = loc.Maximized.value_or(maximized);

    if (loc.Pos)
        regular_bounds.pos = *loc.Pos;
    if (loc.Size)
        regular_bounds.size = *loc.Size;

    if (constrain_to_monitor)
        regular_bounds = ConstrainToMonitor(regular_bounds);

    suspendNotifications = true;

    if (!visible) {
#ifdef GLFW_BACKEND_SUPPORTS_WIN32
        // tune "restore from maximized"
        auto hwnd = glfwGetWin32Window(w);
        auto wp = WINDOWPLACEMENT{sizeof(WINDOWPLACEMENT)};
        ::GetWindowPlacement(hwnd, &wp);
        wp.rcNormalPosition = {regular_bounds.pos.x, regular_bounds.pos.y,
            regular_bounds.pos.x + regular_bounds.size.w,
            regular_bounds.pos.y + regular_bounds.size.h};
        wp.showCmd = SW_HIDE;
        ::SetWindowPlacement(hwnd, &wp);
#endif
    }

    if (!fullscreen && want_fullscreen) {
        fullscreen = true;
        auto mon = findClosestMonitor(regular_bounds);
        auto area = getMonitorArea(mon);

        // hide decorations
        glfwSetWindowAttrib(w, GLFW_DECORATED, 0);
        glfwSetWindowAttrib(w, GLFW_RESIZABLE, 0);

        if (!visible && want_maximized)
            glfwMaximizeWindow(w);

        glfwSetWindowPos(w, area.pos.x, area.pos.y);
        glfwSetWindowSize(w, area.size.w, area.size.h);
    }
    else if (fullscreen && !want_fullscreen) {
        fullscreen = false;

        // restore decorations
        glfwSetWindowAttrib(
            w, GLFW_DECORATED, contains(regular_attr_, Attrib::Decorated) ? GLFW_TRUE : GLFW_FALSE);
        glfwSetWindowAttrib(
            w, GLFW_RESIZABLE, contains(regular_attr_, Attrib::Resizable) ? GLFW_TRUE : GLFW_FALSE);

        if (want_maximized) {
            glfwMaximizeWindow(w);
        }
        else {
            glfwSetWindowPos(w, regular_bounds.pos.x, regular_bounds.pos.y);
            glfwSetWindowSize(w, regular_bounds.size.w, regular_bounds.size.h);
            if (maximized)
                glfwRestoreWindow(w);
        }
    }
    else if (fullscreen) {
        if (!maximized && want_maximized) {
            glfwMaximizeWindow(w);
        }
        if (maximized && !want_maximized)
            glfwRestoreWindow(w);
    }
    else { // !fullscreen
        if (!maximized) {
            glfwSetWindowPos(w, regular_bounds.pos.x, regular_bounds.pos.y);
            glfwSetWindowSize(w, regular_bounds.size.w, regular_bounds.size.h);
            if (want_maximized)
                glfwMaximizeWindow(w);
        }
        else { // maximized
            if (!want_maximized) {
                glfwSetWindowPos(w, regular_bounds.pos.x, regular_bounds.pos.y);
                glfwSetWindowSize(w, regular_bounds.size.w, regular_bounds.size.h);
                glfwRestoreWindow(w);
            }
        }
    }

    suspendNotifications = false;
}

void Window::Cleanup()
{
    if (glfw_inited)
        glfwTerminate();
}

void Window::SetTitle(char const* s) { glfwSetWindowTitle(native_wnd(this->handle_), s); }

void Window::handleMove(Pos const& xy)
{
    auto w = native_wnd(this->handle_);
    auto iconified = bool(glfwGetWindowAttrib(w, GLFW_ICONIFIED));
    auto maximized = bool(glfwGetWindowAttrib(w, GLFW_MAXIMIZED));
    if (!fullscreen && !iconified && !maximized)
        regular_bounds.pos = xy;
}

void Window::handleResize(Size const& wh)
{
    auto w = native_wnd(this->handle_);
    auto iconified = bool(glfwGetWindowAttrib(w, GLFW_ICONIFIED));
    auto maximized = bool(glfwGetWindowAttrib(w, GLFW_MAXIMIZED));
    if (!fullscreen && !iconified && !maximized)
        regular_bounds.size = wh;
}

static void dropHandler(GLFWwindow* w, int path_count, const char* paths[])
{
    if (!w)
        return;
    auto ptr = glfwGetWindowUserPointer(w);
    if (!ptr)
        return;
    auto window = reinterpret_cast<Window*>(ptr);
    if (window->OnDropFiles)
        window->OnDropFiles(path_count, paths);
}

void Window::EnableDropFiles(bool allow)
{
    auto _handle = native_wnd(this->handle_);
    glfwSetDropCallback(_handle, allow ? dropHandler : nullptr);
}

auto ConstrainToMonitor(Window::Bounds const& b) -> Window::Bounds
{
    auto ret = b;

    auto m = findClosestMonitor(b);
    if (!m)
        return b;

    auto area = getMonitorArea(m);
    if (area.empty())
        return b;

    ret.size.w = std::min(ret.size.w, area.size.w);
    ret.size.h = std::min(ret.size.h, area.size.h);
    ret.pos.x = std::clamp(ret.pos.x, area.pos.x, area.pos.x + area.size.w - ret.size.w);
    ret.pos.y = std::clamp(ret.pos.y, area.pos.y, area.pos.y + area.size.h - ret.size.h);
    return ret;
}

auto PrimaryMonitorWorkArea() -> Window::Bounds
{
    auto r = Window::Bounds{{0, 0}, {640, 480}};
    auto m = glfwGetPrimaryMonitor();
    if (m)
        glfwGetMonitorWorkarea(m, &r.pos.x, &r.pos.y, &r.size.w, &r.size.h);
    return r;
}

void InvalidateDeviceObjects() { Render::InvalidateDeviceObjects(); }

auto SetFeature(Feature f, std::string const& value) -> bool { return false; }
auto GetFeature(Feature f) -> std::string { return ""; }
auto IsFeatureSupported(Feature f) -> bool { return false; }

} // namespace ImPlus::Host