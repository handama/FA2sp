#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

enum class WindowsVersion : int
{
    Unknown = 0,
    XP = 1,
    Server2003 = 2,
    Vista = 3,
    Server2008 = 4,
    Win7 = 5,
    Win8 = 6,
    Win81 = 7,
    Win10 = 8,
    Win11 = 9,
    Future = 10
};

struct WindowsOSInfo
{
    bool          isValid = false;
    std::string   friendlyName;
    std::string   versionString;
    DWORD         majorVersion = 0;
    DWORD         minorVersion = 0;
    DWORD         buildNumber = 0;
    DWORD         revision = 0;
    std::string   csdVersion;

    WindowsVersion simpleVersion = WindowsVersion::Unknown;

    bool IsWindowsVistaOrGreater() const {
        return isValid && majorVersion >= 6;
    }
    bool IsWindows7OrGreater() const {
        if (!isValid) return false;
        return (majorVersion > 6) ||
            (majorVersion == 6 && minorVersion >= 1);
    }

    bool IsWindows10OrGreater() const {
        return isValid && majorVersion >= 10;
    }

    bool IsWindows11OrGreater() const {
        return isValid && majorVersion == 10 && buildNumber >= 22000;
    }

    static WindowsOSInfo GetDetailedWindowsVersion();
};
