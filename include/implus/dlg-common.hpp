#pragma once

#include "implus/balloontip.hpp"
#include "implus/buttonbar.hpp"
#include "implus/dlg.hpp"
#include "implus/flow.hpp"
#include "implus/icd.hpp"
#include "implus/input.hpp"
#include "implus/overridable.hpp"

#include <exception>
#include <functional>
#include <future>
#include <optional>
#include <variant>

namespace ImPlus::Dlg {

namespace Feedback {

// EnableAccept enable accept as default action
struct EnableAccept {};

// DisableAccept disables accept as default action
struct DisableAccept {};

// ShowBalloon enables the default action, but instead of accepting displays a hint.
struct ShowBalloon : public ICD_view {};

} // namespace Feedback

// Message shows a simple box with a "Dismiss" button.
// To get status, use: InProgress() / Dismissed().
struct Message {
    ImPlus::Icon Icon;
    std::string MainInstruction;
    ImPlus::FlowContent Flow;
    std::string DismissButtonText;
    static void OpenEx(ImPlus::ImID id, std::string_view title, Message&&);
    static void Open(std::string_view title, Message&&);
};

// Confirm shows a simple box with "OK" / "Cancel" buttons.
// To get status, use: InProgress() / Dismissed() / Accepted().
struct Confirm {
    ImPlus::Icon Icon;
    std::string MainInstruction;
    ImPlus::FlowContent Flow;
    std::string OkButtonText;
    std::string CancelButtonText;
    std::function<void()> OnAccept;
    static void OpenEx(ImPlus::ImID id, std::string_view title, Confirm&&);
    static void Open(std::string_view title, Confirm&&);
};

// TextEditor shows an input field with "OK" / "Cancel" buttons.
// To get status, use: InProgress() / Dismissed() / Retrieve<std::string>().
struct TextEditor {
    using validate_result =
        std::variant<Feedback::EnableAccept, Feedback::DisableAccept, Feedback::ShowBalloon>;
    using validate_callback = std::function<validate_result(std::string const&)>;

    ImPlus::Icon Icon;
    std::string MainInstruction;
    ImPlus::FlowContent Flow;
    std::string Text;
    std::string Hint;
    std::string OkButtonText;
    std::string CancelButtonText;
    validate_callback OnValidate;
    std::function<void(std::string const&)> OnAccept;
    static void OpenEx(ImPlus::ImID id, std::string_view title, TextEditor&&);
    static void Open(std::string_view title, TextEditor&&);
};

struct CustomEditor {
    virtual ~CustomEditor() {}
    static void Open(ImPlus::ImID id, std::string_view title, std::shared_ptr<CustomEditor>);

    ImPlus::Icon Icon;
    std::string MainInstruction;
    ImPlus::FlowContent Description;

    std::string OkButtonText;
    std::string CancelButtonText;

    ImGuiDir DefaultBalloonTipDirection = ImGuiDir_Right;

    struct BalloonTipInfo {
        ImID item_id;
        ImGuiDir direction;
        ICD content;
    };

    bool allow_accept_ = false;
    std::optional<BalloonTipInfo> scheduled_balloon_tip_;

    void ScheduleBalloonTipEx(ImID item_id, ICD&& content, std::optional<ImGuiDir> direction = {});
    void ScheduleBalloonTip(ICD&& content, std::optional<ImGuiDir> direction = {});
    auto BalloonTipScheduled() const -> bool;
    void DisableAccept();

    virtual void Present() {}
    virtual auto Accept() -> bool { return true; }
};

struct Choices {
    ImPlus::Icon Icon;
    std::string MainInstruction;
    ImPlus::FlowContent Flow;
    std::vector<ICD> Items;
    std::function<void(std::size_t)> OnClick;
    static void OpenEx(ImPlus::ImID, std::string_view title, Choices&&);
    static void Open(std::string_view title, Choices&&);
};

} // namespace ImPlus::Dlg