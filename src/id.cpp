#include <imgui_internal.h>

#include "implus/id.hpp"

namespace ImPlus {

ImIDMaker::ImIDMaker(ImID v)
    : w_{ImGui::GetCurrentWindow()}
{
    auto window = static_cast<ImGuiWindow*>(w_);
    window->IDStack.push_back(v);
}

ImIDMaker::~ImIDMaker()
{
    auto window = static_cast<ImGuiWindow*>(w_);
    IM_ASSERT(window->IDStack.Size > 1);
    window->IDStack.pop_back();
}
auto ImIDMaker::operator()() -> ImID
{
    auto window = static_cast<ImGuiWindow*>(w_);
    return window->GetID(n_++);
}

} // namespace ImPlus