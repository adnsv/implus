#pragma once

#include <imgui.h>

#include <any>
#include <functional>
#include <string>
#include <string_view>
#include <variant>

#include <implus/color.hpp>
#include <implus/font.hpp>
#include <implus/length.hpp>

namespace ImPlus {

struct ResettableResource {
    static inline std::vector<ResettableResource*> Instances;

    ResettableResource() { Instances.push_back(this); }
    virtual ~ResettableResource()
    {
        if (auto it = std::find(Instances.begin(), Instances.end(), this); it != Instances.end())
            Instances.erase(it);
    }

    virtual void Reset() {}

    static void ResetAll()
    {
        for (auto p : Instances)
            p->Reset();
    }
};

struct SizableResource {
    virtual ~SizableResource() {}
    virtual auto GetSize() -> ImVec2 { return {0, 0}; }
};

struct GraphicalResource : public SizableResource {
    virtual ~GraphicalResource() {}
    virtual void Render(ImDrawList*, ImVec2 const& xy, ImU32 clr) {};
};

struct GlyphInfo {
    std::string_view Symbol = {};
    int FontTag = 0;
    static void RegisterFontTag(int tag, ImPlus::Font::Resource font);
};

struct Glyph {
    std::string Symbol;
    ImPlus::Font::Resource Font = {};
    float FontScale = 1.0f;
    std::optional<ImU32> Color = {};
    Glyph(GlyphInfo const& info);

    Glyph() {}

    explicit Glyph(
        std::string const& symbol, ImPlus::Font::Resource font = {}, float font_scale = 1.0f)
        : Symbol{symbol}
        , Font{font}
        , FontScale{font_scale}
    {
    }
    Glyph(Glyph const&) = default;
    Glyph(Glyph&&) = default;
    auto operator=(Glyph const&) -> Glyph& = default;
    auto operator=(Glyph&&) -> Glyph& = default;

    auto WithColor(ImU32 c) const -> Glyph
    {
        auto ret = *this;
        ret.Color = c;
        return ret;
    }
    auto WithColor(ImVec4 const& c) const -> Glyph { return WithColor(ImGui::GetColorU32(c)); }
    auto Scaled(float scale) const -> Glyph
    {
        auto ret = *this;
        ret.FontScale *= scale;
        return ret;
    }
};

struct IconBadge {
    std::string Content;
    ImPlus::Font::Resource Font = {};
    ImPlus::ColorSet ColorSet = ImPlus::ColorSet{
        {0, 0, 0, 1},
        {1, 1, 1, 1},
    };
};

using IconOverlay = std::variant<Glyph, IconBadge>;

struct CustomIconData {
    length width = 0_em; // auto-size
    length height = 1_em;
    std::function<void(ImDrawList*, ImVec2 const& bb_min, ImVec2 const& bb_max, ImU32 clr)> on_draw;
};

struct Icon {
public:
    enum class Builtin {
        Box,
        Circle,
        Spinner,
        Bullet,
        DotDotDot,
    };

    static auto Placeholder(length const& width, length const& height) -> CustomIconData;

private:
    struct builtin_content {
        Builtin shape = Builtin::Box;
        length size = 1_em;
    };

    using content_type =
        std::variant<std::monostate, Glyph, builtin_content, CustomIconData, GraphicalResource*>;

    content_type content_ = std::monostate{};
    std::vector<IconOverlay> overlays_;

public:
    Icon() noexcept = default;
    Icon(Icon const&) = default;
    Icon(Icon&&) = default;
    Icon(Glyph const& g)
        : content_{g}
    {
    }
    Icon(Glyph&& g)
        : content_{std::move(g)}
    {
    }
    Icon(GlyphInfo const& g)
        : content_{Glyph{g}}
    {
    }
    Icon(Builtin shape, length size = 1_em)
        : content_{builtin_content{shape, size}}
    {
    }
    Icon(CustomIconData const& c)
        : content_{c}
    {
    }
    Icon(CustomIconData&& c)
        : content_{std::move(c)}
    {
    }
    Icon(GraphicalResource& r)
        : content_{&r}
    {
    }

    auto operator=(Icon const&) -> Icon& = default;
    auto operator=(Icon&&) -> Icon& = default;

    constexpr auto Empty() const { return std::holds_alternative<std::monostate>(content_); };
    auto Measure() const -> ImVec2;
    void Draw(ImDrawList*, ImVec2 const& pos, ImU32 clr) const;

    auto operator+=(IconOverlay const& overlay) -> Icon&
    {
        overlays_.push_back(overlay);
        return *this;
    }
    auto operator+=(IconOverlay&& overlay) -> Icon&
    {
        overlays_.push_back(std::move(overlay));
        return *this;
    }
    auto operator+(IconOverlay const& overlay) const -> Icon
    {
        auto ret = *this;
        ret += overlay;
        return ret;
    }
    auto operator+(IconOverlay&& overlay) const -> Icon
    {
        auto ret = *this;
        ret += std::move(overlay);
        return ret;
    }
};

} // namespace ImPlus