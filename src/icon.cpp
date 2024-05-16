#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <implus/badge.hpp>
#include <implus/icon.hpp>
#include <vector>

namespace ImPlus {

std::vector<std::pair<int, Font::Resource>> tag_map;

inline auto find_font(int tag) -> Font::Resource
{
    for (auto&& p : tag_map)
        if (p.first == tag)
            return p.second;
    return {};
}

void GlyphInfo::RegisterFontTag(int tag, Font::Resource font)
{
    for (auto&& p : tag_map)
        if (p.first == tag) {
            p.second = font;
            return;
        }
    tag_map.emplace_back(std::pair{tag, font});
}

Glyph::Glyph(GlyphInfo const& info)
{
    if (info.FontTag == 0) {
        Symbol = info.Symbol;
        return;
    }
    for (auto&& p : tag_map)
        if (p.first == info.FontTag) {
            Symbol = info.Symbol;
            Font = p.second;
            return;
        }
}

static void draw_builtin(
    ImDrawList* dl, Icon::Builtin shape, ImVec2 const& c, float size, ImU32 clr)
{
    switch (shape) {
    case Icon::Builtin::Box: {
        auto const h = 0.5f * size;
        dl->AddRectFilled({c.x - h, c.y - h}, {c.x + h, c.y + h}, clr);
    } break;

    case Icon::Builtin::Circle: {
        dl->AddCircleFilled(c, 0.5f * size, clr);
    } break;

    case Icon::Builtin::DotDotDot: {
        auto r = size * 0.1f;
        auto x = std::round(c.x) + 0.5f;
        auto y = std::round(c.y) + 0.5f;
        auto dx = std::round(size * 0.35f);
        auto const num_segments = 12;
        dl->AddCircleFilled({x - dx, y}, r, clr, num_segments);
        dl->AddCircleFilled({x, y}, r, clr, num_segments);
        dl->AddCircleFilled({x + dx, y}, r, clr, num_segments);
    } break;

    case Icon::Builtin::Spinner: {
        auto const od = size;
        auto thickness = od * 0.1f;
        auto radius = (od - thickness) * 0.45f;
        auto num_segments = 30;

        auto& g = *GImGui;
        auto start = std::abs(std::sin(g.Time * 1.4f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

        for (int i = 0; i < num_segments; i++) {
            auto const a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
            dl->PathLineTo(
                ImVec2(c.x + ImCos(a + g.Time * 8) * radius, c.y + ImSin(a + g.Time * 8) * radius));
        }

        dl->PathStroke(clr, false, thickness);
    } break;

    default:;
    }
}

auto Icon::Placeholder(length const& width, length const& height) -> CustomIconData
{
    return CustomIconData{
        .width = width,
        .height = height,
        .on_draw =
            [](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImU32 clr) {
                dl->AddRect(bb_min, bb_max, clr);
                auto tl = ImVec2(bb_min.x + 2, bb_min.y + 2);
                auto br = ImVec2{bb_max.x - 2, bb_max.y - 2};
                dl->AddLine(tl, br, clr);
                dl->AddLine({tl.x, br.y - 1}, {br.x, tl.y - 1}, clr);
            },
    };
}

auto Icon::Measure() const -> ImVec2
{
    if (std::holds_alternative<std::monostate>(content_)) {
        return {0, 0};
    }
    else if (auto v = std::get_if<Glyph>(&content_)) {
        if (v->Font)
            ImGui::PushFont(v->Font);
        auto sz = ImGui::CalcTextSize(v->Symbol.data(), v->Symbol.data() + v->Symbol.size());
        if (v->Font)
            ImGui::PopFont();
        return sz;
    }
    else if (auto p = std::get_if<builtin_content>(&content_)) {
        auto h = to_pt(p->size);
        return {h, h};
    }
    else if (auto p = std::get_if<CustomIconData>(&content_)) {
        auto sz = ImVec2{to_pt(p->width), to_pt(p->height)};
        if (sz.x == 0)
            sz.x = sz.y;
        else if (sz.y == 0)
            sz.y = sz.x;
        return sz;
    }
    else if (auto p = std::get_if<GraphicalResource*>(&content_)) {
        if (*p)
            return (*p)->GetSize();
        else
            return {0, 0};
    }
    else {
        return {0, 0};
    }
}

void Icon::Draw(ImDrawList* dl, ImVec2 const& pos, ImU32 clr) const
{
    auto sz = ImVec2{};

    if (std::holds_alternative<std::monostate>(content_)) {
        return;
    }
    else if (auto v = std::get_if<Glyph>(&content_)) {
        if (v->Font)
            ImGui::PushFont(v->Font);
        auto const c = v->Color ? ImGui::GetColorU32(*v->Color) : clr;
        sz = ImGui::CalcTextSize(v->Symbol.data(), v->Symbol.data() + v->Symbol.size());
        dl->AddText(nullptr, 0.0f, pos, c, v->Symbol.data(), v->Symbol.data() + v->Symbol.size());
        if (v->Font)
            ImGui::PopFont();
    }
    else if (auto p = std::get_if<builtin_content>(&content_)) {
        auto h = to_pt(p->size);
        sz = {h, h};
        draw_builtin(dl, p->shape, {pos.x + h * 0.5f, pos.y + h * 0.5f}, h, clr);
    }
    else if (auto p = std::get_if<CustomIconData>(&content_)) {
        sz = ImVec2{to_pt(p->width), to_pt(p->height)};
        if (!sz.x)
            sz.x = sz.y;
        else if (!sz.y)
            sz.y = sz.x;
        if (p->on_draw)
            p->on_draw(dl, pos, pos + sz, clr);
    }
    else if (auto p = std::get_if<GraphicalResource*>(&content_)) {
        if (*p) {
            sz = (*p)->GetSize();
            (*p)->Render(dl, pos, clr);
        }
    }

    for (auto const& overlay : overlays_) {
        if (auto v = std::get_if<Glyph>(&overlay)) {
            if (v->Font)
                ImGui::PushFont(v->Font);
            auto const c = v->Color ? ImGui::GetColorU32(*v->Color) : clr;
            dl->AddText(
                nullptr, 0.0f, pos, c, v->Symbol.data(), v->Symbol.data() + v->Symbol.size());
            if (v->Font)
                ImGui::PopFont();
        }
        else if (auto v = std::get_if<IconBadge>(&overlay)) {
            ImPlus::Badge::Render(dl, pos + sz, v->Content,
                ImPlus::Badge::Options{
                    .Font = v->Font,
                    .ColorSet = v->ColorSet,
                });
        }
    }
}

} // namespace ImPlus