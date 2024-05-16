#pragma once

#include "host.hpp"
#include "settings.hpp"
#include <optional>

namespace ImPlus::Settings {

template <> struct binding_impl<Host::Window::Location> {
    void write_all_fields(writer const& w, Host::Window::Location const& loc)
    {
        if (loc.Pos)
            w.writef("Pos", "%d,%d", loc.Pos->x, loc.Pos->y);
        if (loc.Size)
            w.writef("Size", "%d,%d", loc.Size->w, loc.Size->h);
        if (loc.Maximized == true)
            w.write("State", "Maximized");
        if (loc.FullScreen == true)
            w.write("FullScreen", "Windowed");
    }

    void read_field(
        std::string_view key, std::string_view s, Host::Window::Location& loc)
    {
        if (key == "Pos") {
            int x, y;
            if (sscanf(s.data(), "%i,%i", &x, &y) == 2)
                loc.Pos = {x, y};
        }
        else if (key == "Size") {
            int w, h;
            if (sscanf(s.data(), "%i,%i", &w, &h) == 2)
                loc.Size = {w, h};
        }
        else if (key == "State") {
            loc.Maximized = (s == "Maximized");
        }
        else if (key == "FullScreen") {
            loc.FullScreen = (s == "Windowed");
        }
    }
};

} // namespace ImPlus::Settings