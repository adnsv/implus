#pragma once

#include "font-ranges.hpp"
#include <cstddef>
#include <filesystem>
#include <imgui.h>
#include <initializer_list>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace ImPlus::Font {

// Resource refers to a loaded font face
//
// - supports casting to ImFont*
// - can be used with ImGui::PushFont()
//
struct Resource {
    std::size_t view_id = 0;
    auto imfont() const -> ImFont*;
    operator ImFont*() const { return imfont(); }
    operator bool() const { return view_id != 0; }
};

using percent = float;

struct Adjustment {
    percent Size = 100.0f;
    percent BiasHorz = 0.0f;
    percent BiasVert = 0.0f;
};

inline auto Adjust = std::unordered_map<std::string, Adjustment>{};

void SetBuilderFlags(unsigned int flags);

struct NameInfo {
    std::string Name;
    float PointSize;
};
auto GetDefaultInfo() -> NameInfo;

struct FileInfo {
    std::filesystem::path Filename;
    int FaceIndex = 0;
};

struct BlobInfo {
    std::span<std::byte const> Blob;
    int FaceIndex = 0;

    BlobInfo(std::span<std::byte const> const& data, int face_index = 0)
        : Blob{data}
        , FaceIndex{face_index}
    {
    }

    template <typename T>
    requires std::is_convertible_v<T, std::span<unsigned char const>>
    BlobInfo(T const& data, int face_index = 0)
        : BlobInfo{std::as_bytes(std::span{data}), face_index}
    {
    }
};

auto GetDataBlob(char const* facename) -> BlobInfo;

inline auto Regular = Resource{};

// LoadDefaults tries to load host's default GUI font
auto LoadDefault() -> Resource;

auto Load(FileInfo const&, std::initializer_list<range> ranges, float point_size,
    Adjustment const& = {}) -> Resource;

auto Load(BlobInfo const&, std::initializer_list<range> ranges, float point_size,
    Adjustment const& = {}) -> Resource;

// CreateScaledFont creates scaled subset of a previously loaded font
auto CreateScaled(Resource h, float scale_factor, std::initializer_list<range> ranges = {})
    -> Resource;

// SetMergeMode indicates that the font needs to be merged with the previously
// loaded handle (this effectively implements as glyph fallback)
void SetMergeMode(Resource h, bool mergeWithPrev = true);

// Setup configures scales of all loaded font resources to match
// the specified DPI
auto Setup(float dpi, float oversample) -> bool;

} // namespace ImPlus::Font