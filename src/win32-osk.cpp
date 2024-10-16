#include "win32-osk.hpp"

#include <ShlObj.h>
#include <Shobjidl.h>

const CLSID CLSID_UIHostNoLaunch = {
    0x4CE576FA, 0x83DC, 0x4f88, {0x95, 0x1C, 0x9D, 0x07, 0x82, 0xB4, 0xE3, 0x76}};

const IID IID_ITipInvocation = {
    0x37c994e7, 0x432b, 0x4834, {0xa2, 0xf7, 0xdc, 0xe1, 0xf1, 0x3b, 0x83, 0x4b}};

struct ITipInvocation : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Toggle(HWND wnd) = 0;
};

namespace osk {

static auto LaunchTabTip() -> bool
{
    DWORD pid = 0;
    auto su = STARTUPINFOW{sizeof(STARTUPINFOW)};
    su.dwFlags = STARTF_USESHOWWINDOW;
    su.wShowWindow = SW_HIDE;
    auto stat = PROCESS_INFORMATION{};
    const wchar_t* path = L"%CommonProgramW6432%\\microsoft shared\\ink\\TabTIP.EXE";
    wchar_t buf[512];
    ::SetEnvironmentVariableW(L"__compat_layer", L"RunAsInvoker");
    ::ExpandEnvironmentStringsW(path, buf, 512);

    if (::CreateProcessW(0, buf, 0, 0, 1, 0, 0, 0, &su, &stat)) {
        pid = stat.dwProcessId;
    }
    ::CloseHandle(stat.hProcess);
    ::CloseHandle(stat.hThread);
    return pid != 0;
}

auto GetInputPaneRect(RECT& r) -> bool
{
    IFrameworkInputPane* inputPane = NULL;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_FrameworkInputPane, NULL, CLSCTX_INPROC_SERVER,
            IID_IFrameworkInputPane, (LPVOID*)&inputPane);
        if (SUCCEEDED(hr)) {
            hr = inputPane->Location(&r);
            if (!SUCCEEDED(hr)) {}
            inputPane->Release();
        }
    }
    CoUninitialize();
    return SUCCEEDED(hr);
}

auto IsInputPaneVisible() -> bool
{
    RECT r;
    bool rect_ok = GetInputPaneRect(r);
    return rect_ok && (r.right > r.left) && (r.bottom > r.top);
}

auto ToggleInputPaneVisibility() -> HRESULT
{
    auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return hr;

    // Create an instance of the ITipInvocation interface
    ITipInvocation* invoker = nullptr;
    hr = CoCreateInstance(CLSID_UIHostNoLaunch, 0, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_ITipInvocation, (void**)&invoker);

    if (hr == REGDB_E_CLASSNOTREG) {
        if (LaunchTabTip()) {
            hr = CoCreateInstance(CLSID_UIHostNoLaunch, 0,
                CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_ITipInvocation, (void**)&invoker);
        }
    }

    if (SUCCEEDED(hr)) {
        hr = invoker->Toggle(GetDesktopWindow());
        invoker->Release();
    }

    CoUninitialize();
    return hr;
}

} // namespace osk