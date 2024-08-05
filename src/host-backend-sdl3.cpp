#include "implus/host-render.hpp"
#include "implus/host.hpp"

#include <imgui_internal.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "host-render.hpp"
#include <backends/imgui_impl_sdl3.h>

#include <algorithm>
#include <cstdio>

namespace ImPlus::Host {

static bool sdl_inited = false;
extern Window* main_window_;
extern std::size_t window_counter_;

// static void handleRefreshCallback(GLFWwindow* native)
//{
//     if (auto w = window_from_native(native))
//         if (w->OnRefresh && AllowRefresh())
//             w->OnRefresh();
// }
//
// static void handleFramebufferSizeCallback(
//     GLFWwindow* native, int width, int height)
//{
//     if (auto w = window_from_native(native))
//         if (w->OnFramebufferSize)
//             w->OnFramebufferSize({width, height});
// }
//
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

extern auto AllowRefresh() -> bool;

inline auto native_wnd(void* h) -> SDL_Window* { return reinterpret_cast<SDL_Window*>(h); }

inline auto window_from_native(SDL_Window* native) -> Window*
{
    if (!native)
        return nullptr;

    return reinterpret_cast<Window*>(
        SDL_GetPointerProperty(SDL_GetWindowProperties(native), "implus-wnd", nullptr));
}
inline auto window_from_id(SDL_WindowID id) -> Window*
{
    auto sdlw = SDL_GetWindowFromID(id);
    if (!sdlw)
        return nullptr;

    return reinterpret_cast<Window*>(
        SDL_GetPointerProperty(SDL_GetWindowProperties(sdlw), "implus-wnd", nullptr));
}

static int event_watcher(void* data, SDL_Event* event)
{
    switch (event->type) {

    case SDL_EVENT_WINDOW_EXPOSED: {
        auto w = window_from_id(event->window.windowID);
        if (w && w->OnRefresh && AllowRefresh())
            w->OnRefresh();
    } break;

    case SDL_EVENT_WINDOW_RESIZED: {
        if (auto w = window_from_id(event->window.windowID))
            notifyResize(*w, Window::Size{event->window.data1, event->window.data2});
    } break;

    case SDL_EVENT_WINDOW_MOVED: {
        if (auto w = window_from_id(event->window.windowID))
            notifyMove(*w, Window::Pos{event->window.data1, event->window.data2});
    } break;

    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        auto w = window_from_id(event->window.windowID);
        if (w && w->OnFramebufferSize)
            w->OnFramebufferSize(Window::Size{event->window.data1, event->window.data2});
    } break;

    case SDL_EVENT_DROP_FILE: {
        auto w = window_from_id(event->drop.windowID);
        if (w && w->OnDropFiles) {
            auto p = event->drop.data;
            w->OnDropFiles(1, (const char**)&p);
        }
    }

    default: return 0;
    }

    return 0;
}

Window::Window(InitLocation const& loc, char const* title, Attrib attr)
    : regular_attr_{attr}
{
    ++window_counter_;

    if (!sdl_inited) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMEPAD) != 0) {
            std::fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        }
        sdl_inited = true;

        struct atexit {
            ~atexit() { SDL_Quit(); }
        };

        static auto _ = atexit{};
    }

    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "1");

#if defined(IMPLUS_RENDER_GL3)
    auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#elif defined(IMPLUS_RENDER_VULKAN)
    auto window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
    window_flags = window_flags | SDL_WINDOW_HIDDEN;

    if (!contains(regular_attr_, Attrib::Decorated))
        window_flags = window_flags | SDL_WINDOW_BORDERLESS;
#endif

    if (contains(regular_attr_, Attrib::Resizable))
        window_flags = window_flags | SDL_WINDOW_RESIZABLE;

    auto sz = Size{0, 0};

    auto native = SDL_CreateWindow(title, 512, 512, window_flags);
    SDL_SetPointerProperty(SDL_GetWindowProperties(native), "implus-wnd", this);

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

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
    SDL_SetWindowPosition(native, b.pos.x, b.pos.y);
    SDL_SetWindowSize(native, b.size.w, b.size.h);
#endif

    int x, y, w, h;
    SDL_GetWindowPosition(native, &x, &y);
    SDL_GetWindowSize(native, &w, &h);
    regular_bounds.pos = {x, y};
    regular_bounds.size = {w, h};

    Renderer::SetupInstance(*this);
    Renderer::SetupWindow(*this);

    SDL_AddEventWatch(event_watcher, nullptr);

    context_ = ImGui::CreateContext();

    if (window_counter_ == 1) {
        IMGUI_CHECKVERSION();
        ImGui::StyleColorsDark();

        main_window_ = this;
        Renderer::SetupImplementation(*this);
    }
}

Window::~Window()
{
    --window_counter_;

    if (this == main_window_)
        main_window_ = nullptr;

    auto native = native_wnd(handle_);
    if (window_counter_ == 0) {
        Renderer::ShutdownImplementation();
        ImGui_ImplSDL3_Shutdown();
    }
    if (context_)
        ImGui::DestroyContext(context_);
    Renderer::ShutdownInstance();
    if (handle_)
        SDL_DestroyWindow(native);
}

void Window::Show(bool do_show)
{
    if (do_show)
        SDL_ShowWindow(native_wnd(handle_));
    else
        SDL_HideWindow(native_wnd(handle_));
}

void Window::BringToFront()
{
    auto sw = native_wnd(handle_);
    auto ff = SDL_GetWindowFlags(sw);

    if (ff & SDL_WINDOW_HIDDEN)
        SDL_ShowWindow(sw);

    if (ff & SDL_WINDOW_MINIMIZED)
        SDL_RestoreWindow(sw);

    SDL_RaiseWindow(sw);
}

auto Window::ContentScale() const -> Scale
{
    auto wnd = native_wnd(handle_);

    return {
        .dpi = 96.0f * SDL_GetWindowDisplayScale(wnd),
        .fb_scale = 1.0f,
    };
}

auto all_should_close_ = false;

auto Window::ShouldClose() const -> bool { return should_close_ || all_should_close_; }

void Window::SetShouldClose(bool close) { should_close_ = true; }

void Window::NewFrame(bool poll_events)
{
    auto& io = ImGui::GetIO();

    // todo: figure out how to handle current gl context switching
    // if (glfwGetCurrentContext() != handle_)
    //    glfwMakeContextCurrent(native_wnd(handle_));

    ImGui::SetCurrentContext(context_);

    if (poll_events) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                all_should_close_ = true;

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                auto w = window_from_id(event.window.windowID);
                if (w)
                    w->SetShouldClose(true);
            }
        }
    }

    Renderer::NewFrame(*this);
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void Window::RenderFrame(bool swap_buffers)
{
    ImGui::Render();
    Renderer::PrepareViewport(*this);

    if (OnBeforeDraw)
        OnBeforeDraw();

    Renderer::RenderDrawData();

    if (OnAfterDraw)
        OnAfterDraw();

    if (swap_buffers)
        Renderer::SwapBuffers(*this);

    if (pending_locate_) {
        perform_locate(*pending_locate_, pending_constrain_);
        pending_locate_.reset();
    }
}

auto Window::FramebufferSize() const -> Size
{
    int w, h;
    SDL_GetWindowSizeInPixels(native_wnd(handle_), &w, &h);
    return {w, h};
}

auto Window::Locate() const -> Location
{
    auto w = native_wnd(handle_);
    auto ff = SDL_GetWindowFlags(w);
    auto iconified = bool(ff & SDL_WINDOW_MINIMIZED);
    auto maximized = bool(ff & SDL_WINDOW_MAXIMIZED);
    // auto fullscreen = bool(ff & SDL_WINDOW_FULLSCREEN);

    if (!fullscreen && !iconified && !maximized) {
        SDL_GetWindowPosition(w, &regular_bounds.pos.x, &regular_bounds.pos.y);
        SDL_GetWindowSize(w, &regular_bounds.size.w, &regular_bounds.size.h);
    }
    return {regular_bounds.pos, regular_bounds.size, maximized, fullscreen};
}

auto Window::IsMinimized() const -> bool
{
    auto w = native_wnd(handle_);
    return bool(SDL_GetWindowFlags(w) & SDL_WINDOW_MINIMIZED);
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

auto getMonitorArea(SDL_DisplayID mon) -> Window::Bounds
{
    auto p = Window::Pos{0, 0};
    SDL_Rect r;
    SDL_GetDisplayBounds(mon, &r);
    return {{r.x, r.y}, {r.w, r.h}};
}

auto findClosestMonitor(Window::Bounds const& r) -> SDL_DisplayID
{
    auto t = SDL_Rect{
        .x = r.pos.x,
        .y = r.pos.y,
        .w = r.size.w,
        .h = r.size.h,
    };
    return SDL_GetDisplayForRect(&t);
}

void Window::perform_locate(Location const& loc, bool constrain_to_monitor)
{
    auto w = native_wnd(handle_);
    auto ff = SDL_GetWindowFlags(w);
    auto iconified = bool(ff & SDL_WINDOW_MINIMIZED);
    auto maximized = bool(ff & SDL_WINDOW_MAXIMIZED);
    //    auto fullscreen = bool(ff & SDL_WINDOW_FULLSCREEN);
    auto visible = !bool(ff & SDL_WINDOW_HIDDEN);

    if (!fullscreen && !iconified && !maximized) {
        SDL_GetWindowPosition(w, &regular_bounds.pos.x, &regular_bounds.pos.y);
        SDL_GetWindowSize(w, &regular_bounds.size.w, &regular_bounds.size.h);
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
#ifdef GLFW_EXPOSE_NATIVE_WIN32
        // tune "restore from maximized"
        auto hwnd = glfwGetWin32Window(w);
        auto wp = WINDOWPLACEMENT{sizeof(WINDOWPLACEMENT)};
        wp.rcNormalPosition = {regular_pos.x, regular_pos.y, regular_pos.x + regular_size.w,
            regular_pos.y + regular_size.h};
        wp.showCmd = SW_HIDE;
        ::SetWindowPlacement(hwnd, &wp);
#endif
    }

    if (!fullscreen && want_fullscreen) {
        fullscreen = true;
        auto mon = findClosestMonitor(regular_bounds);
        auto area = getMonitorArea(mon);

        // hide decorations
        SDL_SetWindowFullscreen(w, SDL_TRUE);

        if (!visible && want_maximized)
            SDL_MaximizeWindow(w);

        SDL_SetWindowPosition(w, area.pos.x, area.pos.y);
        SDL_SetWindowSize(w, area.size.w, area.size.h);
    }
    else if (fullscreen && !want_fullscreen) {
        fullscreen = false;

        SDL_SetWindowFullscreen(w, SDL_FALSE);

        if (want_maximized) {
            SDL_MaximizeWindow(w);
        }
        else {
            SDL_SetWindowPosition(w, regular_bounds.pos.x, regular_bounds.pos.y);
            SDL_SetWindowSize(w, regular_bounds.size.w, regular_bounds.size.h);
            if (maximized)
                SDL_RestoreWindow(w);
        }
    }
    else if (fullscreen) {
        if (!maximized && want_maximized) {
            SDL_MaximizeWindow(w);
        }
        if (maximized && !want_maximized)
            SDL_RestoreWindow(w);
    }
    else { // !fullscreen
        if (!maximized) {
            SDL_SetWindowPosition(w, regular_bounds.pos.x, regular_bounds.pos.y);
            SDL_SetWindowSize(w, regular_bounds.size.w, regular_bounds.size.h);

            if (want_maximized)
                SDL_MaximizeWindow(w);
        }
        else { // maximized
            if (!want_maximized) {
                SDL_SetWindowPosition(w, regular_bounds.pos.x, regular_bounds.pos.y);
                SDL_SetWindowSize(w, regular_bounds.size.w, regular_bounds.size.h);
                SDL_RestoreWindow(w);
            }
        }
    }

    suspendNotifications = false;
}

void Window::Cleanup()
{
    if (sdl_inited)
        SDL_Quit();
}

void Window::SetTitle(char const* s) { SDL_SetWindowTitle(native_wnd(this->handle_), s); }

void Window::handleMove(Pos const& xy)
{
    auto w = native_wnd(this->handle_);
    auto ff = SDL_GetWindowFlags(w);
    auto iconified = bool(ff & SDL_WINDOW_MINIMIZED);
    auto maximized = bool(ff & SDL_WINDOW_MAXIMIZED);
    if (!fullscreen && !iconified && !maximized)
        regular_bounds.pos = xy;
}

void Window::handleResize(Size const& wh)
{
    auto w = native_wnd(this->handle_);
    auto ff = SDL_GetWindowFlags(w);
    auto iconified = bool(ff & SDL_WINDOW_MINIMIZED);
    auto maximized = bool(ff & SDL_WINDOW_MAXIMIZED);
    if (!fullscreen && !iconified && !maximized)
        regular_bounds.size = wh;
}

void Window::EnableDropFiles(bool allow)
{
    auto _handle = native_wnd(this->handle_);
    SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, allow ? SDL_TRUE : SDL_FALSE);
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

    auto mon = SDL_GetPrimaryDisplay();
    SDL_Rect r;
    SDL_GetDisplayUsableBounds(mon, &r);
    return Window::Bounds{{r.x, r.y}, {r.w, r.h}};
}

void InvalidateDeviceObjects() { Renderer::InvalidateDeviceObjects(); }

} // namespace ImPlus::Host