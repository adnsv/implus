#
#
# IMGUI_HOST_IMPL host platform backend
#
#   - GLFW
#   - WIN32
#   - SDL2
#   - SDL3
#
#
# IMGUI_RENDER_IMPL renderer backend
#
#   - GL3
#   - VULKAN
#   - DX11
#
# IMGUI_ENABLE_FREETYPE
#
#   - ON force Freetype for font rendering
#   - OFF use built-in font rasterizer
#
#

if(!IMGUI_DIR)
    message(FATAL_ERROR "ImGui -- Missing IMGUI_DIR variable")
endif()

if(WIN32)
    # choose between WIN32/DIRECTX or GLFW/GL3
    set(IMGUI_HOST_IMPL "WIN32" CACHE STRING "ImGui host implementation")
    set_property(CACHE IMGUI_HOST_IMPL PROPERTY STRINGS "WIN32" "GLFW" "SDL2" "SDL3")
    if(IMGUI_HOST_IMPL STREQUAL "WIN32")
        set(IMGUI_DEFAULT_RENDER_IMPL "DX11")
    elseif(IMGUI_HOST_IMPL STREQUAL "GLFW")
        set(IMGUI_DEFAULT_RENDER_IMPL "GL3")
    elseif(IMGUI_HOST_IMPL STREQUAL "SDL2")
        set(IMGUI_DEFAULT_RENDER_IMPL "GL3")
    elseif(IMGUI_HOST_IMPL STREQUAL "SDL3")
        set(IMGUI_DEFAULT_RENDER_IMPL "GL3")
    else()
        message(FATAL_ERROR "Unsupported ImGui host implementation")
    endif()

elseif(ANDROID)
    set(IMGUI_HOST_IMPL "SDL2" CACHE STRING "ImGui host implementation")
    set_property(CACHE IMGUI_HOST_IMPL PROPERTY STRINGS "SDL2" "SDL3")
    
    set(IMGUI_DEFAULT_RENDER_IMPL "GL3")

else()
    # choose between GLFW or SDL2 or SDL3, default to GLFW
    set(IMGUI_HOST_IMPL "GLFW" CACHE STRING "ImGui host implementation")
    set_property(CACHE IMGUI_HOST_IMPL PROPERTY STRINGS "GLFW" "SDL2" "SDL3")
    set(IMGUI_DEFAULT_RENDER_IMPL "GL3")

endif()

set(IMGUI_RENDER_IMPL "${IMGUI_DEFAULT_RENDER_IMPL}" CACHE STRING "ImGui render implementation")
set_property(CACHE IMGUI_RENDER_IMPL PROPERTY STRINGS "DX11" "GL3" "VULKAN")

if(IMGUI_RENDER_IMPL STREQUAL "GL3")
    set(IMGUI_GL_LOADER_IMPL "GLAD" CACHE STRING "ImGui OpenGL loader implementation")
endif()

option(IMGUI_ENABLE_FREETYPE "Enable ImGui Freetype support" OFF)

add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
)

target_include_directories(imgui PUBLIC "${IMGUI_DIR}")

message(STATUS "ImGui -- Host implementation: ${IMGUI_HOST_IMPL}")
set(IMGUI_HOST_IMPL "${IMGUI_HOST_IMPL}" PARENT_SCOPE)

if(IMGUI_HOST_IMPL STREQUAL "GLFW")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp")
    message(STATUS "ImGui -- Linking with glfw target")
    target_link_libraries(imgui PUBLIC "glfw")

elseif(IMGUI_HOST_IMPL STREQUAL "SDL2")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp")
    message(STATUS "ImGui -- Linking with SDL2 target")
    target_link_libraries(imgui PUBLIC "SDL2::SDL2")

elseif(IMGUI_HOST_IMPL STREQUAL "SDL3")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp")
    message(STATUS "ImGui -- Linking with SDL3 target")
    target_link_libraries(imgui PUBLIC "SDL3::SDL3")

elseif(IMGUI_HOST_IMPL STREQUAL "WIN32")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_win32.cpp")

endif()
   
message(STATUS "ImGui -- Render implementation: ${IMGUI_RENDER_IMPL}")
set(IMGUI_RENDER_IMPL "${IMGUI_RENDER_IMPL}" PARENT_SCOPE)

if(IMGUI_RENDER_IMPL STREQUAL "GL3")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
    set(IMGUI_USE_GL_LOADER ON)

elseif(IMGUI_RENDER_IMPL STREQUAL "VULKAN")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp")

elseif(IMGUI_RENDER_IMPL STREQUAL "DX11")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp")
    target_link_libraries(imgui PUBLIC "d3d11" "d3dcompiler" "dxgi" "shcore" "xinput")

elseif(IMGUI_RENDER_IMPL STREQUAL "DX12")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp")
    target_link_libraries(imgui PUBLIC "d3d12" "d3dcompiler" "dxgi" "shcore" "xinput")
    
endif()

set(IMGUI_ENABLE_FREETYPE "${IMGUI_ENABLE_FREETYPE}" PARENT_SCOPE)
if(IMGUI_ENABLE_FREETYPE)
    message(STATUS "ImGui -- Building with Freetype font rendering engine")
    target_sources(imgui PUBLIC "${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp")
    target_include_directories(imgui PUBLIC "${IMGUI_DIR}/misc/freetype")
    target_compile_definitions(imgui PUBLIC "IMGUI_ENABLE_FREETYPE")
    if(EMSCRIPTEN) 
        message(STATUS "ImGui -- assuming Freetype is linked from Emscripten ports")
    else()
        target_link_libraries(imgui PUBLIC "Freetype::Freetype")
    endif()
else()
    message(STATUS "ImGui -- Building with stb_truetype font rendering engine")
endif()

if(IMGUI_RENDER_IMPL STREQUAL "GL3")
    set(IMGUI_GL_LOADER_IMPL "${IMGUI_GL_LOADER_IMPL}" PARENT_SCOPE)
    set(IMGUI_GL_LOADER_MACRO "IMGUI_IMPL_OPENGL_LOADER_${IMGUI_GL_LOADER_IMPL}")

    message(STATUS "ImGui -- OpenGL loader: ${IMGUI_GL_LOADER_IMPL}")
    message(STATUS "ImGui -- IMGUI_GL_LOADER_MACRO = ${IMGUI_GL_LOADER_MACRO}")

    target_compile_definitions(imgui PUBLIC "${IMGUI_GL_LOADER_MACRO}")

    if(IMGUI_GL_LOADER_IMPL STREQUAL "GLAD") 
        message(STATUS "ImGui -- Linking with glad target")
        target_link_libraries(imgui PUBLIC "glad")
    endif()

    if(IMGUI_GL_FLAVOR STREQUAL "GLESv3")
        message(STATUS "ImGui -- Linking with EGL/GLESv3")

        find_path(GLES_INCLUDE_DIR GLES3/gl3.h HINTS ${ANDROID_NDK})
        find_library(GLES_LIBRARY libGLESv3.so HINTS ${GLES_INCLUDE_DIR}/../lib)

        target_link_libraries(imgui PUBLIC
                ${GLES_LIBRARY}
                "EGL"
                #"GLESv3"
        )
    endif()

endif()

if(IMGUI_RENDER_IMPL STREQUAL "VULKAN")
    find_package(Vulkan REQUIRED)
    message(STATUS "ImGui -- Using Vulkan renderer")
    target_link_libraries(imgui PUBLIC "Vulkan::Vulkan")
endif()