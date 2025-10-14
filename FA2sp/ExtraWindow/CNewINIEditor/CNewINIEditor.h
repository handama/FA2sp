#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/STDHelpers.h"

// A static window class
class CNewINIEditor
{
public:
    enum Controls {
        SearchText = 1404,
        SectionList = 1405,
        NewSectionName = 1407,
        INIEdit = 1406,
        NewButton = 1401,
        DeleteButton = 1402,
        ImportButton = 1403, 
        ImportTextButton = 1409, 
        ImporterDesc = 1410, 
        ImporterOK = 1412, 
        ImporterText = 1411, 
    };
    enum GameObject {
        Terrain, Waypoints, Smudge, Structures, Units, Aircraft, Infantry, CellTags
    };

    static void Create(CFinalSunDlg* pWnd);


    static HWND GetHandle()
    {
        return CNewINIEditor::m_hwnd;
    }
    static bool OnEnterKeyDown(HWND& hWnd);
    static int FindLBTextCaseSensitive(HWND hwndCtl, const char* searchString);
    static bool IsGameObject(const char* lpSectionName);
    static bool IsMapPack(const char* lpSectionName);
    static bool IsHouse(const char* lpSectionName);
    static bool IsTeam(const char* lpSectionName);

protected:
    static void Initialize(HWND& hWnd);
    static void InitializeImporter(HWND& hWnd);
    static void Update(HWND& hWnd);

    static void OnSelchangeListbox(int index = -1);
    static void OnEditchangeINIEdit();
    static void OnEditchangeSearch();
    static void OnClickNewSection();
    static void OnClickImportText(HWND& hWnd);
    static void OnClickDelSection(HWND& hWnd);
    static void OnClickImporterOK(HWND& hWnd);
    static void UpdateGameObject(const char* lpSectionName);
    static void UpdateAllGameObject();

    static void Close(HWND& hWnd);
    static void CloseImporter(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK DlgProcImporter(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static HWND m_hwndImporter;
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
public:
    static HWND hSearchText;
    static HWND hSectionList;
    static HWND hNewSectionName;
    static HWND hINIEdit;
    static HWND hNewButton;
    static HWND hDeleteButton;
    static HWND hImportButton;
    static HWND hImportTextButton;
    static HWND hImporterDesc;
    static HWND hImporterOK;
    static HWND hImporterText;
private:
    static WNDPROC OriginalListBoxProc;
    static std::map<int, FString> SectionLabels;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet;

};

