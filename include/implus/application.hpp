#pragma once

#include "host.hpp"
#include "visuals.hpp"

#include <span>
#include <stdexcept>
#include <concepts>
#include <functional>
#include <vector>

namespace ImPlus::Application {

struct Base {
public:
    Base(Host::Window::InitLocation const& loc, char const* title,
        Host::Window::Attrib attr = Host::Window::Attrib::Resizable |
                                    Host::Window::Attrib::Decorated);
    Base(char const* title,
        Host::Window::Attrib attr = Host::Window::Attrib::Resizable |
                                    Host::Window::Attrib::Decorated);
    virtual ~Base();

    void Initialize();
    void Run(std::function<void()> on_render);

protected:
    Host::Window main_wnd_; // use ImPlus::Host::MainWindow()
    void ConfigureIO();
    void InitTheme();
};

inline Base::Base(Host::Window::InitLocation const& loc, char const* title,
    Host::Window::Attrib attr)
    : main_wnd_{loc, title, attr}
{
    if (!main_wnd_)
        throw std::runtime_error("failed to create main window");
}

inline Base::Base(char const* title, Host::Window::Attrib attr)
    : Base{Host::Window::Size{}, title, attr}
{
}

inline Base::~Base() {}

inline void Base::Initialize()
{
    ConfigureIO();
    InitTheme();
}

inline void Base::ConfigureIO()
{
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = nullptr;
}

inline void Base::InitTheme()
{
    Visuals::Theme::Setup(ImPlus::Visuals::Theme::Dark);
}

namespace Callbacks {
    using collection_type = std::vector<std::function<void()>>;
    inline collection_type BeforeFrame;
    inline collection_type RenderFrame;
    inline collection_type AfterEachFrame; // execute after each frame
    inline collection_type AfterThisFrame; // execute once after the current frame
}

} // namespace ImPlus::Application