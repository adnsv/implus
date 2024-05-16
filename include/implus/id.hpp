#pragma once

#include <imgui.h>
#include <string_view>

namespace ImPlus {

struct ImID {
    ImID(ImID const&) = default;
    ImID(ImGuiID v)
        : id_{v}
    {
    }
    ImID(char const* str_id)
        : id_{ImGui::GetID(str_id)}
    {
    }
    ImID(std::string_view sv)
        : id_{ImGui::GetID(sv.data(), sv.data() + sv.size())}
    {
    }
    auto value() const { return id_; }
    operator ImGuiID() const { return id_; }

protected:
    ImGuiID id_ = 0;
};

struct ImIDMaker {
    ImIDMaker(ImID v);
    ~ImIDMaker();
    auto operator()() -> ImID;

protected:
    int n_ = 0;
    void* w_ = nullptr;
};

} // namespace ImPlus