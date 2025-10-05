#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
enum AnnotationType : int {
    AnnoTrigger = 0,
    AnnoTeam,
    AnnoScript,
    AnnoTaskforce,
    AnnoAITrigger,
    AnnoTag,
};
// A static window class
class CTriggerAnnotation
{
public:
    static void Create(CFinalSunDlg* pWnd);
    enum Controls {
        Text = 1000,
        Edit = 1001,
    };

    static HWND GetHandle()
    {
        return CTriggerAnnotation::m_hwnd;
    }

    static FString ID;
    static AnnotationType Type;

protected:
    static void Initialize(HWND& hWnd);
    static void Update(const char* filter = nullptr);
    static void Close(HWND& hWnd);
    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void OnEditchangeEdit();

private:
    static HWND m_hwnd;
    static HWND hText;
    static HWND hEdit;
    static CFinalSunDlg* m_parent;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet;
};

