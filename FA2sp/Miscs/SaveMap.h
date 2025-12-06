#pragma once

#include <Helpers/Macro.h>

#include "../FA2sp.h"
#include <optional>
#include <filesystem>
#include <CFinalSunDlg.h>

class SaveMapExt
{
private:
    static UINT_PTR Timer;
public:
    static bool SaveMap(CINI* pINI, CFinalSunDlg* pFinalSun, FString filepath, int previewOption, bool showDialog);
    static bool SaveMapSilent(FString filepath, bool panic = false);
    static void ResetTimer();
    static void StopTimer();
    static void RemoveEarlySaves();
    static void CALLBACK SaveMapCallback(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime);

    static bool IsAutoSaving;
    static std::optional<std::filesystem::file_time_type> SaveTime;
};