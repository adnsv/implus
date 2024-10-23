#pragma once

#include <string>
#include <string_view>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

auto to_wstring(std::string_view s) -> std::wstring
{
    if (s.empty())
        return {};
    auto n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    if (!n)
        return {};
    auto ret = std::wstring{};
    ret.resize(n);
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), ret.data(), n);
    return ret;
}
auto to_string(std::wstring_view s) -> std::string
{
    if (s.empty())
        return {};
    auto n = ::WideCharToMultiByte(
        CP_THREAD_ACP, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    if (!n)
        return {};
    auto ret = std::string{};
    ret.resize(n);
    ::WideCharToMultiByte(
        CP_THREAD_ACP, 0, s.data(), int(s.size()), ret.data(), n, nullptr, nullptr);
    return ret;
}