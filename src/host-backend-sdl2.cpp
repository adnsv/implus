#include "implus/host-render.hpp"
#include "implus/host.hpp"

#include <imgui_internal.h>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_syswm.h>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <backends/imgui_impl_sdl2.h>

#ifdef IMPLUS_RENDER_GL3
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#if defined(__APPLE__)
// GLSL 150
const char* glsl_version = "#version 150";

#elif defined(__ANDROID__)
const char* glsl_version = nullptr;

#elif defined(__EMSCRIPTEN__)
// GLSL 100
const char* glsl_version = "#version 100";

#else
// GLSL 130
const char* glsl_version = "#version 130";

#endif
static auto glad_inited = false;

#endif

#include <algorithm>
#include <cstdio>

namespace ImPlus::Host {

static bool sdl_inited = false;
static SDL_GLContext gl_context_ = nullptr;
extern Window* main_window_;
extern std::size_t window_counter_;

using SDL_WindowID = Uint32;
using SDL_DisplayID = int;

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
    return native ? reinterpret_cast<Window*>(SDL_GetWindowData(native, "implus-wnd")) : nullptr;
}
inline auto window_from_id(SDL_WindowID id) -> Window*
{
    auto sdlw = SDL_GetWindowFromID(id);
    return sdlw ? reinterpret_cast<Window*>(SDL_GetWindowData(sdlw, "implus-wnd")) : nullptr;
}

static int event_watcher(void* data, SDL_Event* event)
{
    switch (event->type) {
    case SDL_WINDOWEVENT: {
        switch (event->window.event) {
        case SDL_WINDOWEVENT_EXPOSED: {
            auto w = window_from_id(event->window.windowID);
            if (w && w->OnRefresh && AllowRefresh())
                w->OnRefresh();
        } break;
        case SDL_WINDOWEVENT_RESIZED: {
            if (auto w = window_from_id(event->window.windowID))
                notifyResize(*w, Window::Size{event->window.data1, event->window.data2});
        } break;
        case SDL_WINDOWEVENT_MOVED: {
            if (auto w = window_from_id(event->window.windowID))
                notifyMove(*w, Window::Pos{event->window.data1, event->window.data2});
        } break;
        case SDL_WINDOWEVENT_SIZE_CHANGED: {
            auto w = window_from_id(event->window.windowID);
            if (w && w->OnFramebufferSize)
                w->OnFramebufferSize(Window::Size{event->window.data1, event->window.data2});
        } break;
        }
    } break;

    case SDL_DROPFILE: {
        auto w = window_from_id(event->drop.windowID);
        if (w && w->OnDropFiles) {
            auto p = event->drop.file;
            w->OnDropFiles(1, (const char**)&p);
            SDL_free(p);
        }
    } break;

    case SDL_APP_WILLENTERBACKGROUND: {
         if (OnEnterBackground)
            OnEnterBackground();
    } break;

    case SDL_APP_DIDENTERFOREGROUND: {
        if (OnEnterForeground)
            OnEnterForeground();
    } break;

    default: return 0;
    }

    return 0;
}

Window::Window(InitLocation const& loc, char const* title, Attrib attr)
    : regular_attr_{attr}
{
    ++window_counter_;

    if (!sdl_inited) {
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
            std::fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        }
        sdl_inited = true;

        struct atexit {
            ~atexit() { SDL_Quit(); }
        };

        static auto _ = atexit{};
    }

#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#elif defined(__ANDROID__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

#elif defined(__EMSCRIPTEN__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,1);

#elif defined(__linux__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    auto window_flags = SDL_WindowFlags(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
    window_flags = SDL_WindowFlags(window_flags | SDL_WINDOW_HIDDEN);

    if (!contains(regular_attr_, Attrib::Decorated))
        window_flags = SDL_WindowFlags(window_flags | SDL_WINDOW_BORDERLESS);
#endif

    if (contains(regular_attr_, Attrib::Resizable))
        window_flags = SDL_WindowFlags(window_flags | SDL_WINDOW_RESIZABLE);

    auto sz = Size{0, 0};

    auto native = SDL_CreateWindow(
        title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 256, window_flags);

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

    gl_context_ = SDL_GL_CreateContext(native);
    SDL_GL_MakeCurrent(native, gl_context_);
    if (!glad_inited) {
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            SDL_GL_DeleteContext(gl_context_);
            SDL_DestroyWindow(native);
            handle_ = nullptr;
            std::fputs("GLAD initialization failure\n", stderr);
            return;
        }
        glad_inited = true;
    }

    SDL_GL_SetSwapInterval(1);

    SDL_SetWindowData(native, "implus-wnd", this);

    SDL_AddEventWatch(event_watcher, nullptr);

    context_ = ImGui::CreateContext();

    if (window_counter_ == 1) {
        IMGUI_CHECKVERSION();
        ImGui::StyleColorsDark();

        main_window_ = this;
        ImGui_ImplOpenGL3_Init(glsl_version);
        ImGui_ImplSDL2_InitForOpenGL(native, gl_context_);
    }
}

Window::~Window()
{
    --window_counter_;

    if (this == main_window_)
        main_window_ = nullptr;

    auto native = native_wnd(handle_);
    if (window_counter_ == 0) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    }
    if (context_)
        ImGui::DestroyContext(context_);
    if (gl_context_)
        SDL_GL_DeleteContext(gl_context_);
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
    auto dispidx = SDL_GetWindowDisplayIndex(wnd);
    float dpi = 96.0f;
    SDL_GetDisplayDPI(dispidx, &dpi, nullptr, nullptr);
    if (dpi < 1.0f)
        dpi = 96.0f;

    return {
        .dpi = dpi,
        .fb_scale = 1.0f,
    };
}

auto all_should_close_ = false;

auto Window::ShouldClose() const -> bool { return should_close_ || all_should_close_; }

void Window::SetShouldClose(bool close) { should_close_ = true; }

void Window::NewFrame(bool poll_events) const
{
    auto& io = ImGui::GetIO();

    // todo: figure out how to handle current gl context switching
    // if (glfwGetCurrentContext() != handle_)
    //    glfwMakeContextCurrent(native_wnd(handle_));

    ImGui::SetCurrentContext(context_);

    if (poll_events) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                all_should_close_ = true;

            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
                auto w = window_from_id(event.window.windowID);
                if (w)
                    w->SetShouldClose(true);
            }
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void Window::RenderFrame(bool swap_buffers)
{
    ImGui::Render();
    int display_w, display_h;

    SDL_GetWindowSizeInPixels(native_wnd(handle_), &display_w, &display_h);

    glViewport(0, 0, display_w, display_h);
    glClearColor(Background.x, Background.y, Background.z, Background.w);
    glClear(GL_COLOR_BUFFER_BIT);

    if (OnBeforeDraw)
        OnBeforeDraw();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (OnAfterDraw)
        OnAfterDraw();

    if (swap_buffers)
        SDL_GL_SwapWindow(native_wnd(handle_));

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
    return SDL_GetRectDisplayIndex(&t);
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
    SDL_EventState(SDL_DROPFILE, allow ? SDL_ENABLE : SDL_DISABLE);
    //SDL_ToggleDragAndDropSupport();
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
    SDL_Rect r;
    SDL_GetDisplayUsableBounds(0, &r);
    return Window::Bounds{{r.x, r.y}, {r.w, r.h}};
}

void InvalidateDeviceObjects()
{
    // something is wrong with resource re-creation
    // for now, we only force font re-creation as a temporary workaround
    // ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();
}

} // namespace ImPlus::Host