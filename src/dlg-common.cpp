#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/button.hpp"
#include "implus/dlg-common.hpp"

namespace ImPlus::Dlg {

void Message::Open(std::string_view title, Message&& content)
{
    OpenEx(ImGui::GetID("##.MESSAGE.DIALOG."), title, std::move(content));
}

void Message::OpenEx(ImPlus::ImID id, std::string_view title, Message&& content)
{
    auto opts = Dlg::Options{
        .Title = std::string{title},
        .StretchContentWidth = 10_em,
    };
    OpenModality(id, std::move(opts), [content = std::move(content)]() {
        BeginMainInstructionGroup(content.Icon, content.MainInstruction, content.Flow);
        ImGui::EndGroup();
        ImGui::Spacing();

        auto action = Dlg::Buttons(
            {
                content.DismissButtonText.empty() ? "Dismiss" : content.DismissButtonText,
            },
            Buttonbar::Flags::FirstIsDefault | ImPlus::Buttonbar::Flags::LastIsCancel);

        if (action.has_value())
            Close();
    });
}

void Confirm::Open(std::string_view title, Confirm&& content)
{
    return OpenEx(ImGui::GetID("##.CONFIRM.DIALOG."), title, std::move(content));
}

void Confirm::OpenEx(ImPlus::ImID id, std::string_view title, Confirm&& content)
{
    auto opts = Dlg::Options{
        .Title = std::string{title},
        .StretchContentWidth = 10_em,
    };
    OpenModality(id, std::move(opts), [content = std::move(content)]() {
        BeginMainInstructionGroup(content.Icon, content.MainInstruction, content.Flow);
        ImGui::EndGroup();
        ImGui::Spacing();

        auto flags = Buttonbar::Flags::FirstIsDefault | ImPlus::Buttonbar::Flags::LastIsCancel;
        auto action = Dlg::Buttons(
            {
                content.OkButtonText.empty() ? "OK" : content.OkButtonText,
                content.CancelButtonText.empty() ? "Cancel" : content.CancelButtonText,
            },
            flags);

        if (action == 0) {
            Close();

            // note: you can still call KeepOpen inside the OnAccept to prevent the
            // dialog from closing
            if (content.OnAccept)
                content.OnAccept();
        }
    });
}

void TextEditor::Open(std::string_view title, TextEditor&& content)
{
    OpenEx(ImGui::GetID("##.TEXT_EDITOR.DIALOG."), title, std::move(content));
}

void TextEditor::OpenEx(ImPlus::ImID id, std::string_view title, TextEditor&& content)
{
    auto opts = Dlg::Options{
        .Title = std::string{title},
    };
    OpenModality(id, std::move(opts), [content = std::move(content)]() mutable {
        BeginMainInstructionGroup(content.Icon, content.MainInstruction, content.Flow);

        Dlg::ConfigureNextInputField(
            Dlg::InputFieldFlags::DefaultAction | Dlg::InputFieldFlags::DefaultFocus);
        ImPlus::InputTextWithHint("##input", content.Hint.c_str(), content.Text);
        auto input_id = ImGui::GetItemID();
        ImPlus::HandleLastItemBalloonTip();
        if (IsRejectActionKeyPressed())
            Close();

        ImGui::EndGroup();
        ImGui::Spacing();

        auto fb = content.OnValidate ? content.OnValidate(content.Text) : Feedback::EnableAccept{};

        auto enable_ok_btn = std::holds_alternative<Feedback::EnableAccept>(fb) ||
                             std::holds_alternative<Feedback::ShowBalloon>(fb);

        auto flags = Buttonbar::Flags::FirstIsDefault | ImPlus::Buttonbar::Flags::LastIsCancel;
        if (!enable_ok_btn)
            flags = flags | Buttonbar::Flags::DisableAllButLast;

        auto action = Dlg::Buttons(
            {
                content.OkButtonText.empty() ? "OK" : content.OkButtonText,
                content.CancelButtonText.empty() ? "Cancel" : content.CancelButtonText,
            },
            flags);

        if (!action.has_value())
            return;

        if (*action == 0) {
            if (auto h = std::get_if<Feedback::ShowBalloon>(&fb)) {
                ImGui::ActivateItemByID(input_id);
                ImPlus::OpenBalloonTip(input_id, *h, {.Direction = ImGuiDir_Right});
            }
            else if (std::holds_alternative<Feedback::EnableAccept>(fb)) {
                Close();
                // note: you can still call KeepOpen inside the OnAccept to prevent the
                // dialog from closing
                if (content.OnAccept)
                    content.OnAccept(content.Text);
            }
        }
    });
}

void CustomEditor::Open(
    ImPlus::ImID id, std::string_view title, std::shared_ptr<CustomEditor> content)
{
    auto opts = Dlg::Options{
        .Title = std::string{title},
    };
    OpenModality(id, std::move(opts), [c = std::move(content)]() mutable {
        auto& content = *c;
        content.allow_accept_ = true;
        content.scheduled_balloon_tip_.reset();

        BeginMainInstructionGroup(content.Icon, content.MainInstruction, content.Description);

        content.Present();

        ImGui::EndGroup();
        ImGui::Spacing();

        auto allow_accept = content.allow_accept_;

        if (IsRejectActionKeyPressed())
            Close();

        auto flags = Buttonbar::Flags::FirstIsDefault | ImPlus::Buttonbar::Flags::LastIsCancel;
        if (!allow_accept)
            flags = flags | Buttonbar::Flags::DisableAllButLast;

        auto action = Dlg::Buttons(
            {
                content.OkButtonText.empty() ? "OK" : content.OkButtonText,
                content.CancelButtonText.empty() ? "Cancel" : content.CancelButtonText,
            },
            flags);

        if (!action.has_value())
            return;

        if (*action == 0) {
            // do not call Accept if we already balloon tip scheduled from within content.Present
            if (allow_accept && !content.BalloonTipScheduled()) {
                // notice that content.Accept can also schedule a balloon tip
                allow_accept = content.Accept(); 
            }

            if (content.BalloonTipScheduled()) {
                ImGui::ActivateItemByID(content.scheduled_balloon_tip_->item_id);
                ImPlus::OpenBalloonTip(content.scheduled_balloon_tip_->item_id,
                    content.scheduled_balloon_tip_->content,
                    {.Direction = content.scheduled_balloon_tip_->direction});
            }

            if (allow_accept)
                Close();
        }
    });
}

auto CustomEditor::BalloonTipScheduled() const -> bool {
    return scheduled_balloon_tip_.has_value();
}

void CustomEditor::ScheduleBalloonTipEx(
    ImID item_id, ICD&& content, std::optional<ImGuiDir> direction)
{
    if (scheduled_balloon_tip_.has_value())
        return;
    scheduled_balloon_tip_ = BalloonTipInfo{
        .item_id = item_id,
        .direction = direction.value_or(DefaultBalloonTipDirection),
        .content = std::move(content),
    };
}

void CustomEditor::ScheduleBalloonTip(ICD&& content, std::optional<ImGuiDir> direction)
{
    auto last_item_id = ImGui::GetItemID();
    ScheduleBalloonTipEx(last_item_id, std::move(content), direction);
}

void CustomEditor::DisableAccept() { allow_accept_ = false; }

void Choices::Open(std::string_view title, Choices&& cc)
{
    OpenEx(ImGui::GetID("##.CHOICES.DIALOG."), title, std::move(cc));
}

void Choices::OpenEx(ImPlus::ImID id, std::string_view title, Choices&& content)
{
    auto opts = Dlg::Options{
        .Title = std::string{title},
        .Flags = Flags::NoButtonArea,
    };
    OpenModality(id, std::move(opts), [content = std::move(content)]() {
        BeginMainInstructionGroup(content.Icon, content.MainInstruction, content.Flow);
        ImGui::EndGroup();

        auto const button_sizing = ImPlus::Sizing::XYArg{
            .Horz = ImPlus::Sizing::Advanced{.Desired = 16_em, .Stretch = 24_em},
            //.Vert = ImPlus::Sizing::Advanced{.Desired = 2_em},
        };

        auto window = ImGui::GetCurrentWindow();
        auto clicked = std::optional<std::size_t>{};
        auto idx = std::size_t(0);
        Style::Button::Layout.push(Content::Layout::HorzNear);
        for (auto&& ch : content.Items) {
            auto const item_id = window->GetID(idx);
            if (ImPlus::Button(item_id, ch, button_sizing))
                clicked = idx;
            ++idx;
        }
        Style::Button::Layout.pop();

        DoneContentArea();

        if (IsClosing())
            clicked = Buttonbar::CancelSentinel;

        if (clicked) {
            Close();
            if (content.OnClick)
                content.OnClick(*clicked);
        }
    });
}

} // namespace ImPlus::Dlg
