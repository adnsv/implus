#include "win32-kbd.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// #include <hidpi.h>
#include <hidsdi.h>
#include <vector>

#include <Ntddkbd.h>
#include <SetupAPI.h>

namespace kbd {

auto FullKeyboardAttached() -> std::optional<bool>
{

    UINT deviceCount = 0;
    if (GetRawInputDeviceList(nullptr, &deviceCount, sizeof(RAWINPUTDEVICELIST)) != 0)
        return {};

    std::vector<RAWINPUTDEVICELIST> deviceList(deviceCount);
    if (GetRawInputDeviceList(deviceList.data(), &deviceCount, sizeof(RAWINPUTDEVICELIST)) !=
        deviceCount)
        return {};

    auto preparsedDataBuffer = std::vector<std::byte>{};

    auto full_kbd = false;

    for (auto const& device : deviceList) {
        if (device.dwType != RIM_TYPEKEYBOARD)
            continue;

        RID_DEVICE_INFO info = {};
        info.cbSize = sizeof(RID_DEVICE_INFO);
        UINT size = sizeof(RID_DEVICE_INFO);

        if (GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICEINFO, &info, &size) > 0) {
            if (info.keyboard.dwNumberOfKeysTotal > 83) {
                full_kbd = true;
            }
        }

        UINT preparsedDataSize = 1;
        if (GetRawInputDeviceInfo(
                device.hDevice, RIDI_PREPARSEDDATA, nullptr, &preparsedDataSize) != 0)
            continue;

        if (!preparsedDataSize)
            continue;

        UINT name_size = 0;
        UINT result = GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, nullptr, &name_size);
        if (result == (UINT)-1 || name_size == 0 || name_size > 256)
            continue;
        wchar_t name[256];
        GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, name, &name_size);

        auto f = ::CreateFileW(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0);
        if (f != INVALID_HANDLE_VALUE) {
            ::CloseHandle(f);
        }


#if 0
        UINT name_size = 0;
        UINT result = GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, nullptr, &name_size);
        if (result == (UINT)-1 || name_size == 0 || name_size > 256)
            continue;
        wchar_t name[256];
        GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, name, &name_size);

        UINT preparsedDataSize = 1;
        if (GetRawInputDeviceInfoW(
                device.hDevice, RIDI_PREPARSEDDATA, nullptr, &preparsedDataSize) != 0)
            continue;

        if (!preparsedDataSize)
            continue;

        preparsedDataBuffer.resize(preparsedDataSize);

        if (GetRawInputDeviceInfoW(device.hDevice, RIDI_PREPARSEDDATA, preparsedDataBuffer.data(),
                &preparsedDataSize) == -1)
            continue;

        auto preparsedData = reinterpret_cast<PHIDP_PREPARSED_DATA>(preparsedDataBuffer.data());
        HIDP_CAPS caps;
        if (HidP_GetCaps(preparsedData, &caps) != HIDP_STATUS_SUCCESS)
            continue;
#endif
    }
    return false;
}

#if 0    

auto FullKeyboardAttached() -> std::optional<bool>
{
    GUID hid_uid;
    HidD_GetHidGuid(&hid_uid);

    auto info_set =
        SetupDiGetClassDevsW(&hid_uid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info_set == INVALID_HANDLE_VALUE)
        return {};

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    auto detailDataBuffer = std::vector<std::byte>{};

    for (DWORD deviceIndex = 0; SetupDiEnumDeviceInterfaces(
             info_set, nullptr, &hid_uid, deviceIndex, &deviceInterfaceData);
         ++deviceIndex) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(
            info_set, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr);

        detailDataBuffer.resize(requiredSize);

        auto detailData =
            reinterpret_cast<SP_INTERFACE_DEVICE_DETAIL_DATA_W*>(detailDataBuffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(
                info_set, &deviceInterfaceData, detailData, requiredSize, nullptr, nullptr))
            continue;

        HANDLE hidDevice = CreateFileW(detailData->DevicePath, GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (hidDevice != INVALID_HANDLE_VALUE) {
            HIDD_ATTRIBUTES hidAttributes;
            PHIDP_PREPARSED_DATA preparsedData;

            if (HidD_GetAttributes(hidDevice, &hidAttributes) &&
                HidD_GetPreparsedData(hidDevice, &preparsedData)) {
                HIDP_CAPS capabilities;
                if (HidP_GetCaps(preparsedData, &capabilities) == HIDP_STATUS_SUCCESS) {
                    if (capabilities.UsagePage == HID_USAGE_PAGE_GENERIC &&
                        capabilities.Usage == HID_USAGE_GENERIC_KEYBOARD) {
                        HidD_FreePreparsedData(preparsedData);
                        CloseHandle(hidDevice);
                        SetupDiDestroyDeviceInfoList(info_set);
                        return true; // Full keyboard found
                    }
                }
                HidD_FreePreparsedData(preparsedData);
            }
            CloseHandle(hidDevice);
        }
    }

    SetupDiDestroyDeviceInfoList(info_set);
    return false; // No full keyboard found
}
#endif

#if 0
auto isFullKeyboard(RID_DEVICE_INFO_HID const& d) -> bool
{
    if (d.usUsagePage != HID_USAGE_PAGE_GENERIC)
        return false;

    // if (d.hid.usUsage == HID_USAGE_GENERIC_KEYPAD)
    //     return false;

    if (d.usUsage == HID_USAGE_GENERIC_KEYBOARD)
        return true;

    return false;
}

auto FullKeyboardAttached() -> std::optional<bool>
{
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    auto deviceInfoSet =
        SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return {};

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD deviceIndex = 0; SetupDiEnumDeviceInterfaces(
             deviceInfoSet, NULL, &hidGuid, deviceIndex, &deviceInterfaceData);
         ++deviceIndex) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

        std::vector<BYTE> detailDataBuffer(requiredSize);
        PSP_DEVICE_INTERFACE_DETAIL_DATA detailData =
            reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(detailDataBuffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData,
                requiredSize, NULL, &deviceInfoData)) {
            auto devInst = deviceInfoData.DevInst;
            DEVPROPTYPE propertyType;
            USHORT usage = 0, usagePage = 0;
            ULONG propertySize;

            CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_DeviceInterface_HID_UsagePage, &propertyType,
                reinterpret_cast<PBYTE>(&usagePage), &propertySize, 0);
            CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_DeviceInterface_HID_UsageId, &propertyType,
                reinterpret_cast<PBYTE>(&usage), &propertySize, 0);

            if (usagePage == HID_USAGE_PAGE_GENERIC &&
                (usage == HID_USAGE_GENERIC_KEYBOARD || usage == HID_USAGE_GENERIC_KEYPAD)) {
                RID_DEVICE_INFO deviceInfo;
                UINT cbSize = sizeof(deviceInfo);
                deviceInfo.cbSize = cbSize;

                HANDLE deviceHandle =
                    CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if (deviceHandle != INVALID_HANDLE_VALUE) {
                    UINT result = GetRawInputDeviceInfo(reinterpret_cast<HANDLE>(deviceHandle),
                        RIDI_DEVICEINFO, &deviceInfo, &cbSize);
                    if (result != static_cast<UINT>(-1)) {
                        if (isFullKeyboard(deviceInfo.keyboard, usage, usagePage)) {
                            info.fullKeyboardCount++;
                        }
                        else {
                            info.keypadCount++;
                        }
                    }
                    CloseHandle(deviceHandle);
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
}
#endif

#if 0

DEFINE_DEVPROPKEY(DEVPKEY_Device_HID_UsagePage, 0x4d1ebee, 0xf4ca, 0x11d1, 0x00, 0x00, 0xf8, 0x79,
    0xf8, 0x06, 0x00, 0x30, 1);
DEFINE_DEVPROPKEY(DEVPKEY_Device_HID_UsageId, 0x4d1ebee, 0xf4ca, 0x11d1, 0x00, 0x00, 0xf8, 0x79,
    0xf8, 0x06, 0x00, 0x30, 2);

bool GetHidUsage(
    SP_DEVINFO_DATA& deviceInfoData, HDEVINFO& deviceInfoSet, USAGE& usagePage, USAGE& usageId)
{
    DEVPROPTYPE propType;
    DWORD requiredSize = 0;

    // Query the Device Property for HID Usage Page
    if (SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_HID_UsagePage,
            &propType, reinterpret_cast<PBYTE>(&usagePage), sizeof(usagePage), &requiredSize, 0)) {
        // Query the Device Property for HID Usage ID
        if (SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_HID_UsageId,
                &propType, reinterpret_cast<PBYTE>(&usageId), sizeof(usageId), &requiredSize, 0)) {
            return true;
        }
    }
    return false;
}

auto DetectFullKeyboard() -> std::optional<bool>
{
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_KEYBOARD, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return {};
    }

    SP_DEVICE_INTERFACE_DATA interfaceData = {};
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    SP_DEVINFO_DATA deviceInfoData = {};
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    USAGE usagePage = 0, usageId = 0;
    bool hasKeyboard = false;

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(
             deviceInfoSet, nullptr, &GUID_DEVINTERFACE_KEYBOARD, i, &interfaceData);
         ++i) {
        if (SetupDiGetDeviceInterfaceDetailW(
                deviceInfoSet, &interfaceData, nullptr, 0, nullptr, &deviceInfoData)) {
            if (GetHidUsage(deviceInfoData, deviceInfoSet, usagePage, usageId)) {
                if (usagePage == HID_USAGE_PAGE_GENERIC) {
                    if (usageId == HID_USAGE_GENERIC_KEYBOARD) {
                        hasKeyboard = true;
                        break;
                    }
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return hasKeyboard;
}

#endif

} // namespace kbd