#include "implus/dbgw.hpp"

#include <imgui.h>

#ifdef ENABLE_IMGUI_METRICS
auto showImGuiMetrics = false;
#endif
#ifdef ENABLE_IMGUI_DEMO
auto showImGuiDemo = false;
#endif
#ifdef ENABLE_IMPLUS_DEMO
#include "implus/demo/demo.hpp"
ImPlus::Demo::Window implusDemoWindow;
auto showImPlusDemo = false;
#endif

namespace ImPlus::DBGW {

auto wnd::Enabled() const -> bool { return flag_ != nullptr; }
auto wnd::Visible() const -> bool { return flag_ && *flag_; }
void wnd::Show()
{
    if (flag_)
        *flag_ = true;
}
void wnd::Hide()
{
    if (flag_)
        *flag_ = false;
}
void wnd::Toggle()
{
    if (flag_)
        *flag_ = !*flag_;
}

wnd ImGuiMetrics = {
#ifdef ENABLE_IMGUI_METRICS
    &showImGuiMetrics
#else
    nullptr
#endif
};
wnd ImGuiDemo = {
#ifdef ENABLE_IMGUI_DEMO
    &showImGuiDemo
#else
    nullptr
#endif
};
wnd ImPlusDemo = {
#ifdef ENABLE_IMPLUS_DEMO
    &showImPlusDemo
#else
    nullptr
#endif
};

auto PopulateMenuItems(bool wantSeparatorBefore) -> bool
{
    auto ret = false;

    auto handle = [&](wnd& w, char const* label) {
        if (!w.Enabled())
            return;
        ret = true;
        if (wantSeparatorBefore) {
            wantSeparatorBefore = false;
            ImGui::Separator();
        }
        if (ImGui::MenuItem(label, "", w.Visible()))
            w.Toggle();
    };

    handle(ImGuiMetrics, "ImGui Metrics##dbgw-imgui-metrics");
    handle(ImGuiDemo, "ImGui Demo##dbgw-imgui-demo");
    handle(ImPlusDemo, "ImPlus Demo##dbgw-implus-demo");
    return ret;
}

void Display()
{
#ifdef ENABLE_IMGUI_METRICS
    if (showImGuiMetrics)
        ImGui::ShowMetricsWindow(&showImGuiMetrics);
#endif
#ifdef ENABLE_IMGUI_DEMO
    if (showImGuiDemo)
        ImGui::ShowDemoWindow(&showImGuiDemo);
#endif

#ifdef ENABLE_IMPLUS_DEMO
    if (showImPlusDemo)
        implusDemoWindow.Display(&showImPlusDemo);
#endif
}

} // namespace ImPlus::DBGW