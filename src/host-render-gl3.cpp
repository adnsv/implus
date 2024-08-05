#include "host-render.hpp"
#include <stdexcept>

#ifdef IMPLUS_GL_LOADER_GLAD
#include <glad/glad.h>
#endif

#if defined(IMPLUS_HOST_GLFW)
#include <GLFW/glfw3.h> // glfw3 must be included after glad!

#include <backends/imgui_impl_glfw.h>

#elif defined(IMPLUS_HOST_SDL2)
#include <SDL.h>
#include <backends/imgui_impl_sdl2.h>

#elif defined(IMPLUS_HOST_SDL3)
#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>

#endif

#include <backends/imgui_impl_opengl3.h>

namespace Renderer {

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

#if defined(IMPLUS_HOST_GLFW)
#elif defined(IMPLUS_HOST_SDL2) || defined(IMPLUS_HOST_SDL3)
static SDL_GLContext gl_context_ = nullptr;
#endif

void SetupHints()
{

#if defined(IMPLUS_HOST_GLFW)

#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GL_TRUE);

#elif defined(__linux__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+
#endif

#elif defined(IMPLUS_HOST_SDL2) || defined(IMPLUS_HOST_SDL2)

#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#elif defined(__ANDROID__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

#elif defined(__EMSCRIPTEN__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

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
#endif
}

void SetupInstance(ImPlus::Host::Window& wnd) {}

void SetupWindow(ImPlus::Host::Window& wnd)
{
#if defined(IMPLUS_HOST_GLFW)
    auto window = static_cast<GLFWwindow*>(wnd.Handle());
    glfwMakeContextCurrent(window);
    if (!glad_inited) {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error{"GLAD initialization failure"};
        glad_inited = true;
    }
    glfwSwapInterval(1);

#elif defined(IMPLUS_HOST_SDL2) || defined(IMPLUS_HOST_SDL3)
    auto window = static_cast<SDL_Window*>(wnd.Handle());
    gl_context_ = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context_);

    if (!glad_inited) {
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
#if defined(IMPLUS_HOST_SDL2)
            SDL_GL_DeleteContext(gl_context_);
#elif defined(IMPLUS_HOST_SDL2)
            SDL_GL_DestroyContext(gl_context_);
#endif
            gl_context_ = nullptr;
            throw std::runtime_error{"GLAD initialization failure"};
        }
        glad_inited = true;
    }

    SDL_GL_SetSwapInterval(1);
#endif
}

void SetupImplementation(ImPlus::Host::Window& wnd)
{
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
        throw std::runtime_error("Failed to init OpenGL renderer");

#if defined(IMPLUS_HOST_GLFW)
    auto window = static_cast<GLFWwindow*>(wnd.Handle());
    ImGui_ImplGlfw_InitForOpenGL(window, true);

#elif defined(IMPLUS_HOST_SDL2)
    auto window = static_cast<SDL_Window*>(wnd.Handle());
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context_);

#elif defined(IMPLUS_HOST_SDL3)
    auto window = static_cast<SDL_Window*>(wnd.Handle());
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context_);
#endif
}

void ShutdownImplementation() { ImGui_ImplOpenGL3_Shutdown(); }

void ShutdownInstance()
{
#if defined(IMPLUS_HOST_SDL2)
    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }
#endif
#if defined(IMPLUS_HOST_SDL3)
    if (gl_context_) {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }
#endif
}

void InvalidateDeviceObjects()
{
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();
}

void NewFrame(ImPlus::Host::Window&) { ImGui_ImplOpenGL3_NewFrame(); }

void PrepareViewport(ImPlus::Host::Window& wnd)
{
    auto const fbsize = wnd.FramebufferSize();
    glViewport(0, 0, fbsize.w, fbsize.h);
    glClearColor(wnd.Background.x, wnd.Background.y, wnd.Background.z, wnd.Background.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderDrawData() { ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); }

void SwapBuffers(ImPlus::Host::Window& wnd)
{
#if defined(IMPLUS_HOST_GLFW)
    auto window = static_cast<GLFWwindow*>(wnd.Handle());
    glfwSwapBuffers(window);
#elif defined(IMPLUS_HOST_SDL2) || defined(IMPLUS_HOST_SDL3)
    auto window = static_cast<SDL_Window*>(wnd.Handle());
    SDL_GL_SwapWindow(window);
#endif
}

} // namespace Renderer