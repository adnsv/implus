#pragma once

namespace ImPlus::DBGW {

// debug and demo windows

auto PopulateMenuItems(bool wantSeparatorBefore = false) -> bool;

void Display();

struct wnd {
    auto Enabled() const -> bool;
    auto Visible() const -> bool;
    void Show();
    void Hide();
    void Toggle();

    wnd(bool* showFlag)
        : flag_{showFlag}
    {
    }

protected:
    bool* flag_ = nullptr;
};

extern wnd ImGuiMetrics;
extern wnd ImGuiDemo;
extern wnd ImPlusDemo;

} // namespace ImPlus::DBGW