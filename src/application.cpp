#include <imgui_internal.h>

#include <chrono>
#include <implus/application.hpp>
#include <implus/dlg.hpp>
#include <thread>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace ImPlus::Application {

#ifdef __EMSCRIPTEN__
struct loop_args {
    ImPlus::Host::Window& w;
    std::function<void()>& on_render;
};

void emscripten_loop(void*);
#endif

void Base::Run(std::function<void()> on_render)
{
    auto& w = main_wnd_;

    Dlg::internal::Initialize();

    auto render_frame = [&](bool poll_events) {
        w.NewFrame(poll_events);

        for (auto&& cb : Callbacks::BeforeFrame)
            cb();

        if (on_render)
            on_render();

        for (auto&& cb : Callbacks::RenderFrame)
            cb();

        w.RenderFrame();

        // execute callbacks scheduled at the end of each frame
        for (auto&& cb : Callbacks::AfterEachFrame)
            cb();

        for (auto&& cb : Callbacks::AfterThisFrame)
            cb();

        Callbacks::AfterThisFrame.clear();
    };

    w.OnRefresh = [&]() { render_frame(false); };

    // To avoid initial flicker, pre-render first frame before the
    // window is shown
    ImPlus::Visuals::SetupFrame(w.ContentScale());
#ifndef __EMSCRIPTEN__
    render_frame(false);
    w.Show();
#endif

#ifdef __EMSCRIPTEN__
    auto args = loop_args{w, on_render};
    emscripten_set_main_loop_arg(emscripten_loop, &args, 0, true);

#else
    while (!w.ShouldClose()) {
        if (ImPlus::Visuals::SetupFrame(w.ContentScale())) {
            // SetupFrame returns true if one of the following has changed:
            // - Visuals::Zoom
            // - window.Scale
            // - theme
            //
            // If you have cached resources that depend on these properties
            // update or invalidate those here
        }

        render_frame(true);

        if (w.IsMinimized())
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
#endif
}

#ifdef __EMSCRIPTEN__
void emscripten_loop(void* args_ptr)
{
    auto& args = *reinterpret_cast<loop_args*>(args_ptr);

    if (ImPlus::Visuals::SetupFrame(args.w.ContentScale())) {
        // SetupFrame returns true if one of the following has changed:
        // - Visuals::Zoom
        // - window.Scale
        // - theme
        //
        // If you have cached resources that depend on these properties
        // update or invalidate those here
    }

    args.w.NewFrame(true);

    for (auto&& cb : Callbacks::BeforeFrame)
        cb();

    if (args.on_render)
        args.on_render();

    for (auto&& cb : Callbacks::RenderFrame)
        cb();

    args.w.RenderFrame();

    // execute callbacks scheduled at the end of each frame
    for (auto&& cb : Callbacks::AfterEachFrame)
        cb();

    for (auto&& cb : Callbacks::AfterThisFrame)
        cb();

    Callbacks::AfterThisFrame.clear();
}
#endif

} // namespace ImPlus::Application