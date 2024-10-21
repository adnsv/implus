#pragma once

#include <imgui.h>

#include <memory>
#include <vector>

#include "editables.hpp"

#include "../accelerator.hpp"
#include "../arithmetics.hpp"
#include "../balloontip.hpp"
#include "../banner.hpp"
#include "../blocks.hpp"
#include "../button.hpp"
#include "../buttonbar.hpp"
#include "../checkbox.hpp"
#include "../color.hpp"
#include "../dlg-common.hpp"
#include "../dlg.hpp"
#include "../input.hpp"
#include "../itemizer.hpp"
#include "../length.hpp"
#include "../listbox.hpp"
#include "../pagination.hpp"
#include "../pathbox.hpp"
#include "../splitter.hpp"
#include "../toolbar.hpp"
#include "../touchscroll.hpp"

namespace ImPlus::Demo {

using namespace ImPlus::literals;

struct DemoBase {
    std::string name = "Unitled";
    ImGuiWindowFlags cflags = {};
    ImGuiWindowFlags wflags = {};

    DemoBase(char const* n, ImGuiChildFlags cf, ImGuiWindowFlags wf)
        : name{n}
        , cflags{cf}
        , wflags{wf}
    {
    }
    virtual ~DemoBase() {}
    virtual void Display() {};
};

struct ButtonDemo : public DemoBase {
    opt_parameter<length> force_width = {"force_width", 10_em};
    opt_parameter<length> force_height = {"force_height", 3_em};
    parameter<ButtonAppearance> button_appearance = {
        "button_appearance", ButtonAppearance::Regular};

    ButtonDemo()
        : DemoBase{"Button", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~ButtonDemo() override {}

    void Display() override
    {
        auto gen_id = ImIDMaker{"##ids"};

        // --- property editors -----------
        editor("Force width", force_width, 2_em, 30_em);
        editor("Force height", force_height, 1_em, 20_em);
        editor("ButtonAppearance", button_appearance);

        // --- content ------------------

        ImGui::BeginChild("scrollable", {0, 0},
            ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding |
                ImGuiChildFlags_NavFlattened,
            ImGuiWindowFlags_HorizontalScrollbar);

        auto size_arg = Sizing::XYArg{};
        if (force_width.use)
            size_arg.Horz = force_width.value;
        if (force_height.use)
            size_arg.Vert = force_height.value;

        ImGui::Button("Stock ImGUI Button");

        ImPlus::Style::Button::Appearance.push(button_appearance.value);

        ImPlus::Button(gen_id(),
            {ImPlus::Icon::Builtin::Circle, "HorzCenter Layout (default)", "With description"},
            size_arg, ImGuiButtonFlags_None);

        ImPlus::Style::Button::Layout.push(Content::Layout::HorzNear);
        ImPlus::Button(gen_id(),
            {ImPlus::Icon::Builtin::Circle, "HorzNear Layout", "With description"}, size_arg);
        ImPlus::Style::Button::Layout.pop();
        ImPlus::Style::Button::Layout.push(Content::Layout::VertCenter);
        ImPlus::Button(gen_id(),
            {ImPlus::Icon::Builtin::Circle, "VertCenter Layout", "With description"}, size_arg);
        ImPlus::Style::Button::Layout.pop();
        ImPlus::Style::Button::Layout.push(Content::Layout::VertNear);
        ImPlus::Button(gen_id(),
            {ImPlus::Icon::Builtin::Circle, "VertNear Layout", "With description"}, size_arg);
        ImPlus::Style::Button::Layout.pop();

        if (ImPlus::BeginDropDownButton(
                gen_id(), {ImPlus::Icon::Builtin::Box, "DropDown"}, size_arg)) {
            ImGui::MenuItem("first");
            ImGui::MenuItem("second");
            ImGui::MenuItem("third");
            ImPlus::EndDropDownButton();
        }

        ImPlus::Style::Button::Layout.push(Content::Layout::HorzNear);
        if (ImPlus::BeginDropDownButton(
                gen_id(), {ImPlus::Icon::Builtin::Box, "DropDown HorzNear"}, size_arg)) {
            ImGui::MenuItem("first");
            ImGui::MenuItem("second");
            ImGui::MenuItem("third");
            ImPlus::EndDropDownButton();
        }
        ImPlus::Style::Button::Layout.pop();

        ImPlus::LinkButton(gen_id(),
            {ImPlus::Icon::Builtin::Circle, "ImPlus LinkButton", "With description"}, size_arg);

        ImPlus::Style::Button::Appearance.pop();

        ImGui::EndChild();
    }
};

struct ButtonbarDemo : public DemoBase {
    parameter<ImVec2> align_all = {"align_all", ImVec2{0, 0}};
    parameter<Axis> item_stacking = {"item_stacking", ImPlus::Style::Buttonbar::ItemStacking()};
    parameter<Content::Layout> item_layout = {"itemlayout", ImPlus::Style::Buttonbar::ItemLayout()};
    Text::OverflowPolicy OverflowPolicy = Style::Button::OverflowPolicy().Caption;
    parameter<length> overflow_width = {"overflow_width", 20_em};
    opt_parameter<length> stretch_width = {"stretch_width", 5_em, true};
    opt_parameter<length> stretch_height = {"stretch_height", 5_em, false};
    parameter<bool> show_descriptions = {"show_descriptions", false};

    opt_parameter<length> force_item_width = {"force_item_width", 10_em};
    opt_parameter<length> force_item_height = {"force_item_height", 3_em};

    ButtonbarDemo()
        : DemoBase{"Buttonbar", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }

    ~ButtonbarDemo() override {}

    void Display() override
    {
        // -- property editors
        editor("Item stacking", item_stacking);
        editor("Icon stacking", item_layout);
        editor("Descriptions", show_descriptions);
        editor_alignment_xy("Alignment", align_all);
        editOverflowPolicy("##overflow-policy", "Overflow policy:", OverflowPolicy);
        editor("Force item width", force_item_width, 2_em, 30_em);
        editor("Force item height", force_item_height, 1_em, 20_em);

        ImGui::BeginDisabled(item_stacking.value != Axis::Horz || force_item_width.use);
        editor("Overflow width", overflow_width, 2_em, 30_em);
        editor("Stretch width", stretch_width, 2_em, 30_em);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(item_stacking.value == Axis::Horz || force_item_height.use);
        editor("Stretch height", stretch_height, 2_em, 30_em);
        ImGui::EndDisabled();

        // -- content -----------------

        ImPlus::Style::Buttonbar::OverflowPolicy.push({OverflowPolicy});
        ImPlus::Style::Buttonbar::ItemStacking.push(item_stacking.value);
        ImPlus::Style::Buttonbar::ItemLayout.push(item_layout.value);
        ImPlus::Style::Buttonbar::Horizontal::OverflowWidth.push(overflow_width.value);
        ImPlus::Style::Buttonbar::Horizontal::StretchItemWidth.push(stretch_width.get());
        ImPlus::Style::Buttonbar::Vertical::StretchItemHeight.push(stretch_height.get());
        ImPlus::Style::Buttonbar::ForceItemWidth.push(force_item_width.get());
        ImPlus::Style::Buttonbar::ForceItemHeight.push(force_item_height.get());

        Buttonbar::Display("##buttons",
            {
                Buttonbar::Button{"OK"},
                Buttonbar::Button{"A long button caption"},
                Buttonbar::Button{{ImPlus::Icon::Builtin::Spinner, "With Spinner",
                    show_descriptions.value ? "With description" : ""}},
                Buttonbar::Button{{ImPlus::Icon::Builtin::DotDotDot, "With DotDotDot",
                    show_descriptions.value ? "With longer description" : ""}},
                Buttonbar::Button{.Content = "Cancel", .SpacingBefore = 0.75_em},
            },
            align_all.value);

        ImPlus::Style::Buttonbar::OverflowPolicy.pop();
        ImPlus::Style::Buttonbar::ItemStacking.pop();
        ImPlus::Style::Buttonbar::ItemLayout.pop();
        ImPlus::Style::Buttonbar::Horizontal::OverflowWidth.pop();
        ImPlus::Style::Buttonbar::Vertical::StretchItemHeight.pop();
        ImPlus::Style::Buttonbar::Horizontal::StretchItemWidth.pop();
        ImPlus::Style::Buttonbar::ForceItemWidth.pop();
        ImPlus::Style::Buttonbar::ForceItemHeight.pop();
    }
};

struct ToolbarDemo : public DemoBase {
    parameter<bool> want_line_after = {"want_line_after", false};
    parameter<bool> remove_spacing_after = {"remove_spacing_after", false};
    parameter<std::optional<Content::Layout>> item_layout = {"item_layout"};
    parameter<Toolbar::TextVisibility> text_visibility = {
        "text_visibility", Toolbar::TextVisibility::Always};
    parameter<bool> overflow_only = {
        "overflow_only", false}; // place all items into an overflow submenu
    parameter<bool> use_window_padding = {"use_window_padding", false};
    parameter<Axis> item_stacking = {"item_stacking", Axis::Horz};

    bool toggleOn = false; // on/off state for toggle button

    ToolbarDemo()
        : DemoBase{"Toolbar", ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar}
    {
    }
    ~ToolbarDemo() override {}

    void Display() override
    {
        ImPlus::Style::Toolbar::BackgroundColor.push(
            Color::Func::Enhance(ImGuiCol_WindowBg, -0.05f));

        auto opts = Toolbar::Options{
            .Padding = use_window_padding.value,
            .WantLineAfter = want_line_after.value,
            .RemoveSpacingAfterToolbar = remove_spacing_after.value,
            .ItemLayout = item_layout.value,
            .ShowText = text_visibility.value,
        };

        if (ImPlus::Toolbar::Begin("##toolbar", item_stacking.value, opts)) {
            ImPlus::Toolbar::Button({ImPlus::Icon::Builtin::Box, "First"});
            ImPlus::Toolbar::Label("Label");
            ImPlus::Toolbar::Button({ImPlus::Icon::Builtin::Box, "Second"});
            ImPlus::Toolbar::Button("No Icon");
            ImPlus::Toolbar::Button(
                {ImPlus::Icon::Builtin::Circle, "With description", "Description text"});

            if (ImPlus::Toolbar::BeginDropDown(
                    {ImPlus::Icon::Builtin::Circle, "DropDown", "Click to see a dropdown menu"})) {
                ImGui::MenuItem("Item 1");
                ImGui::MenuItem("Item 2");
                ImGui::MenuItem("Item 3");
                ImGui::EndPopup();
            }

            ImPlus::Toolbar::Label("->TrailPlacement->", -1, false);

            ImPlus::Toolbar::TrailPlacement();
            if (ImPlus::Toolbar::Button(
                    {{}, "Toggle", "Click to toggle the selected state"}, toggleOn))
                toggleOn = !toggleOn;

            ImPlus::Toolbar::Separator();

            ImPlus::Toolbar::Button(
                {ImPlus::Icon::Builtin::Box, "Disabled", "tooltip is still visible"}, false, false);

            ImPlus::Toolbar::End();
        }

        ImPlus::Style::Toolbar::BackgroundColor.pop();

        auto vert = item_stacking.value == Axis::Vert;
        if (vert) {
            ImGui::SameLine();
            ImGui::BeginGroup();
        }

        editor("Show text", text_visibility);
        editor("Use padding", use_window_padding);
        editor("Item stacking", item_stacking);
        editor("Item layout", item_layout);

        if (vert)
            ImGui::EndGroup();
    }
};

struct SplitterDemo : public DemoBase {
    // construct with 2 bands
    Splitter splt{{
        Splitter::Band{{10_em}},
        Splitter::Band{{spring(50)}, {}, ImVec4{0.4f, 0.5f, 0.3f, 1.0f}},
    }};
    SplitterDemo()
        : DemoBase{"Splitter", ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar}
    {
        // add 2 more bands
        splt.push_back(
            {{spring(50)}, {ImGuiWindowFlags_None, ImGuiChildFlags_AlwaysUseWindowPadding}});
        splt.push_back({{5_em}, {ImGuiWindowFlags_None, ImGuiChildFlags_AlwaysUseWindowPadding}});
        splt.back().Size.Minimum = 5_em;
    }
    ~SplitterDemo() override {}

    void Display() override
    {
        auto print_sizes = [](Splitter::Band& b) {
            ImGui::TextUnformatted("Desired:");
            if (auto ps = std::get_if<spring>(&b.Size.Desired)) {
                ImGui::Text("  %g spring", ps->numeric_value());
            }
            else if (auto pln = std::get_if<length>(&b.Size.Desired)) {
                if (auto ems = std::get_if<em_length>(pln)) {
                    ImGui::Text("  %g em", ems->numeric_value());
                }
                else if (auto pts = std::get_if<pt_length>(pln)) {
                    ImGui::Text("  %g pt", *pts);
                }
            }

            ImGui::Spacing();
            ImGui::TextUnformatted("Minimum:");
            if (auto ems = std::get_if<em_length>(&b.Size.Minimum)) {
                ImGui::Text("  %g em", ems->numeric_value());
            }
            else if (auto pts = std::get_if<pt_length>(&b.Size.Minimum)) {
                ImGui::Text("  %g pt", *pts);
            }

            ImGui::Spacing();
            ImGui::TextUnformatted("Actual:");
            ImGui::Text("  %g px", b.GetSize());
        };

        splt.Display({//
            [&] {
                ImGui::TextUnformatted("Regular");
                ImGui::Button("#1", {ImGui::GetContentRegionAvail().x, 0});
                print_sizes(splt[0]);
            },
            [&] {
                ImGui::TextUnformatted("Custom background color");
                ImGui::Button("#2", {ImGui::GetContentRegionAvail().x, 0});
                print_sizes(splt[1]);
            },
            [&] {
                ImGui::TextUnformatted("With padding");
                ImGui::Button("#3", {ImGui::GetContentRegionAvail().x, 0});
                print_sizes(splt[2]);
            },
            [&] {
                ImGui::TextUnformatted("With padding");
                ImGui::Button("#4", {ImGui::GetContentRegionAvail().x, 0});
                print_sizes(splt[3]);
            }});
    }
};

struct AcceleratorDemo : public DemoBase {
    AcceleratorDemo()
        : DemoBase{"Accelerator", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~AcceleratorDemo() override {}
    void Display() override
    {
        auto k1 = ImPlus::KeyChordFromStr("Control+H");
        auto k2 = ImPlus::KeyChordFromStr("Super+F12");
        auto k3 = ImPlus::KeyChordFromStr("Shortcut+Shift+Alt+Keypad4");

        ImGui::Text("Control+H -> %s", ImPlus::KeyChordToString(k1).c_str());
        ImGui::Text("Super+F12 -> %s", ImPlus::KeyChordToString(k2).c_str());
        ImGui::Text("Shortcut+Shift+Alt+Keypad4 -> %s", ImPlus::KeyChordToString(k3).c_str());
    }
};

enum class fruits {
    apples,
    oranges,
};

struct ListBoxDemo_Simple : public DemoBase {

    fruits enum_sel = fruits(-1);

    std::vector<std::string> vec_items = {
        "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven"};
    std::size_t vec_sel = std::size_t(-1);

    parameter<bool> show_vec = {"show_vec", false};

    ListBoxDemo_Simple()
        : DemoBase{"ListBox Simple", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~ListBoxDemo_Simple() override {}

    void Display() override
    {
        // --- editors ----
        editor("From vector<string>:", show_vec);

        // --- content -----

        ImGui::BeginChild("scrollable", {0, 0},
            ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding |
                ImGuiChildFlags_NavFlattened,
            ImGuiWindowFlags_HorizontalScrollbar);

        if (show_vec.value) {
            ImPlus::Listbox::Strings("##vec-items", vec_items, vec_sel);
        }
        else {
            /* ImPlus::Listbox::Enums({fruits::apples, fruits::oranges}, enum_sel, [](fruits const&
             f) { switch (f) { case fruits::apples: return "apples"; case fruits::oranges: return
             "oranges"; default: return "unknown";
                 }
             });*/
        }

        ImGui::EndChild();
    }
};

struct FlowDemo : public DemoBase {
    KeyValPlacement kvp = KeyValPlacement::Inline;
    std::string text1;
    std::string text2;
    std::string text3;
    std::string text4;
    std::string lbl;
    int intval = 0;
    std::optional<int> optintval = {};
    double floatval1 = 3.14159265358979323846264;
    float floatval2 = 1e8;
    std::size_t combo_index = 0;
    bool checkval = false;

    FlowDemo()
        : DemoBase{"Flow", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~FlowDemo() override {}

    void Display() override
    {
        Style::Flow::KeyValPlacement.push(kvp);

        auto f = ImPlus::Flow{"##demo-flow", 30_em};

        f.Paragraph("Main Instruction Paragraph", {.Font = ImPlus::GetMainInstructionFont()});

#if 0
        f.Paragraph("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
                    "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo "
                    "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse "
                    "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non "
                    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
#endif

        ImGui::Indent();
        f.Paragraph("Indented paragraph. Ut enim ad minim veniam, quis nostrud exercitation "
                    "ullamco laboris nisi ut aliquip ex ea commodo consequat.");
        ImGui::Unindent();

        f.MultiTogglerField("##placement", "Placement", {"Inline", "Block"}, kvp);

        f.InputTextField("##input-1", "Key label", text1);
        f.InputTextField("##input-2", "Next key label", lbl);
        f.InputTextField("##input-3", lbl, text3);
        f.InputNumericField("##input-int", "numeric int", intval);
        f.InputNumericField("##input-optint", "optional<int>", optintval);
        f.InputNumericField("units##input-float", "float", floatval1);
        ImPlus::Style::Numeric::DecimalSeparator.push(',');
        f.InputNumericField("##input-float1", "decimal-comma", floatval1);
        ImPlus::Style::Numeric::DecimalSeparator.pop();

        if (f.CollapsingHeader("##h1", "H1")) {
            ImGui::TextUnformatted("blah");
        }
        else {
            f.CollapsingHeaderOverlayText("blah");
        }

        if (f.CollapsingHeader("##h2", "Collapsing Header")) {
            f.ComboStringField("##combobox", "Combo field", {"one", "two", "three"}, combo_index);
            f.CheckboxField("##checkbox", "Checkbox field", checkval);
            f.TogglerField("##toggler", "Toggler field", checkval);
            ImGui::Indent();
            f.InputTextField("##input-4", "Indented text field", text4);
            f.TextField("key1", "value #1");
            f.TextField("key2", "value #2");
            ImGui::Unindent();
        }
        else {
            f.CollapsingHeaderOverlayText("Overlay");
        }

        Style::Flow::KeyValPlacement.pop();
    }
};

#if 0
struct PropertyListDemo : public DemoBase {
    PropertyListDemo()
        : DemoBase{"PropertyList", ImGuiWindowFlags_AlwaysUseWindowPadding}
    {
    }
    ~PropertyListDemo() override {}
    float v = 3.1415926f;
    bool b = false;
    std::size_t combo_sel = 0;

    void Display() override
    {
        ImGui::BeginChild("scrollable", {0, 0}, ImGuiChildFlags_AlwaysUseWindowPadding,
            ImGuiWindowFlags_HorizontalScrollbar |
                ImGuiWindowFlags_NavFlattened);

        auto dc = ImPlus::PropertyList::Context{};

        if (dc.CollapsingHeader("Header", true)) {
            dc.KeyVal("Key1", "Value1");
            dc.KeyFmt("Key2", "Formatted %d", 42);

            dc.KeyVal("Int", 42);
            dc.KeyVal("Flt", 3.14159f);
            dc.KeyValU("with units", "%", 42.3f);
            dc.KeyValU("with units", "mm", 33);
        }

        if (auto hc = dc.CollapsingHeaderEx("Fields", true)) {
            dc.InputArithmetic("Arithmetic Field", "##lbl1", v);
            dc.InputArithmetic("With Units", "Units##lbl2", v);
            dc.Checkbox("Check box", "##lbl3", b);
            dc.ComboStrings("Combo Field", "##lbl4", {"one", "two", "three"}, combo_sel);

            static bool checkbox = true;
            dc.KeyWithCheckBox("KeyWithCheckBox", checkbox);
            ImGui::TextUnformatted("value");
            dc.KeyVal("", "empty key");
        }
        else {
            hc.Val("Addition Content in Header");
        }

        if (dc.CollapsingHeader("Level 0")) {
            ImGui::Indent();
            if (dc.CollapsingHeader("Level 1")) {
                ImGui::Indent();
                if (dc.CollapsingHeader("Level 2")) {
                    dc.KeyVal("key", "val");
                }
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }
        ImGui::EndChild();
    }
};
#endif

struct BannerDemo : public DemoBase {
    std::string content = "This is a banner";
    std::string description =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fusce et neque sagittis, porta "
        "dui quis, convallis mauris. Aenean pellentesque finibus sapien nec molestie.";
    bool show_close_btn = true;

    BannerDemo()
        : DemoBase{"Banner", ImGuiChildFlags_None, ImGuiWindowFlags_None}
    {
    }
    ~BannerDemo() override {}

    void Display() override
    {
        if (!content.empty()) {
            ImPlus::Style::Banner::BackgroundColor.push({0.25, 0.5, 0.5, 1.0});
            auto want_close =
                Banner::Display("##banner", {{Icon::Builtin::Box, 2_em}, content, description},
                    show_close_btn ? Banner::ShowCloseButton : Banner::None);
            if (want_close)
                content.clear();
            ImPlus::Style::Banner::BackgroundColor.pop();
        }
        else {
            if (ImGui::Button("Show"))
                content = "Showing again with new content";
        }
        ImGui::Checkbox("With close button", &show_close_btn);
    }
};

struct PathboxDemo : public DemoBase {
    PathboxDemo()
        : DemoBase{"Pathbox", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~PathboxDemo() override {}

    void Display() override
    {
        auto items = std::vector<ImPlus::Pathbox::Item>{};
        items.emplace_back("First");
        items.emplace_back("Second");
        items.emplace_back("Third item");
        items.emplace_back("One more item");
        items.emplace_back("This one has longer caption");
        ImPlus::Pathbox::Display("blah", {}, items);
    }
};

struct PaginationDemo : public DemoBase {
    parameter<ImPlus::Pagination::Placement> horz_placement = {
        "placement", ImPlus::Pagination::Placement::Near};
    parameter<std::size_t> limit_boxes = {"limit_boxes", 11};
    parameter<std::size_t> n_pages = {"n_pages", 14};
    opt_parameter<length> width = {"custom_width", 10_em, false};
    std::size_t index = 0;

    PaginationDemo()
        : DemoBase{"Pagination", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }

    ~PaginationDemo() override {}

    void Display() override
    {
        editor("Horz Placement", horz_placement);
        editor("Limit Box Count", limit_boxes, 5, 25);
        editor("N Pages", n_pages, 0, 110);
        editor("Width", width, 2_em, 30_em);

        auto opts = ImPlus::Pagination::Options{
            .HorzPlacement = horz_placement.value,
            .LimitBoxCount = limit_boxes.value,
        };

        auto sz = ImVec2{0, 0};
        if (auto v = width.get(); v)
            sz.x = ImPlus::to_pt(*v);

        auto c = ImPlus::Pagination::Display("##p", sz, index, n_pages.value, opts);
        if (c)
            index = *c;
    }
};

struct TypographyDemo : public DemoBase {
    std::string label = "Hello, World!";
    std::string descr = "01234567890123456789  \nabcdefghijkl   mnopqrstuvwxyz : 123";
    parameter<Content::Layout> layout = {"layout", Content::Layout::HorzNear};
    parameter<bool> show_icon = {"show_icon", true};

    Text::OverflowPolicy OverflowPolicy;
    pt_length OverflowWidth = 500_pt;
    ImVec2 sz;

    TypographyDemo()
        : DemoBase{"Typography", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~TypographyDemo() override {}

    void Display() override
    {
        editOverflowPolicy("##overflow-policy", "Overflow policy: ", OverflowPolicy);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Overflow width:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("pt##overflow-width", &OverflowWidth, 0, 1024, "%.0f");

        editor("Stacking", layout);
        editor("Show icon", show_icon);

        auto pos = ImGui::GetCursorScreenPos();
        auto marker_min = ImVec2(pos.x + OverflowWidth, pos.y);
        auto marker_max = ImVec2(pos.x + OverflowWidth + 10, pos.y + ImGui::GetTextLineHeight());

        auto lv =
            ICD_view{show_icon.value ? ImPlus::Icon{ImPlus::Icon::Builtin::Circle} : ImPlus::Icon{},
                label, descr};
        auto lb = ICDBlock{lv, layout.value, {}, OverflowPolicy, OverflowWidth};
        lb.Display();

        auto dl = ImGui::GetWindowDrawList();
        dl->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 255, 0, 255));
        dl->AddRectFilled(marker_min, marker_max, IM_COL32(255, 0, 255, 255));
    }
};

struct InputDemo : public DemoBase {
    std::string buf1;
    std::string buf2;
    float v = 3.14;
    parameter<ImGuiDir> balloon_tip_direction = {"balloon_tip_direction", ImGuiDir_Right};
    parameter<float> balloon_tip_alignment = {"balloon_tip_alignment", 0.5f};

    InputDemo()
        : DemoBase{"Input", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~InputDemo() override {}

    void Display() override
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Input1");
        ImGui::SameLine();
        ImPlus::InputText("##input1", buf1);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Input2");
        ImGui::SameLine();
        ImPlus::InputText("##input2", buf2);

        editor("Balloon tip direction", balloon_tip_direction);
        editor_alignment("Balloon tip alignment", balloon_tip_alignment);

        if (ImGui::Button("Balloon #1")) {
            ImPlus::OpenBalloonTip("##input1", {"Balloon content #1"},
                {
                    .Direction = balloon_tip_direction.value,
                    .Alignment = balloon_tip_alignment.value,
                });
        }
        if (ImGui::Button("Balloon #2")) {
            ImPlus::OpenBalloonTip("##input2", {"Balloon content #2"},
                {
                    .Direction = balloon_tip_direction.value,
                    .Alignment = balloon_tip_alignment.value,
                });
        }

        static auto want_balloon = false;

        ImPlus::InputNumeric("##numeric-input", v, [](float& v) {
            if (v < 0) {
                v = 0;
            }
            else if (v > 100.0f) {
                v = 100.0f;
            }
            return true;
        });
    }
};

struct PwdDialog : public ImPlus::Dlg::CustomEditor {
    std::string password;
    std::string confirm;

    ~PwdDialog() override {}

    void Present() override
    {

        Dlg::ConfigureNextInputField(Dlg::InputFieldFlags::DefaultFocus);
        ImGui::TextUnformatted("Password:");
        ImPlus::InputTextWithHint("##password", "Password", password);
        if (password.empty())
            ScheduleBalloonTip({"Empty input is not allowed"});

        Dlg::ConfigureNextInputField(Dlg::InputFieldFlags::DefaultAction);
        ImGui::TextUnformatted("Confirm:");
        ImPlus::InputTextWithHint("##confirm", "Confirm", confirm);
        if (confirm.empty())
            ScheduleBalloonTip({"Empty input is not allowed"});
        else if (confirm != password)
            ScheduleBalloonTip({"Passwords don't match"});
    }

    auto Accept() -> bool override
    {
        Dlg::Message::Open("Password", {.MainInstruction = "Password Accepted"});
        return true;
    }
};

struct DialogDemo : public DemoBase {
    bool Show = false;
    std::string long_msg =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec hendrerit leo eu elit "
        "bibendum, vitae consequat urna sodales. Phasellus mi purus, efficitur ac nisi vitae, "
        "interdum malesuada risus. In a auctor tortor. Nulla quis congue mi. Ut sit amet rutrum "
        "lacus. In sollicitudin vel magna eget varius. In ac malesuada risus, fringilla porttitor "
        "est. ";

    std::string b3 = "OK";
    std::string s1 = "";

    DialogDemo()
        : DemoBase{"Dialogs", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~DialogDemo() override {}

    void Display() override
    {

        if (ImPlus::Button("##show-message", "Show Message"))
            ImPlus::Dlg::Message::Open("Title", //
                {
                    .Icon = ImPlus::Icon::Builtin::Box,
                    .MainInstruction = "Main Instruction",
                    .Flow = "Your description content here.",
                    .DismissButtonText = "Dismiss",
                });

        if (ImPlus::Button("##show-confirm", "Show Confirm"))
            ImPlus::Dlg::Confirm::Open("", //
                {
                    .MainInstruction = "Main Instruction",
                    .Flow = "Flow content",
                    .OkButtonText = "Confirm",
                });

        if (ImPlus::Button("##show-inputtext", "Show InputText"))
            ImPlus::Dlg::TextEditor::Open("Edit Text",
                {
                    .Icon = {ImPlus::Icon::Placeholder(6_em, 6_em)},
                    .MainInstruction = "Main Instruction",
                    .Flow = "Enter text here",
                    .Text = s1,
                    .OkButtonText = "Rename",
                    .OnValidate = [prev = s1](
                                      std::string const& s) -> Dlg::TextEditor::validate_result {
                        if (s.empty())
                            return Dlg::Feedback::ShowBalloon{"Empty input is not allowed"};
                        else if (s == prev)
                            return Dlg::Feedback::DisableAccept{};
                        else
                            return Dlg::Feedback::EnableAccept{};
                    },
                    .OnAccept =
                        [this](std::string const& s) {
                            s1 = s;
                            // note: you can still call KeepOpen inside the OnAccept to
                            // prevent the dialog from closing
                        },
                });

        if (ImPlus::Button("##show-custom-editor", "Show Custom Editor")) {

            auto d = std::make_shared<PwdDialog>();

            PwdDialog::Open("##pwd-dialog", "Enter Password", std::move(d));
        }

        if (ImPlus::Button("##show-choices", "Show Choices"))
            ImPlus::Dlg::Choices::Open("Choices", //
                {
                    .MainInstruction = "Main Instruction",
                    .Flow = "Make your choice",
                    .Items =
                        {
                            {{}, "First Choice", "Description of the first choice"},
                            {{}, "Second Choice", "Description of the second choice"},
                            {{}, "Third Choice", "Description of the third choice"},
                        },
                    .OnClick =
                        [](std::size_t idx) {
                            ImPlus::Dlg::Message::OpenEx("##SHOW_CHOICE", "Showing Choice", //
                                {
                                    .Icon = ImPlus::Icon::Builtin::Box,
                                    .MainInstruction =
                                        std::string{"Selected choice #"} + std::to_string(idx),
                                    .DismissButtonText = "Dismiss",
                                });
                        },
                });

        auto const mydlg_id = ImGui::GetID("##MY_DIALOG");
        if (ImGui::Button("Custom")) {
            auto opts = ImPlus::Dlg::Options{
                .Title = "My Dialog",
                .Flags = ImPlus::Dlg::Flags::Resizable,
            };

            ImPlus::Dlg::OpenModality(mydlg_id, std::move(opts), [&]() {
                // ImPlus::Dlg::Block({ImPlus::Icon::Builtin::Circle, "Main Title", long_msg},
                // 24_em);

                ImPlus::Dlg::ConfigureNextInputField(
                    Dlg::InputFieldFlags::DefaultAction | Dlg::InputFieldFlags::DefaultFocus);
                ImPlus::InputText("##input1", s1);

                static auto idx = std::size_t{0};
                ImPlus::Combo::Strings("##combobox", {"one", "two", "three"}, idx);

                static bool c1 = false;
                static bool c2 = false;
                ImGui::Checkbox("Check1", &c1);
                ImGui::Checkbox("Check2", &c2);

                if (ImPlus::Button("##child", "Child Dlg...")) {
                    ImPlus::Dlg::TextEditor::Open("Edit Text",
                        {
                            .Icon = {ImPlus::Icon::Placeholder(6_em, 6_em)},
                            .MainInstruction = "Main Instruction",
                            .Flow = "Enter text here",
                            .Text = s1,
                            .OkButtonText = "Rename",
                            .OnValidate = [prev = s1](std::string const& s)
                                -> Dlg::TextEditor::validate_result {
                                if (s.empty())
                                    return Dlg::Feedback::ShowBalloon{"Empty input is not allowed"};
                                else if (s == prev)
                                    return Dlg::Feedback::DisableAccept{};
                                else
                                    return Dlg::Feedback::EnableAccept{};
                            },
                            .OnAccept =
                                [this](std::string const& s) {
                                    s1 = s;
                                    // note: you can still call KeepOpen inside the OnAccept to
                                    // prevent the dialog from closing
                                },
                        });
                }

                Dlg::DbgDisplayNavInfo();

                auto action =
                    Dlg::Buttons({"OK", "Cancel"}, ImPlus::Buttonbar::Flags::FirstIsDefault |
                                                       ImPlus::Buttonbar::Flags::LastIsCancel);
                if (action.has_value())
                    Dlg::Close();
            });
        }
    }
};

struct ScrollingDemo : public DemoBase {
    float sample_float_1 = 0.0f;
    float sample_float_2 = 0.0f;
    std::size_t sel_index = 0;
    std::size_t n_items = 230;

    ScrollingDemo()
        : DemoBase{"Touch Scrolling", ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None}
    {
    }
    ~ScrollingDemo() override {}

    void Display() override
    {
        char buf[128];

        auto makeid = ImPlus::ImIDMaker("##items");

        for (auto i = std::size_t{0}; i < n_items; ++i) {
            if (i == 0) {
                ImGui::TextUnformatted("  --- just text");
            }
            else if (i == 2) {
                ImGui::TextUnformatted("  --- just text");
                ImGui::TextUnformatted("  --- just text");
                ImGui::TextUnformatted("  --- just text");
                ImGui::TextUnformatted("  --- just text");
            }

            if (i == 4) {
                ImGui::Button("blah-blah");
                ImGui::InputFloat("##blah", &sample_float_1);
                ImGui::SliderFloat("##blah2", &sample_float_2, 0.0f, 100.0f);
            }

            auto item_selected = i == sel_index;


            auto ItemID = makeid();

            snprintf(buf, 128, "Item #%d (%.x)", int(i), ItemID);

            auto caption = buf;
            auto blk = ImPlus::TextBlock{caption, {0, 0}};

            auto state = ImPlus::SelectableBox(ItemID, buf, item_selected,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, blk.Size,
                {},
                [&](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max,
                    ImPlus::ColorSet const& cs) { blk.Render(dl, bb_min, bb_max, cs.Content); });

            if (state.Pressed)
                sel_index = i;
        }

        ImGui::Button("Button");

        for (auto i = std::size_t{0}; i < 30; ++i) {
            auto ItemID = makeid();
            snprintf(buf, 128, "Another Item #%d (%.x)", int(i), ItemID);
            ImGui::TextUnformatted(buf);
        }

        ImPlus::HandleTouchScroll(ImPlus::TouchScrollFlags_EnableMouseScrolling);
    }
};


struct Window {
    ImPlus::Splitter splt{{
        // lhs band:
        Splitter::Band{{8_em},
            {ImGuiWindowFlags_None,
                ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened},
            ImPlus::Color::Func::Enhance(ImGuiCol_WindowBg, -0.05f)},
        // rhs band:
        Splitter::Band{{spring(100.0f)}, {ImGuiWindowFlags_None, ImGuiChildFlags_NavFlattened}},
    }};

    std::vector<std::unique_ptr<DemoBase>> demo_list;
    std::size_t demo_index = -1;

    Window()
    {
        demo_list.push_back(std::make_unique<ButtonDemo>());
        demo_list.push_back(std::make_unique<ButtonbarDemo>());
        demo_list.push_back(std::make_unique<ToolbarDemo>());
        demo_list.push_back(std::make_unique<ListBoxDemo_Simple>());
        demo_list.push_back(std::make_unique<SplitterDemo>());
        demo_list.push_back(std::make_unique<FlowDemo>());
        // demo_list.push_back(std::make_unique<PropertyListDemo>());
        demo_list.push_back(std::make_unique<BannerDemo>());
        demo_list.push_back(std::make_unique<PathboxDemo>());
        demo_list.push_back(std::make_unique<PaginationDemo>());
        demo_list.push_back(std::make_unique<TypographyDemo>());
        demo_list.push_back(std::make_unique<InputDemo>());
        demo_list.push_back(std::make_unique<DialogDemo>());
        demo_list.push_back(std::make_unique<AcceleratorDemo>());
        demo_list.push_back(std::make_unique<ScrollingDemo>());
    }

    void Display(bool* p_open)
    {
        ImGui::SetNextWindowSize({to_pt(40_em), to_pt(30_em)}, ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        auto expanded = ImGui::Begin(
            "ImPlus Demo", p_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
        ImGui::PopStyleVar();
        if (expanded) {
            splt.Display({
                [&] { // list of demos
                    ImPlus::Listbox::Strings(
                        "", demo_list, demo_index, [](auto&& v) { return v.get()->name; });
                },
                [&] { // demo view panel
                    if (demo_index < demo_list.size()) {
                        auto* demo = demo_list[demo_index].get();
                        ImGui::BeginChild("##content", {0, 0}, demo->cflags, demo->wflags);
                        demo->Display();
                        ImGui::EndChild();
                    }
                },

            });
        }
        ImGui::End();
    };
};

} // namespace ImPlus::Demo