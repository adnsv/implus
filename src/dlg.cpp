#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/application.hpp"
#include "implus/dlg.hpp"

#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include <unordered_map>

namespace ImPlus::Dlg {

enum child_area_state {
    child_window_pending,
    child_group_pending,
    child_window_started,
    child_group_started,
    child_done,
};

static inline auto is_pending(child_area_state v)
{
    return v == child_window_pending || v == child_group_pending;
}

static inline auto is_started(child_area_state v)
{
    return v == child_window_started || v == child_group_started;
}

struct display_info {
    ImGuiID WindowID;
    Flags flags;
    ImVec2 area_extra = {0, 0};
    float padding_extra_bot = 0;
    ImVec2 button_size = {0, 0};
    float button_area_spacing = 0;
    bool autosizing = false;
    child_area_state child_state = child_group_pending;
    bool close_action_pending = false;
    bool default_action_allowed = true;
};

std::vector<display_info> display_stack;

auto BeginContentArea() -> bool
{
    if (display_stack.empty())
        return false;
    auto& info = display_stack.back();
    if (info.child_state == child_window_pending) {
        auto area_flags = ImGuiWindowFlags_NavFlattened;

        ImGui::BeginChild("##.DLG.CHILD.AREA.", {-info.area_extra.x, -info.area_extra.y},
            ImGuiChildFlags_None, area_flags);

        info.child_state = child_window_started;
        return true;
    }
    else if (info.child_state == child_group_pending) {
        ImGui::BeginGroup();
        info.child_state = child_group_started;
        return true;
    }
    return false;
}

auto get_shortcut_owner(ImGuiKeyChord kc) -> ImGuiID
{
    auto d = ImGui::GetShortcutRoutingData(kc);
    return d ? d->RoutingCurr : ImGuiKeyOwner_NoOwner;
}

void DoneContentArea()
{
    if (display_stack.empty())
        return;
    auto& info = display_stack.back();

    if (info.child_state == child_done)
        return;

    auto const want_buttons = (info.flags & Flags::NoButtonArea) == Flags{};
    auto const vertical = (info.flags & Flags::VerticalButtonArea) != Flags{};

    if (info.child_state == child_window_started) {
        ImGui::EndChild();
        if (vertical) {
            ImGui::SameLine();
        }
        // ImGui::GetCurrentWindow()->DC.CursorPos += info.button_area_offset;
    }
    else if (info.child_state == child_group_started) {
        ImGui::EndGroup();
        if (vertical) {
            ImGui::SameLine();
        }
        // ImGui::GetCurrentWindow()->DC.CursorPos += info.button_area_offset;
    }

    if (want_buttons) {
        auto pos = ImGui::GetCursorScreenPos();
        auto br = ImGui::GetContentRegionMaxAbs();
        if (vertical) {
            pos.x = pos.x - ImGui::GetStyle().ItemSpacing.x + info.button_area_spacing;
            pos.x = std::max(pos.x, br.x - info.button_size.x);
        }
        else {
            pos.y = pos.y - ImGui::GetStyle().ItemSpacing.y + info.button_area_spacing;
            pos.y = std::max(pos.y, br.y - info.button_size.y - info.padding_extra_bot);
        }
        ImGui::SetCursorScreenPos(pos);
    }

    info.child_state = child_done;
}

auto internal::CurrentFlags() -> Flags
{
    return display_stack.empty() ? Flags{} : display_stack.back().flags;
}
auto internal::GetButtonSize() -> ImVec2
{
    return display_stack.empty() ? ImVec2{} : display_stack.back().button_size;
}

// IsCancelShortcut detects shortcut chords (ImGuiKey_Escape | ImGuiKey_NavGamepadCancel).
auto IsCancelShortcut(ImGuiID owner_id) -> bool
{
    auto& io = GImGui->IO;
    const bool allow_gamepad = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 &&
                               (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;

    return ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_None, owner_id) ||
           (allow_gamepad &&
               ImGui::Shortcut(ImGuiKey_NavGamepadCancel, ImGuiInputFlags_None, owner_id));
}

auto IsRejectActionKeyPressed() -> bool
{
    if (!ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows | ImGuiFocusedFlags_NoPopupHierarchy))
        return false;
    auto& io = GImGui->IO;
    const bool allow_gamepad = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 &&
                               (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;

    return ImGui::IsKeyPressed(ImGuiKey_Escape) ||
           (allow_gamepad && ImGui::IsKeyPressed(ImGuiKey_NavGamepadCancel));
}

auto IsAcceptActionKeyPressed() -> bool
{
    // only current window with children, return false when popups are open and focused.
    if (!ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows | ImGuiFocusedFlags_NoPopupHierarchy))
        return false;

    auto const is_enter_pressed =
        ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
    return is_enter_pressed;
}

auto Begin(ImGuiID id, Options const& opts) -> bool
{
    auto& g = *GImGui;
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None)) {
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        return false;
    }

    auto const want_buttons = (opts.Flags & Flags::NoButtonArea) == Flags{};
    auto const vertical_buttons =
        (opts.Flags & Flags::VerticalButtonArea) == Flags::VerticalButtonArea;
    auto const resizable = (opts.Flags & Flags::Resizable) == Flags::Resizable;
    auto const flat_title = Style::Dialog::FlatTitle();

    auto button_size = ImVec2{0, 0};
    if (want_buttons) {
        if (vertical_buttons) {
            auto w = to_pt<rounded>(Style::Dialog::VerticalButtonWidth());
            button_size.x = std::max(1.0f, w);
        }
        else {
            auto h = to_pt<rounded>(Style::Dialog::HorizontalButtonHeight());
            if (!h)
                h = ImGui::GetFrameHeight();
            button_size.y = std::max(1.0f, *h);
        }
    }

    auto const button_area_spacing = to_pt<rounded>(0.5_em);

    auto const button_extra = ImVec2{
        button_size.x ? button_size.x + button_area_spacing : 0,
        button_size.y ? button_size.y + button_area_spacing : 0,
    };

    auto wnd_flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;

    if (!resizable)
        wnd_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

    auto const& viewport = *ImGui::GetMainViewport();

    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) == 0) {
        ImGui::SetNextWindowPos(viewport.GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    auto const padding_horz = to_pt<rounded>(Style::Dialog::ContentPadding());

    auto padding_top = padding_horz;
    auto padding_bottom = padding_horz;
    if (flat_title) {
        if (opts.Title.empty())
            padding_top = 0;
        else
            padding_top = std::round(padding_horz * 0.5);
    }

    auto const padding_vert = std::min(padding_top, padding_bottom);
    auto const padding_extra_bot = std::max(0.0f, padding_bottom - padding_vert);

    auto const title_bar_height = g.FontSize + g.Style.FramePadding.y * 2.0f;

    auto const extra_width = padding_horz * 2.0f + g.Style.FramePadding.x * 2.0f + button_extra.x;
    auto const extra_height = padding_vert * 2.0f + padding_extra_bot +
                              g.Style.FramePadding.y * 2.0f + title_bar_height + button_extra.y;

    auto min_outer_size = ImVec2{
        to_pt<rounded>(opts.MinimumContentWidth) + extra_width,
        to_pt<rounded>(opts.MinimumContentHeight) + extra_height,
    };

    auto const max_outer_size = ImVec2{
        std::max(min_outer_size.x, viewport.WorkSize.x),
        std::max(min_outer_size.y, viewport.WorkSize.y),
    };

    auto stretch_outer_w = to_pt<rounded>(opts.StretchContentWidth) + extra_width;
    if (stretch_outer_w > min_outer_size.x)
        min_outer_size.x = std::min(stretch_outer_w, max_outer_size.x);
    ImGui::SetNextWindowSizeConstraints(min_outer_size, max_outer_size);

    auto initial_size =
        ImVec2{to_pt<rounded>(opts.InitialWidth), to_pt<rounded>(opts.InitialHeight)};
    if (initial_size.x > 0 || initial_size.y > 0) {
        initial_size.x = std::max(min_outer_size.x, std::min(initial_size.x, max_outer_size.x));
        initial_size.y = std::max(min_outer_size.y, std::min(initial_size.y, max_outer_size.y));
        ImGui::SetNextWindowSize(initial_size, ImGuiCond_Appearing);
    }

    auto keep_open = true;
    auto const name = opts.Title + "##" + std::to_string(id);
    if (flat_title) {
        const auto clr = g.Style.Colors[ImGuiCol_PopupBg];
        ImGui::PushStyleColor(ImGuiCol_TitleBg, clr);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, clr);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {padding_horz, padding_vert});
    ImGui::Begin(name.c_str(), &keep_open, wnd_flags);
    ImGui::PopStyleVar();
    if (flat_title)
        ImGui::PopStyleColor(2);

    auto window = g.CurrentWindow;

    auto& info = display_stack.emplace_back(Dlg::display_info{
        .WindowID = window->ID,
        .flags = opts.Flags,
        .button_size = button_size,
        .close_action_pending = !keep_open,
    });

    info.button_area_spacing = button_area_spacing;
    info.area_extra = {button_extra.x, button_extra.y + padding_extra_bot};
    info.padding_extra_bot = padding_extra_bot;

    auto const appearing = window->Appearing;
    auto want_auto_size = appearing || !resizable;

    info.autosizing = want_auto_size;

    if (appearing || want_auto_size)
        info.child_state = child_group_pending;
    else
        info.child_state = child_window_pending;

    if (IsCancelShortcut(id))
        info.close_action_pending = true;

    ImGui::BeginGroup(); // outer group

    BeginContentArea();

    return true;
}

void End()
{
    IM_ASSERT(!display_stack.empty());

    auto& info = display_stack.back();
    if (info.close_action_pending /*|| info.default_action_wanted*/)
        ImGui::CloseCurrentPopup();

    DoneContentArea();

    ImGui::EndGroup(); // outer group
    if (info.padding_extra_bot) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);
        ImGui::Dummy({0, info.padding_extra_bot});
    }

    display_stack.pop_back();

    ImGui::EndPopup();
}

void KeepOpen(bool keep)
{
    if (!display_stack.empty())
        display_stack.back().close_action_pending = !keep;
}

auto IsClosing() -> bool
{
    return !display_stack.empty() && display_stack.back().close_action_pending;
}

auto IsAutosizing() -> bool { return !display_stack.empty() && display_stack.back().autosizing; }

void AllowDefaultAction()
{
    if (!display_stack.empty())
        display_stack.back().default_action_allowed = true;
}

void ForbidDefaultAction()
{
    if (!display_stack.empty())
        display_stack.back().default_action_allowed = false;
}

auto DefaultActionAllowed() -> bool
{
    return ImGui::IsWindowFocused(
               ImGuiFocusedFlags_RootAndChildWindows | ImGuiFocusedFlags_NoPopupHierarchy) &&
           display_stack.back().default_action_allowed;
}

auto InDialogWindow(bool popup_hierarchy) -> bool
{
    if (display_stack.empty())
        return false;

    auto& info = display_stack.back();

    auto dialog_wnd = ImGui::FindWindowByID(info.WindowID);
    if (!dialog_wnd)
        return false;

    auto curr_wnd = ImGui::GetCurrentWindowRead();
    if (curr_wnd->ID == info.WindowID)
        return true;

    if (curr_wnd == dialog_wnd)
        return true;

    return ImGui::IsWindowChildOf(curr_wnd, dialog_wnd, popup_hierarchy);
}

InputFieldFlags next_input_field_flags = {};

void ConfigureNextInputField(InputFieldFlags flags)
{
    // idially this should be handled with something like ImGui's nextitemflags
    next_input_field_flags = flags;
}

void HandleInputField()
{
    auto const default_focus =
        (next_input_field_flags & InputFieldFlags::DefaultFocus) == InputFieldFlags::DefaultFocus;
    auto const default_action =
        (next_input_field_flags & InputFieldFlags::DefaultAction) == InputFieldFlags::DefaultAction;
    next_input_field_flags = {};

    if (default_focus) {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere(-1);
        ImGui::SetItemDefaultFocus();
    }

    // Call this after the InputText (or a similar) command.
    if (!InDialogWindow())
        return;

    if (display_stack.empty())
        return;

    auto& info = display_stack.back();

    auto& g = *GImGui;
    auto const item_id = g.LastItemData.ID;
    if (!item_id)
        return;

    // note: this probably should be rewritten if ImGui's InputTextEx implements activation via
    // ImGuiKey_Enter shortcut.
    auto is_active = g.ActiveId == item_id;
    if (is_active) {
        info.default_action_allowed = default_action;
    }
    else {
        auto is_focused = g.NavId == item_id;
        if (is_focused && (g.ActiveIdPreviousFrame == item_id)) {
            // did user just hit the Enter or Escape?
            if (default_action)
                info.default_action_allowed = true;
        }
    }
}

void DbgDisplayNavInfo()
{
    auto& g = *GImGui;

    if (display_stack.empty())
        return;

    auto& info = display_stack.back();

    auto print_id = [&](char const* prefix, ImGuiID id) {
        if (id == info.WindowID)
            ImGui::Text("%s -> DlgWnd", prefix);
        else if (id == ImGuiKeyOwner_NoOwner)
            ImGui::Text("%s -> none", prefix);
        else
            ImGui::Text("%s -> %8x", prefix, id);
    };

    print_id("ActiveId", g.ActiveId);
    print_id("FocusId", g.NavId);

    if (auto data = ImGui::GetShortcutRoutingData(ImGuiKey_Escape)) {
        print_id("ImGuiKey_Escape", data->RoutingCurr);
    }
    if (auto data = ImGui::GetShortcutRoutingData(ImGuiKey_Enter)) {
        print_id("ImGuiKey_Enter", data->RoutingCurr);
    }

    if (auto data = ImGui::GetShortcutRoutingData(ImGuiKey_Space)) {
        print_id("ImGuiKey_Space", data->RoutingCurr);
    }
}

// data storage and globals

struct modal_entry {
    ImGuiID id;
    Options opts;
    std::function<void()> on_display;
};

std::vector<modal_entry> modal_stack;
std::vector<modal_entry> modal_pending;

auto HasModality() -> bool { return !modal_stack.empty(); }

void OpenModality(ImGuiID id, Options&& opts, std::function<void()>&& on_display)
{
    if (!on_display)
        return;

    auto it = std::find_if(modal_pending.begin(), modal_pending.end(),
        [id](modal_entry const& e) { return e.id == id; });

    modal_pending.erase(it, modal_pending.end());

    ImGui::OpenPopup(id);
    modal_pending.push_back(modal_entry{id, std::move(opts), std::move(on_display)});
}

auto DisplayModalities()
{
    auto n = std::size_t{0};
    auto it = modal_stack.begin();
    while (it != modal_stack.end()) {
        if (!ImGui::IsPopupOpen(it->id, ImGuiPopupFlags_None))
            ImGui::OpenPopup(it->id);
        auto b = Dlg::Begin(it->id, it->opts);
        if (!b)
            break;

        it->on_display();
        if (!IsClosing()) {
            ++it;
            continue;
        }

        Dlg::End();
        break;
    }
    it = modal_stack.erase(it, modal_stack.end());
    while (it != modal_stack.begin()) {
        Dlg::End();
        --it;
    }
}

void internal::Initialize()
{
    static bool inited = false;
    if (inited)
        return;

    Application::Callbacks::RenderFrame.push_back(DisplayModalities);

    Application::Callbacks::AfterEachFrame.push_back([] {
        for (auto&& p : modal_pending) {
            auto it = std::find_if(modal_stack.begin(), modal_stack.end(),
                [&](modal_entry const& e) { return e.id == p.id; });
            modal_stack.erase(it, modal_stack.end());
            modal_stack.push_back(std::move(p));
        }
        modal_pending.clear();
    });
    inited = true;
}

} // namespace ImPlus::Dlg