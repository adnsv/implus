#include <imgui_internal.h>

#include "implus/commands.hpp"

namespace ImPlus::Command {

void List::ProcessAccelerators()
{
    // todo: wait until ImGui's shortcut routing matures.
    auto const mods = GImGui->IO.KeyMods;
    Entry* hit = nullptr;
    for (auto&& kv : (*this)) {
        if (hit)
            break;
        for (auto kc : kv.second.Accelerators) {
            if (kc & ImGuiMod_Shortcut)
                kc = ImGui::FixupKeyChord(kc);
        
            auto m = ImGuiKeyChord(kc & ImGuiMod_Mask_);
            if (m != mods)
                continue;

            auto k = ImGuiKey(kc & ~ImGuiMod_Mask_);
            if (k == ImGuiKey_None)
                k = ConvertSingleModFlagToKey(mods);

            if (ImGui::IsKeyPressed(k)) {
                hit = &kv.second;
                break;
            }
        }
    }

    if (hit && hit->OnExecute && hit->Enabled.value())
        hit->OnExecute();
}

} // namespace ImPlus::Command