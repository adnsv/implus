cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(implus)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/imgui.cmake")

# from ImGui configuration
set(IMPLUS_HOST_IMPL "${IMGUI_HOST_IMPL}")
set(IMPLUS_HOST_IMPL "${IMPLUS_HOST_IMPL}" PARENT_SCOPE)
set(IMPLUS_RENDER_IMPL "${IMGUI_RENDER_IMPL}")
set(IMPLUS_RENDER_IMPL "${IMPLUS_RENDER_IMPL}" PARENT_SCOPE)
set(IMPLUS_USE_FREETYPE "${IMGUI_ENABLE_FREETYPE}")
set(IMPLUS_USE_FREETYPE "${IMPLUS_USE_FREETYPE}" PARENT_SCOPE)

if(IMGUI_GL_LOADER_IMPL STREQUAL "GLAD")
    add_subdirectory("glad")
endif()

if(IMGUI_GL_LOADER_IMPL)
    set(IMPLUS_GL_LOADER_IMPL "${IMGUI_GL_LOADER_IMPL}")
    set(IMPLUS_GL_LOADER_IMPL "${IMPLUS_GL_LOADER_IMPL}" PARENT_SCOPE)
    set(IMPLUS_GL_LOADER_MACRO "IMPLUS_GL_LOADER_${IMPLUS_GL_LOADER_IMPL}")
endif()

add_subdirectory(lbrk)

add_library(implus STATIC
    "src/accelerator.cpp"
    "src/application.cpp"
    "src/arithmetics.cpp"
    "src/balloontip.cpp"
    "src/badge.cpp"
    "src/banner.cpp"
    "src/blocks.cpp"
    "src/button.cpp"
    "src/buttonbar.cpp"
    "src/checkbox.cpp"
    "src/color.cpp"
    "src/combo.cpp"
    "src/commands.cpp"
    "src/dbgw.cpp"
    "src/dlg.cpp"
    "src/dlg-common.cpp"
    "src/dropdown.cpp"
    "src/flow.cpp"
    "src/font-common.cpp"
    "src/host.cpp"
    "src/host-native.cpp"
    "src/icon.cpp"
    "src/id.cpp"
    "src/input.cpp"
    "src/interact.cpp"
    "src/internal/draw-utils.cpp"
    "src/internal/font-engine.cpp"
    "src/internal/split-label.cpp"
    "src/length.cpp"
    "src/listbox.cpp"
    "src/menu.cpp"
    "src/navigation.cpp"
    "src/pane.cpp"
    "src/panel.cpp"
    "src/pathbox.cpp"
    "src/placement.cpp"
    "src/selbox.cpp"
    "src/sizing.cpp"
    "src/splitter.cpp"
    "src/text.cpp"
    "src/toggler.cpp"
    "src/toolbar.cpp"
    "src/tooltip.cpp"
    "src/visuals.cpp"
)

target_compile_features(implus PUBLIC cxx_std_20)
if(WIN32)
    target_compile_definitions(implus PUBLIC "-DNOMINMAX")
endif()

target_link_libraries(implus PUBLIC "imgui" "implus::lbrk")

set(IMPLUS_HOST_MACRO "IMPLUS_HOST_${IMPLUS_HOST_IMPL}")
set(IMPLUS_RENDER_MACRO "IMPLUS_RENDER_${IMGUI_RENDER_IMPL}")

target_compile_definitions(implus PUBLIC
    "${IMPLUS_HOST_MACRO}"
    "${IMPLUS_RENDER_MACRO}"
    "${IMPLUS_GL_LOADER_MACRO}"
)

message(STATUS "ImPlus -- Host: ${IMPLUS_HOST_IMPL}")

if(IMPLUS_HOST_IMPL STREQUAL "GLFW")
    target_sources(implus PUBLIC "src/host-backend-glfw.cpp")
    target_link_libraries(implus PUBLIC "glfw")

    if(GLFW_BUILD_WIN32)
        message(STATUS "ImPlus -- with support for WIN32")
        target_compile_definitions(implus PUBLIC "GLFW_BACKEND_SUPPORTS_WIN32")
        target_compile_definitions(implus PUBLIC "IMPLUS_HOST_GLFW_WIN32")
        set(has_glfw_impl TRUE)
    endif()

    if(GLFW_BUILD_COCOA)
        message(STATUS "ImPlus -- with support for COCOA")
        target_compile_definitions(implus PUBLIC "GLFW_BACKEND_SUPPORTS_COCOA")
        target_compile_definitions(implus PUBLIC "IMPLUS_HOST_GLFW_COCOA")
        set(has_glfw_impl TRUE)
    endif()

    if(GLFW_BUILD_X11)
        message(STATUS "ImPlus -- with support for X11")
        target_compile_definitions(implus PUBLIC "GLFW_BACKEND_SUPPORTS_X11")
        target_compile_definitions(implus PUBLIC "IMPLUS_HOST_GLFW_X11")
        set(has_glfw_impl TRUE)
    endif()

    if(GLFW_BUILD_WAYLAND)
        message(STATUS "ImPlus -- with support for WAYLAND")
        target_compile_definitions(implus PUBLIC "GLFW_BACKEND_SUPPORTS_WAYLAND")
        target_compile_definitions(implus PUBLIC "IMPLUS_HOST_GLFW_WAYLAND")
        set(has_glfw_impl TRUE)
    endif()

    if(!has_glfw_impl)
        message(FATAL_ERROR "GLFW implementation is missing or unknown")
    endif()

elseif(IMPLUS_HOST_IMPL STREQUAL "SDL2")
    target_sources(implus PUBLIC "src/host-backend-sdl2.cpp")
    target_link_libraries(implus PUBLIC "SDL2::SDL2")

elseif(IMPLUS_HOST_IMPL STREQUAL "SDL3")
    target_sources(implus PUBLIC "src/host-backend-sdl3.cpp")
    target_link_libraries(implus PUBLIC "SDL3::SDL3")

elseif(IMPLUS_HOST_IMPL STREQUAL "WIN32")
    target_sources(implus PUBLIC "src/host-backend-win32.cpp")
    target_compile_definitions(implus PUBLIC "IMPLUS_HOST_NATIVE_WIN32")

endif()

if (IMPLUS_RENDER_IMPL STREQUAL "DX11")
    target_sources(implus PUBLIC "src/host-render-dx11.cpp")
elseif(IMPLUS_RENDER_IMPL STREQUAL "GL3")
    target_sources(implus PUBLIC "src/host-render-gl3.cpp")
elseif(IMPLUS_RENDER_IMPL STREQUAL "VULKAN")
    target_sources(implus PUBLIC "src/host-render-vulkan.cpp")
endif()



if(WIN32)
    set(IMPLUS_DEFAULT_FONT_WIN32 ON)

elseif(EMSCRIPTEN)
    # using fontconfig stub

elseif(ANDROID)
    # using fontconfig stub

elseif(APPLE)
    set(IMPLUS_USE_FONTCONFIG ON)
    set(IMPLUS_DEFAULT_FONT_COCOA ON)

elseif(UNIX)
    set(IMPLUS_USE_FONTCONFIG ON)
    set(IMPLUS_DEFAULT_FONT_GNOME ON) # use gsettings: org.gnome.desktop.interface font-name

else()
    message(STATUS "ImPlus -- Default font stub")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_definitions(implus PUBLIC IMPLUS_APPLE_CLANG_RANGES)
endif()

if(IMPLUS_USE_FONTCONFIG)
    message(STATUS "ImPlus -- Building with fontconfig")
    find_package(Fontconfig REQUIRED)
    target_link_libraries(implus PUBLIC Fontconfig::Fontconfig)
    target_compile_definitions(implus PUBLIC "IMPLUS_USE_FONTCONFIG")
    target_sources(implus PUBLIC "src/font-fontconfig.cpp")
endif()

if(IMPLUS_DEFAULT_FONT_WIN32)
    message(STATUS "ImPlus -- Default font origin: Windows GDI")
    target_sources(implus PRIVATE "src/font-wingdi.cpp")
    target_compile_definitions(implus PUBLIC IMPLUS_DEFAULT_FONT_WIN32)

elseif(IMPLUS_DEFAULT_FONT_GNOME)
    message(STATUS "ImPlus -- Default font origin: Gnome Desktop Settings")
    target_sources(implus PRIVATE "src/font-gnome.cpp")
    target_compile_definitions(implus PUBLIC IMPLUS_DEFAULT_FONT_GNOME)

elseif(IMPLUS_DEFAULT_FONT_COCOA)
    message(STATUS "ImPlus -- Default font origin: Cocoa")
    target_sources(implus PRIVATE "src/font-apple.mm")
    target_compile_definitions(implus PUBLIC IMPLUS_DEFAULT_FONT_COCOA)

else()
    message(STATUS "ImPlus -- Default font stub")
    target_sources(implus PRIVATE "src/font-stub.cpp")
    target_compile_definitions(implus PUBLIC IMPLUS_DEFAULT_FONT_STUB)

endif()

option(IMPLUS_REPLACE_SEGOE_WITH_CALIBRI "Replace autodetected Segoe Font with Calibri" OFF)

if(IMPLUS_REPLACE_SEGOE_WITH_CALIBRI)
    target_compile_definitions(implus PUBLIC "IMPLUS_REPLACE_SEGOE_WITH_CALIBRI")
endif(IMPLUS_REPLACE_SEGOE_WITH_CALIBRI)

target_include_directories(implus
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(IMPLUS_USE_FREETYPE)
    target_compile_definitions(implus PUBLIC "IMPLUS_USE_FREETYPE")

    # target_link_libraries(implus PUBLIC "Freetype::Freetype")
endif()

if(MSVC)
    # disable C4996 warning for getenv
    target_compile_definitions(implus PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()

# support for debug/demo windows
option(IMPLUS_ENABLE_IMGUI_METRICS "Enable support for IMGUI demo")
option(IMPLUS_ENABLE_IMGUI_DEMO "Enable support for IMGUI demo")
option(IMPLUS_ENABLE_IMPLUS_DEMO "Enable support for IMPLUS demo")

if(IMPLUS_ENABLE_IMGUI_METRICS)
    message(STATUS "ImPlus -- Enable ImGui metrics support")
    target_compile_definitions(implus PUBLIC "ENABLE_IMGUI_METRICS")
endif()

if(IMPLUS_ENABLE_IMGUI_DEMO)
    message(STATUS "ImPlus -- Enable ImGui demo support")
    target_compile_definitions(implus PUBLIC "ENABLE_IMGUI_DEMO")
endif()

if(IMPLUS_ENABLE_IMPLUS_DEMO)
    message(STATUS "ImPlus -- Enable ImPlus demo support")
    target_compile_definitions(implus PUBLIC "ENABLE_IMPLUS_DEMO")
endif()