#pragma once

#include <algorithm>
#include <concepts>
#include <imgui.h>

namespace ImPlus {

template <typename T>
concept rectangle = requires(T const& r) {
                        {
                            r.Min
                            } ;
                        {
                            r.Max
                            };
                    };

struct Interval {
    float Min = 0.0f;
    float Max = 0.0f;
    constexpr auto Width() const -> float { return Max - Min; }
    constexpr auto Clamp(float v) const
    {
        return Min < Max ? std::clamp(v, Min, Max) : std::clamp(v, Max, Min);
    }
};

struct Rect {
    ImVec2 Min = {0, 0};
    ImVec2 Max = {0, 0};
    constexpr Rect() noexcept = default;
    constexpr Rect(Rect const&) noexcept = default;
    constexpr Rect(ImVec2 const& tl, ImVec2 const& br) noexcept
        : Min{tl}
        , Max{br}
    {
    }
    constexpr Rect(rectangle auto const& other) noexcept
        : Min{other.Min}
        , Max{other.Max}
    {
    }
    constexpr Rect(float l, float t, float r, float b) noexcept
        : Min{l, t}
        , Max{r, b}
    {
    }
    constexpr auto operator=(Rect const&) noexcept -> Rect& = default;
    auto operator=(rectangle auto const& rhs) noexcept -> Rect&
    {
        Min = rhs.Min;
        Max = rhs.Max;
        return *this;
    }

    constexpr auto Empty() const { return Max.x <= Min.x || Max.y <= Min.y; }
    constexpr auto Width() const { return Max.x - Min.x; }
    constexpr auto Height() const { return Max.y - Min.y; }
    constexpr auto Horz() const { return Interval{Min.x, Max.x}; }
    constexpr auto Vert() const { return Interval{Min.y, Max.y}; }
    constexpr auto Size() const { return ImVec2{Max.x - Min.x, Max.y - Min.y}; }

    void Expand(float dx, float dy)
    {
        Min.x -= dx;
        Min.y -= dy;
        Max.x += dx;
        Max.y += dy;
    }
    void Expand(ImVec2 const d)
    {
        Min.x -= d.x;
        Min.y -= d.y;
        Max.x += d.x;
        Max.y += d.y;
    }
    constexpr auto Center() const
    {
        return ImVec2{
            (Min.x + Max.x) * 0.5f,
            (Min.y + Max.y) * 0.5f,
        };
    }
    constexpr auto Contains(ImVec2 const& pt) const -> bool
    {
        return Min.x <= pt.x && pt.x < Max.x && Min.y <= pt.y && pt.y < Max.y;
    }
    constexpr auto Clamp(ImVec2 const& pt) const -> ImVec2
    {
        return ImVec2{
            std::clamp(pt.x, Min.x, Max.x),
            std::clamp(pt.y, Min.y, Max.y),
        };
    }
    constexpr void Normalize()
    {
        if (Max.x < Min.x)
            std::swap(Max.x, Min.x);
        if (Max.y < Min.y)
            std::swap(Min.y, Max.y);
    }
};

struct Mapping {
    constexpr Mapping() noexcept = default;
    constexpr Mapping(Mapping const&) noexcept = default;
    constexpr Mapping(float offset, float scale) noexcept
        : offset_{offset}
        , scale_{scale}
    {
    }
    constexpr Mapping(Interval const& from, Interval const& to) noexcept
        : offset_{to.Min - from.Min * to.Width() / from.Width()}
        , scale_{to.Width() / from.Width()}
    {
    }

    constexpr auto Offset() const { return offset_; }
    constexpr auto Scale() const { return scale_; }
    constexpr auto Direct(float v) const { return offset_ + v * scale_; }
    constexpr auto Inverse(float v) const { return (v - offset_) / scale_; }
    constexpr auto Inverse() const -> Mapping
    {
        auto r = 1.0f / scale_;
        return Mapping{-offset_ * r, r};
    }
    constexpr auto Direct(Interval const& vv) const
    {
        return Interval{Direct(vv.Min), Direct(vv.Max)};
    }
    constexpr auto Inverse(Interval const& vv) const
    {
        return Interval{Inverse(vv.Min), Inverse(vv.Max)};
    }

    auto operator()(float v) const { return Direct(v); }
    auto operator()(Interval const& vv) const { return Direct(vv); }

    // operator* combines two consecutive mappings into one
    // (m1*m2)(v) = m2(m1(v))
    friend constexpr auto operator*(Mapping const& m1, Mapping const& m2) -> Mapping
    {
        return Mapping{m2.Direct(m1.offset_), m1.scale_ * m2.scale_};
    }

private:
    float offset_ = 0.0f;
    float scale_ = 1.0f;
};

struct Mapping2D {
    Mapping Horz;
    Mapping Vert;
    constexpr Mapping2D() noexcept = default;
    constexpr Mapping2D(Mapping2D const&) noexcept = default;
    constexpr Mapping2D(Mapping const& horz, Mapping const& vert) noexcept
        : Horz{horz}
        , Vert{vert}
    {
    }
    constexpr Mapping2D(Rect const& from, Rect const& to)
        : Horz{from.Horz(), to.Horz()}
        , Vert{from.Vert(), to.Vert()}
    {
    }

    constexpr auto Direct(ImVec2 const& v) const
    {
        return ImVec2{Horz.Direct(v.x), Vert.Direct(v.y)};
    }
    constexpr auto Inverse(ImVec2 const& v) const
    {
        return ImVec2{Horz.Inverse(v.x), Vert.Inverse(v.y)};
    }
    constexpr auto Inverse() const { return Mapping2D{Horz.Inverse(), Vert.Inverse()}; }
    constexpr auto operator()(ImVec2 const& v) const { return Direct(v); }
};

} // namespace ImPlus