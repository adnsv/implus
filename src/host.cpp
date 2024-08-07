#include <imgui_internal.h>

#include <implus/host.hpp>
#include <implus/render-device.hpp>

#include <algorithm>
#include <codecvt>

namespace ImPlus::Host {

void Window::SetFullscreen(bool on)
{
    if (on == IsFullscreen())
        return;

    Locate(Location{.FullScreen = on});
}

void Window::Locate(Location const& loc, bool constrain_to_monitor)
{
    if (WithinFrame()) {
        pending_locate_ = loc;
        pending_constrain_ = constrain_to_monitor;
    }
    else {
        pending_locate_.reset();
        perform_locate(loc, constrain_to_monitor);
    }
}

auto AllowRefreshWithinFrameScope = false;

auto WithinFrame() -> bool
{
    auto g = ImGui::GetCurrentContext();
    return g && g->WithinFrameScope;
}

auto AllowRefresh() -> bool
{
    return AllowRefreshWithinFrameScope || !WithinFrame();
}

Window* main_window_ = nullptr;
std::size_t window_counter_ = 0;

auto MainWindow() -> Window&
{
    assert(main_window_);
    return *main_window_;
};

} // namespace ImPlus::Host