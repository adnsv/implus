#include "implus/font.hpp"
#include "internal/font-engine.hpp"

#include <cmath>
#include <fstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ImPlus::Font {

namespace buffer {
// file buffers
using blob = std::vector<char>;
using view = std::span<std::byte const>;

struct entry {
    buffer::blob blob;
};

auto map = std::unordered_map<std::string, entry>{};

auto read_file(std::filesystem::path const& fn) -> blob
{
    auto f = std::ifstream{};
    f.open(fn, std::ios::binary | std::ios::in);
    if (f.good())
        return blob{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
    else
        return {};
}

auto get(std::string const& fn) -> view
{
    auto& entry = map[fn];
    if (entry.blob.empty())
        entry.blob = read_file(fn);
    if (entry.blob.empty())
        return {};
    return {(std::byte const*)entry.blob.data(), entry.blob.size()};
}

} // namespace buffer

auto make_rangeset(range const* first, range const* last) -> range::codepoint*
{
    using rangeset = std::vector<range>;
    static range empty[2] = {range{0, 0}, range{0, 0}};

    if (first == last)
        return &empty[0].lo;

    static std::vector<rangeset> rangesets;
    auto rs = rangeset{first, last};
    rs.push_back(range{0, 0});
    rangesets.push_back(std::move(rs));
    auto& b = rangesets.back().front();
    return &b.lo;
}

// resource_data extends ImFontConfig with a couple of additional
// fields that help to auto-calculate dynamic DPI-dependent font
// attributes.
struct resource_data : public ImFontConfig {
    // size_t ResourceIndex = 0;
    float point_size = 12.0f;
    percent bias_horz = 0.0f; // percentage, relative
    percent bias_vert = 0.0f; // percentage, relative
    ImFont* font = nullptr;
};

std::vector<resource_data> views;

#ifdef IMGUI_ENABLE_FREETYPE
unsigned int font_builder_flags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
#else
unsigned int font_builder_flags = 0;
#endif

void SetBuilderFlags(unsigned int flags) { font_builder_flags = flags; }

auto range_set_from(face const& ff) -> range::codepoint*
{
    auto ranges = std::vector<range>{};

    auto rstart = uint16_t{0};
    for (unsigned cp = 32; cp < 65536; ++cp) {
        if (ff.glyph_index_of(cp) != face::nglyph) {
            if (!rstart)
                rstart = cp;
        }
        else if (rstart) {
            ranges.push_back(range{rstart, uint16_t(cp - 1)});
            rstart = 0;
        }
    }

    return make_rangeset(ranges.data(), ranges.data() + ranges.size());
}

auto calc_vert_bias(face const& ff) -> float
{
    auto vm = ff.get_metrics();
    if (!vm)
        return 0;

    auto top = 0;
    auto bot = 0;
    auto n = 0;

    auto handle = [&](unsigned cp) {
        if (auto m = ff.codepoint_vmetrics(cp)) {
            ++n;
            if (m->ascender > top)
                top = m->ascender;
            if (m->descender < bot)
                bot = m->descender;
        }
    };

    handle('X');
    handle('(');
    handle('[');
    handle('/');
    handle('g');
    handle('j');
    handle('f');

    if (n <= 0)
        return 0.0f;

    auto old_height = vm->ascender + vm->descender;
    auto new_height = top + bot;
    auto old_ratio = float(vm->ascender) / old_height;
    auto new_ratio = float(top) / new_height;

    return 100.0f * (old_ratio - new_ratio) * new_height / old_height;
}

auto Load(BlobInfo const& bi, std::initializer_list<range> ranges, float point_size,
    Adjustment const& adj) -> Resource
{
    auto ff = face{bi};
    auto fm = ff.get_metrics();

    auto& rv = views.emplace_back();
    rv.point_size = point_size * adj.Size / 100.0f;
    rv.bias_horz = adj.BiasHorz;
    rv.bias_vert = calc_vert_bias(ff) + adj.BiasVert;
    rv.FontDataOwnedByAtlas = false; // <- important !

    rv.FontData = (void*)(bi.Blob.data());
    rv.FontDataSize = static_cast<int>(bi.Blob.size());

    rv.FontNo = 0;
    rv.SizePixels = 12.0f;
    if (ranges.size() == 0)
        rv.GlyphRanges = range_set_from(ff);
    else
        rv.GlyphRanges = make_rangeset(ranges.begin(), ranges.end());
    rv.FontBuilderFlags = font_builder_flags;
    rv.PixelSnapH = 1; // snap all glyphs to pixel grid

    auto id = views.size();
    auto s = ff.get_name();
    snprintf(rv.Name, sizeof(rv.Name), "%s", s.c_str());

    return {id};
}

auto Load(FileInfo const& fi, std::initializer_list<range> ranges, float point_size,
    Adjustment const& adj) -> Resource
{
    auto const b = buffer::get(fi.Filename.string());
    if (b.empty())
        return {};

    auto const fbi = BlobInfo{b, fi.FaceIndex};

    return Load(fbi, ranges, point_size, adj);
}

float lastDpi = 0.0f;
float lastOversample = 0.0f;

auto CreateScaled(Resource h, float scale_factor, std::initializer_list<range> ranges) -> Resource
{
    if (!h.view_id || h.view_id > views.size())
        return {};

    auto t = views[h.view_id - 1];
    t.point_size *= scale_factor;
    t.SizePixels *= scale_factor;
    t.GlyphOffset.x *= scale_factor;
    t.GlyphOffset.y *= scale_factor;
    if (ranges.size() != 0)
        t.GlyphRanges = make_rangeset(ranges.begin(), ranges.end());
    t.MergeMode = false;

    views.push_back(std::move(t));
    return Resource{views.size()};
}

void SetMergeMode(Resource h, bool mergeWithPrev)
{
    if (!h.view_id || h.view_id > views.size())
        return;
    views[h.view_id - 1].MergeMode = true;
}

auto Resource::imfont() const -> ImFont*
{
    if (!view_id || view_id > views.size())
        return nullptr;
    return views[view_id - 1].font;
}

auto Setup(float dpi, float oversample) -> bool
{
    if (dpi < 1.0f)
        dpi = 1.0f;
    if (oversample < 1.0f)
        oversample = 1.0f;

    if (lastDpi == dpi && lastOversample == oversample)
        return false;

    lastDpi = dpi;
    lastOversample = oversample;
    auto& io = ImGui::GetIO();

    auto scale_factor = dpi / 72.0f;
    io.Fonts->Clear();
    auto first = true;
    for (auto& v : views) {
        v.OversampleH = v.PixelSnapH ? oversample : 3 * oversample;
        v.OversampleV = oversample;

        auto pixel_size = v.point_size * scale_factor;
        v.SizePixels = std::round(pixel_size);

        v.GlyphOffset = {std::round(pixel_size * v.bias_horz / 100.0f),
            std::round(pixel_size * v.bias_vert / 100.0f)};

        if (first)
            v.MergeMode = false;
        first = false;
        v.font = io.Fonts->AddFont(&v);
    }

    return true;
}

} // namespace ImPlus::Font
