#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <cmath>
#include <implus/touchscroll.hpp>

/*
TODO:
- check imgui.cpp: just_scrolled_manually_while_resizing

*/

namespace ImPlus {

static auto magnitude(ImVec2 const& vec) -> float
{
    return std::sqrt(vec.x * vec.x + vec.y * vec.y);
}

static auto clampMagnitude(ImVec2 const& vec, float max_len) -> ImVec2
{
    auto v2 = vec.x * vec.x + vec.y * vec.y;
    if (v2 > max_len * max_len) {
        auto const mag = std::sqrt(v2);
        auto const scale = max_len / mag;
        return ImVec2{vec.x * scale, vec.y * scale};
    }
    return vec;
}

struct KineticScroller {
    using time_point = double;
    using time_interval = double;

private:
    enum State {
        Idle,
        Pending,
        Moving,
        Kinetic,
    };

    float deceleration = 0.92f;
    float maxVelocity = 4000.0f;
    float moveThreshold = 24.0f;   //
    time_interval moveTimeout = 0; //

    State state = Idle;
    ScrollDirection direction = ScrollBoth;
    ImVec2 velocity = {0, 0};
    time_point initialTouchTime = {0};
    time_point lastUpdateTime = {0};
    ImVec2 initialTouchPosition = {};
    ImVec2 lastTouchPosition = {0, 0};

    static auto now() -> time_point
    {
        return ImGui::GetTime();
    }

    auto filterDisplacement(ImVec2 d) -> ImVec2
    {
        switch (direction) {
        case ScrollHorz:
            return {d.x, 0.0f};
        case ScrollVert:
            return {0.0f, d.y};
        default:
            return d;
        }
    }

public:
    void TouchBegan(
        ImVec2 const& pos, float move_threshold, float move_timeout, ScrollDirection dir)
    {
        moveThreshold = move_threshold;
        moveTimeout = move_timeout;
        state = Pending;
        direction = dir;
        velocity = {0.0f, 0.0f};
        initialTouchPosition = pos;
        initialTouchTime = now();
        lastTouchPosition = initialTouchPosition;
        lastUpdateTime = initialTouchTime;
    }

    auto TouchMoved(ImVec2 const& pos, bool* started_moving) -> ImVec2
    {
        if (started_moving)
            *started_moving = false;
        auto const t = now();
        auto const deltaTime = time_interval(t - lastUpdateTime);
        if (deltaTime <= 0)
            return {};

        if (state == Pending) {
            if (moveTimeout > 0 && time_interval(t - initialTouchTime) > moveTimeout) {
                state = Idle;
                return {};
            }
            auto const d = filterDisplacement(pos - initialTouchPosition);
            if (magnitude(d) > moveThreshold) {
                if (started_moving)
                    *started_moving = true;
                state = Moving;
            }
        }

        auto const displacement = filterDisplacement(pos - lastTouchPosition);
        velocity = clampMagnitude(displacement * (1.0f / deltaTime), maxVelocity);

        lastTouchPosition = pos;
        lastUpdateTime = t;

        return state == Pending ? ImVec2{0, 0} : displacement;
    }

    void TouchEnded()
    {
        if (state == Moving)
            state = Kinetic;
        else
            state = Idle;
    }

    void Stop()
    {
        state = Idle;
    }

    auto UpdateScrollOffset() -> ImVec2
    {
        if (state != Kinetic)
            return {0, 0};

        auto const t = now();
        auto const deltaTime = time_interval(t - lastUpdateTime);

        if (deltaTime <= 0)
            return {0, 0};

        auto deltaOffset = velocity * deltaTime;

        // Apply frame-rate independent deceleration
        float decelerationFactor = std::pow(deceleration, deltaTime * 60.0f);
        velocity = velocity * decelerationFactor;

        // Stop scrolling if velocity is very low
        if (std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y) < 1.0f) {
            velocity = {0, 0};
            state = Idle;
        }

        lastUpdateTime = t;
        return deltaOffset;
    }

    auto IsIdle() const
    {
        return state == Idle;
    }

    auto IsPending() const
    {
        return state == Pending;
    }

    auto IsMoving() const
    {
        return state == Moving;
    }

    auto IsKinetic() const
    {
        return state == Kinetic;
    }
};

static KineticScroller TouchScroller;
ImGuiWindow* TouchScrollingWindow = nullptr;

static float move_threshold_mouse = 6;
static float move_threshold_pen = 8;
static float move_threshold_touch = 16;
static float move_timeout = 2.0;               // seconds
static float move_timeout_active_child = 0.25; // seconds when clicked on active child

auto StopTouchScrolling()
{
    TouchScroller.Stop();
    if (TouchScrollingWindow) {
        if (ImGui::GetKeyOwner(ImGuiKey_MouseLeft) == TouchScrollingWindow->ID)
            ImGui::SetKeyOwner(ImGuiKey_MouseLeft, ImGuiKeyOwner_NoOwner);
        TouchScrollingWindow = nullptr;
    }
}

void UpdateTouchScrolling()
{
    auto& g = *ImGui::GetCurrentContext();

    if (TouchScroller.IsIdle())
        StopTouchScrolling();

    if (TouchScrollingWindow && !TouchScrollingWindow->WasActive)
        TouchScrollingWindow = nullptr;

    if (!TouchScrollingWindow)
        return;

    auto drag_delta = ImVec2{0, 0};

    if (g.IO.MouseReleased[ImGuiMouseButton_Left]) {
        TouchScroller.TouchEnded();
    }
    else if (g.IO.MouseDown[ImGuiMouseButton_Left]) {
        if (TouchScroller.IsPending()) {
            auto started_moving = false;
            drag_delta = TouchScroller.TouchMoved(ImGui::GetMousePos(), &started_moving);
            if (TouchScroller.IsIdle())
                StopTouchScrolling();
            if (started_moving)
                ImGui::SetKeyOwner(ImGuiKey_MouseLeft, TouchScrollingWindow->ID);
        }
        else if (TouchScroller.IsMoving()) {
            if (ImGui::GetKeyOwner(ImGuiKey_MouseLeft) == TouchScrollingWindow->ID) {
                drag_delta = TouchScroller.TouchMoved(ImGui::GetMousePos(), nullptr);
                if (TouchScroller.IsIdle())
                    StopTouchScrolling();
            }
            else {
                StopTouchScrolling();
            }
        }
        else {
            StopTouchScrolling();
        }
    }
    else if (TouchScroller.IsPending() && TouchScroller.IsMoving()) {
        StopTouchScrolling();
        return;
    }

    if (TouchScroller.IsKinetic()) {
        drag_delta = TouchScroller.UpdateScrollOffset();
        if (!drag_delta.x && !drag_delta.y)
            StopTouchScrolling();
    }

    if (TouchScrollingWindow) {
        if (drag_delta.x)
            ImGui::SetScrollX(TouchScrollingWindow, TouchScrollingWindow->Scroll.x - drag_delta.x);
        if (drag_delta.y)
            ImGui::SetScrollY(TouchScrollingWindow, TouchScrollingWindow->Scroll.y - drag_delta.y);
    }
}

static bool context_hooked = false;
static auto hook_TouchScrolling()
{
    if (!context_hooked) {
        context_hooked = true;
        auto h = ImGuiContextHook{};
        h.Type = ImGuiContextHookType_NewFramePost;
        h.Callback = [](ImGuiContext*, ImGuiContextHook*) { UpdateTouchScrolling(); };
        ImGui::AddContextHook(ImGui::GetCurrentContext(), &h);
    }
}

auto CalcSupportedScrolling(ImGuiWindow const* window) -> ScrollDirection
{
    auto const& m = window->ScrollMax;
    if (m.x > 0 && m.y > 0)
        return ScrollBoth;
    else if (m.x > 0)
        return ScrollHorz;
    else if (m.y > 0)
        return ScrollVert;
    else
        return ScrollNone;
}

void HandleTouchScroll(TouchScrollFlags flags)
{
    if (!TouchScrollingWindow) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left, false)) {

            auto move_threshold = 6.0f;
            switch (ImGui::GetIO().MouseSource) {
            case ImGuiMouseSource_Mouse:
                if (!(flags & TouchScrollFlags_EnableMouseScrolling)) {
                    StopTouchScrolling();
                    return;
                }
                move_threshold = move_threshold_mouse;
                break;
            case ImGuiMouseSource_Pen:
                if (!(flags & TouchScrollFlags_EnablePenScrolling)) {
                    StopTouchScrolling();
                    return;
                }
                move_threshold = move_threshold_pen;
                break;
            case ImGuiMouseSource_TouchScreen:
                if (flags & TouchScrollFlags_DisableTouchScrolling) {
                    StopTouchScrolling();
                    return;
                }
                move_threshold = move_threshold_touch;
                break;
            }

            auto window = ImGui::GetCurrentWindow();
            auto const dir = CalcSupportedScrolling(window);
            if (dir == ScrollNone)
                return;

            auto const pos = ImGui::GetMousePos();
            if (!window->InnerRect.Contains(pos))
                return;

            auto hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
            auto active_child_hovered =
                !hovered && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows |
                                                   ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            if (hovered) {
                hook_TouchScrolling();
                TouchScrollingWindow = window;
                TouchScroller.TouchBegan(pos, move_threshold, move_timeout, dir);
            }
            else if (active_child_hovered) {
                if (flags & TouchScrollFlags_DisableOnActiveChild)
                    return;
                hook_TouchScrolling();
                TouchScrollingWindow = window;
                TouchScroller.TouchBegan(pos, move_threshold, move_timeout_active_child, dir);
            }
        }
    }
}

} // namespace ImPlus