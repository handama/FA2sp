#include "WinVer.h"

WindowsOSInfo WindowsOSInfo::GetDetailedWindowsVersion()
{
    WindowsOSInfo info;

    typedef LONG(NTAPI* pRtlGetVersion)(PRTL_OSVERSIONINFOW);

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        return info;
    }

    pRtlGetVersion pRtl = reinterpret_cast<pRtlGetVersion>(
        GetProcAddress(hNtdll, "RtlGetVersion")
        );

    if (!pRtl) {
        return info;
    }

    RTL_OSVERSIONINFOW rtlVer = { sizeof(rtlVer) };
    if (pRtl(&rtlVer) != 0) {
        return info;
    }

    info.isValid = true;
    info.majorVersion = rtlVer.dwMajorVersion;
    info.minorVersion = rtlVer.dwMinorVersion;
    info.buildNumber = rtlVer.dwBuildNumber;

    if (rtlVer.szCSDVersion[0] != L'\0') {
        int len = WideCharToMultiByte(CP_ACP, 0, rtlVer.szCSDVersion, -1, nullptr, 0, nullptr, nullptr);
        std::string csd(len - 1, 0);
        WideCharToMultiByte(CP_ACP, 0, rtlVer.szCSDVersion, -1, &csd[0], len, nullptr, nullptr);
        info.csdVersion = csd;
    }

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD ubr = 0;
        DWORD size = sizeof(ubr);
        if (RegQueryValueExW(hKey, L"UBR", nullptr, nullptr, (LPBYTE)&ubr, &size) == ERROR_SUCCESS) {
            info.revision = ubr;
        }
        RegCloseKey(hKey);
    }

    std::ostringstream friendly;

    if (info.majorVersion == 5 && info.minorVersion == 1) {
        info.simpleVersion = WindowsVersion::XP;
        friendly << "Windows XP";
    }
    else if (info.majorVersion == 6 && info.minorVersion == 0) {
        info.simpleVersion = WindowsVersion::Vista;
        friendly << "Windows Vista";
    }
    else if (info.majorVersion == 6 && info.minorVersion == 1) {
        info.simpleVersion = WindowsVersion::Win7;
        friendly << "Windows 7";
    }
    else if (info.majorVersion == 6 && info.minorVersion == 2) {
        info.simpleVersion = WindowsVersion::Win8;
        friendly << "Windows 8";
    }
    else if (info.majorVersion == 6 && info.minorVersion == 3) {
        info.simpleVersion = WindowsVersion::Win81;
        friendly << "Windows 8.1";
    }
    else if (info.majorVersion == 10 && info.minorVersion == 0) {
        if (info.buildNumber >= 22000) {
            info.simpleVersion = WindowsVersion::Win11;
            friendly << "Windows 11";
        }
        else {
            info.simpleVersion = WindowsVersion::Win10;
            friendly << "Windows 10";
        }
    }
    else if (info.majorVersion > 10) {
        info.simpleVersion = WindowsVersion::Future;
        friendly << "Windows (future version " << info.majorVersion << ")";
    }
    else {
        friendly << "Unknown Windows";
    }
    info.friendlyName = friendly.str();

    std::ostringstream verStr;
    verStr << info.majorVersion << "." << info.minorVersion << "." << info.buildNumber;
    if (info.revision > 0) verStr << "." << info.revision;
    info.versionString = verStr.str();

    return info;
}