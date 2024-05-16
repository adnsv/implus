#include "implus/host.hpp"

#if defined(IMPLUS_HOST_GLFW)
#include <GLFW/glfw3.h>
#endif

#if defined(IMPLUS_HOST_NATIVE_WIN32)
// Native WIN32
#include <windows.h>

#elif defined(IMPLUS_HOST_GLFW)
// GLFW
#if defined(GLFW_BACKEND_SUPPORTS_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#if defined(GLFW_BACKEND_SUPPORTS_COCOA)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#if defined(GLFW_BACKEND_SUPPORTS_X11)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#if defined(GLFW_BACKEND_SUPPORTS_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#if !defined(__EMSCRIPTEN__)
#include <GLFW/glfw3native.h>
#endif

#elif defined(IMPLUS_HOST_SDL2)
// SDL2
#include <SDL.h>
#include <SDL_syswm.h>

#elif defined(IMPLUS_HOST_SDL3)
// SDL3
#include <SDL3/SDL.h>

#else
#error Unsupported Windowing Backend

#endif

auto ImPlus::Host::GetBackendNativeHandle(ImPlus::Host::Window const& wnd) -> NativeWindowHandle
{
#if defined(IMPLUS_HOST_NATIVE_WIN32)
    // HWND
    return {wnd.Handle(), NativeHandleType::WIN32_HWND};

#elif defined(IMPLUS_HOST_SDL2)
    auto sw = static_cast<SDL_Window*>(wnd.Handle());
    SDL_SysWMinfo w;

    if (SDL_GetWindowWMInfo(sw, &w) < 0)
        return {nullptr, NativeHandleType::UNKNOWN_HANDLE_TYPE};

    switch (w.subsystem) {
#if defined(SDL_ENABLE_SYSWM_WINDOWS)
    case SDL_SYSWM_WINDOWS:
        // Windows
        return {w.info.win.window, NativeHandleType::WIN32_HWND};
#endif

#if defined(SDL_ENABLE_SYSWM_X11)
    case SDL_SYSWM_X11:
        // X11
        return {reinterpret_cast<void*>(w.info.x11.window), NativeHandleType::X11_WINDOW_XID};
#endif

#if defined(SDL_ENABLE_SYSWM_COCOA)
    case SDL_SYSWM_COCOA:
        // MacOS
        return {w.info.cocoa.window, NativeHandleType::COCOA_ID};
#endif

#if defined(SDL_ENABLE_SYSWM_WAYLAND)
    case SDL_SYSWM_WAYLAND:
        // Wayland
        return {w.info.wl.surface, NativeHandleType::WAYLAND_SURFACE};
#endif

#if defined(SDL_ENABLE_SYSWM_ANDROID)
    case SDL_SYSWM_ANDROID:
        // Android
        return {w.info.android.window, NativeHandleType::ANDROID_WINDOW};
#endif

    default:
        // Unknown
        return {};
    }

    return {};

#elif defined(IMPLUS_HOST_SDL3)
    auto sw = static_cast<SDL_Window*>(wnd.Handle());
    auto props = SDL_GetWindowProperties(sw);

#if defined(SDL_PLATFORM_WIN32)
    auto hwnd = SDL_GetProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    return {hwnd, NativeHandleType::WIN32_HWND};

#elif defined(SDL_PLATFORM_MACOS)
    // auto p = (__bridge NSWindow *)SDL_GetProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
    // NULL); return {p->id, NativeHandleType::COCOA_ID}

#elif defined(SDL_PLATFORM_LINUX)
    auto drv = SDL_GetCurrentVideoDriver();
    if (SDL_strcmp(drv, "x11") == 0) {
        auto xdisp = SDL_GetProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        auto xwindow = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (xdisp && xwindow) {
            return {reinterpret_cast<void*>(xwindow), NativeHandleType::X11_WINDOW_XID};
        }
        else {
            return {};
        }
    }
    else if (SDL_strcmp(drv, "wayland") == 0) {
        auto disp_ptr = (struct wl_display*)SDL_GetProperty(
            props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        auto surface_ptr = (struct wl_surface*)SDL_GetProperty(
            props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        if (disp_ptr && surface_ptr) {
            return {reinterpret_cast<void*>(surface_ptr), NativeHandleType::WAYLAND_SURFACE};
        }
        else {
            return {};
        }
    }
    else {
        return {};
    }

#endif

    SDL_GetProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, nullptr);
    // todo: implement

#if defined(__WIN32__)
    auto w = SDL_GetProperty(SDL_GetWindowProperties(sw), "SDL.window.win32.hwnd", NULL);
    return {w, NativeHandleType::WIN32_HWND};
#elif defined(__MACOS__)
    auto nswindow = SDL_GetProperty(SDL_GetWindowProperties(sw), "SDL.window.cocoa.window", NULL);
    return {w, NativeHandleType::COCOA_ID};
#endif

#elif defined(__EMSCRIPTEN__)
    // assuming GLFW/EMSCRIPTEN
    return {};

#elif defined(IMPLUS_HOST_GLFW)
    // GLFW
    auto gw = static_cast<GLFWwindow*>(wnd.Handle());

    switch (glfwGetPlatform()) {
#ifdef GLFW_EXPOSE_NATIVE_WIN32
    case GLFW_PLATFORM_WIN32: return {glfwGetWin32Window(gw), NativeHandleType::WIN32_HWND};
#endif
#ifdef GLFW_EXPOSE_NATIVE_COCOA
    case GLFW_PLATFORM_COCOA: return {glfwGetCocoaWindow(gw), NativeHandleType::COCOA_ID};
#endif
#ifdef GLFW_EXPOSE_NATIVE_X11
    case GLFW_PLATFORM_X11:
        return {reinterpret_cast<void*>(glfwGetX11Window(gw)), NativeHandleType::X11_WINDOW_XID};
#endif
#ifdef GLFW_EXPOSE_NATIVE_WAYLAND
    case GLFW_PLATFORM_WAYLAND:
        return {
            reinterpret_cast<void*>(glfwGetWaylandWindow(gw)), NativeHandleType::WAYLAND_SURFACE};
#endif
    default: return {};
    }

    return {};

#endif
}