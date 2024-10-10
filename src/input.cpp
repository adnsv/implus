#include <imgui_internal.h>

#include "implus/accelerator.hpp"
#include "implus/balloontip.hpp"
#include "implus/dlg.hpp"
#include "implus/input.hpp"
#include <clocale>

namespace ImPlus {

void internal::make_localized_decimal(ImGuiInputTextFlags& f)
{
    f = f | ImGuiInputTextFlags_LocalizeDecimalPoint;
}

void internal::disable_mark_edited(ImGuiInputTextFlags& f)
{
    f = f | ImGuiInputTextFlags_NoMarkEdited;
}

void internal::mark_last_item_edited() { ImGui::MarkItemEdited(ImGui::GetItemID()); }

void internal::reload_input_text_buffer()
{
    if (ImGui::IsItemActive()) {
        auto ts = ImGui::GetInputTextState(ImGui::GetItemID());
        if (ts)
            ts->ReloadUserBufAndSelectAll();
    }
}

std::optional<char> cached_user_decimal_char = {};

auto internal::user_decimal_char() -> char
{
    if (!cached_user_decimal_char) {
        // a bit simplified, but should work in most cases
        auto c = char('.');
        auto prev = std::setlocale(LC_NUMERIC, "");
        auto ld = std::localeconv();
        if (ld && ld->decimal_point && ld->decimal_point[0] == ',')
            c = ',';
        std::setlocale(LC_NUMERIC, prev);
        cached_user_decimal_char = c;
    }
    return *cached_user_decimal_char;
}

struct callback_data {
    std::string& str;
};

static auto callback(ImGuiInputTextCallbackData* data) -> int
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto& str = reinterpret_cast<callback_data*>(data->UserData)->str;
        str.resize(data->BufTextLen);
        data->Buf = str.data();
    }
    return 0;
}

auto InputTextMultiline(
    ImID id, std::string& str, ImVec2 const& size_arg, ImGuiInputTextFlags flags) -> bool
{
    flags |= ImGuiInputTextFlags_CallbackResize;
    auto data = callback_data{str};

    ImGui::PushID(id);
    auto const r = ImGui::InputTextMultiline(
        "", const_cast<char*>(str.data()), str.capacity() + 1, size_arg, flags, callback, &data);
    auto const bb = GImGui->LastItemData.Rect;
    Dlg::HandleInputField();
    ImGui::PopID();
    BalloonTip(id, bb.Min, bb.Max);
    return r;
}

#if 0
auto handleContextPopup(ImGuiID input_id, ImGuiInputTextFlags flags)
{
    auto const is_readonly = (flags & ImGuiInputTextFlags_ReadOnly) != 0;
    auto const is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;

    auto state = ImGui::GetInputTextState(input_id);
    auto has_sel = state->HasSelection();

    static auto const sc_undo = ImPlus::KeyChordToString(ImGuiMod_Shortcut | ImGuiKey_Z);
    static auto const sc_redo = ImPlus::KeyChordToString(ImGuiMod_Shortcut | ImGuiKey_Y);
    static auto const sc_cut = ImPlus::KeyChordToString(ImGuiMod_Shortcut | ImGuiKey_X);
    static auto const sc_copy = ImPlus::KeyChordToString(ImGuiMod_Shortcut | ImGuiKey_C);
    static auto const sc_paste = ImPlus::KeyChordToString(ImGuiMod_Shortcut | ImGuiKey_V);
    static auto const sc_del = ImPlus::KeyChordToString(ImGuiKey_Delete);
    static auto const sc_all = ImPlus::KeyChordToString(ImGuiMod_Shortcut | ImGuiKey_A);

    char const* clipboard_text = is_readonly ? nullptr : ImGui::GetClipboardText();

    ImGui::MenuItem("Undo", sc_undo.c_str(), false, !is_readonly && state->GetUndoAvailCount() > 0);
    ImGui::Separator();
    ImGui::MenuItem("Cut", sc_cut.c_str(), false, has_sel && !is_readonly);
    ImGui::MenuItem("Copy", sc_copy.c_str(), false, has_sel);
    ImGui::MenuItem("Paste", sc_paste.c_str(), false, clipboard_text != nullptr);
    ImGui::MenuItem("Delete", sc_del.c_str(), false, has_sel && !is_readonly);
    ImGui::Separator();
    ImGui::MenuItem("Select All", sc_all.c_str(), false,
        state->CurLenW &&
            (state->GetSelectionStart() != 0 || state->GetSelectionEnd() != state->CurLenW));
}
#endif

auto InputTextWithHint(ImID id, const char* hint, std::string& str, ImVec2 const& size_arg,
    ImGuiInputTextFlags flags) -> bool
{
    flags |= ImGuiInputTextFlags_CallbackResize;
    auto data = callback_data{str};
    ImGui::PushID(id);
    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline));
    auto const r = ImGui::InputTextEx("", hint, const_cast<char*>(str.data()), str.capacity() + 1,
        size_arg, flags, callback, &data);
    auto const bb = GImGui->LastItemData.Rect;
    Dlg::HandleInputField();
    ImGui::PopID();
    BalloonTip(id, bb.Min, bb.Max);
    return r;
}

auto InputTextWithHint(
    ImID id, const char* hint, std::string& str, ImGuiInputTextFlags flags) -> bool
{
    return InputTextWithHint(id, hint, str, {}, flags);
}

auto InputText(ImID id, std::string& str, ImGuiInputTextFlags flags) -> bool
{
    return InputTextWithHint(id, nullptr, str, flags);
}

auto InputTextBuffered(
    ImID id, char const* hint, char* buf, std::size_t buf_size, ImGuiInputTextFlags flags) -> bool
{
    ImGui::PushID(id);
    auto const r = ImGui::InputTextWithHint("", hint, buf, buf_size, flags);
    auto const bb = GImGui->LastItemData.Rect;
    Dlg::HandleInputField();
    ImGui::PopID();
    BalloonTip(id, bb.Min, bb.Max);
    return r;
}

} // namespace ImPlus