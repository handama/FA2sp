#pragma once
#include "FA2PP.h"
#include "../Helpers/MultimapHelper.h"

class FString;
class CNewTrigger;
class VirtualComboBoxEx;

enum class DropType : int
{
    AttachedTrigger = 0,
    ActionParam0,
    ActionParam1,
    ActionParam2,
    ActionParam3,
    ActionParam4,
    ActionParam5,
    EventParam0,
    EventParam1,
    TeamEditorTag,
    TeamEditorTaskForce,
    TeamEditorScript,
    AIEditorTeam0,
    AIEditorTeam1,
    BatchTriggerListView,
    ActionListBox,
    EventListBox,

    Unknown,
};

struct DropTarget
{
    HWND hWnd;
    RECT screenRect;
    DropType type;
    CNewTrigger* triggerInstance;
    HWND hRoot;
};

struct ListViewHitResult
{
    int item = -1;
    int subItem = -1;
    UINT flags = 0;
};

struct SortPart
{
    std::string text;
    std::string number;
    bool isNumber;
};

struct SortLabelKey
{
    FString original;
    std::vector<SortPart> parts;
};

struct SortKeyIndex
{
    SortLabelKey key;
    size_t index;
};

struct VCBItemEntry
{
    FString text;
    SortLabelKey key;
};

class ExtraWindow
{
public:
    static FString GetTeamDisplayName(const char* id);
    static FString GetAITriggerDisplayName(const char* id);
    static FString FormatTriggerDisplayName(const char* id, const char* name);
    static FString GetEventDisplayName(const char* id, int index = -1, bool addIndex = true);
    static FString GetActionDisplayName(const char* id, int index = -1, bool addIndex = true);
    static void SetEditControlFontSize(HWND hWnd, float nFontSizeMultiplier, bool richEdit = false, const char* newFont = "");
    static int FindCBStringExactStart(HWND hComboBox, const char* searchText);
    static void SyncComboBoxContent(HWND hSource, HWND hTarget, bool addNone = false);
    static void AdjustDropdownWidth(HWND hWnd);
    static FString GetTriggerDisplayName(const char* id);
    static FString GetTriggerName(const char* id);
    static FString GetAITriggerName(const char* id);
    static FString GetTagName(const char* id);
    static FString GetTagDisplayName(const char* id);
    static FString GetTranslatedSectionName(const char* section);

    static void LoadParams(VirtualComboBoxEx& vcb, FString idx, CNewTrigger* instance = nullptr);
    static void LoadParam_Waypoints(VirtualComboBoxEx& vcb);
    static void LoadParam_ActionList(VirtualComboBoxEx& vcb);
    static void LoadParam_CountryList(VirtualComboBoxEx& vcb);
    static void LoadParam_HouseAddon_Multi(VirtualComboBoxEx& vcb);
    static void LoadParam_HouseAddon_MultiAres(VirtualComboBoxEx& vcb);
    static void LoadParam_TechnoTypes(VirtualComboBoxEx& vcb, int specificType = -1, int style = 0, bool sort = true);
    static void LoadParam_Triggers(VirtualComboBoxEx& vcbd, CNewTrigger* instance);
    static void LoadParam_Tags(VirtualComboBoxEx& vcb);
    static void LoadParam_Teamtypes(VirtualComboBoxEx& vcb);
    static void LoadParam_Stringtables(VirtualComboBoxEx& vcb);

    static bool bComboLBoxSelected;
    static bool bEnterSearch; 
    // true means click inside combobox
    static bool OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly = false);
    static void OnEditCComboBox(HWND& hWnd, std::map<int, FString>& labels);

    static void SortLabels(std::vector<FString>& labels);
    static void SortLabels(std::vector<std::pair<FString, FString>>& labels, bool first = true);
    static void SortRawStrings(std::vector<FString>& labels);
    static void SortRawStrings(std::vector<std::pair<FString, FString>>& labels, bool first = true);
    static void SortRawStrings(std::vector<std::pair<std::string, std::string>>& labels, bool first = true);
    static void SortTeams(HWND& hWnd, FString section, int& selectedIndex, FString id = "");
    static void SortTeams(VirtualComboBoxEx& vcb, FString section, int& selectedIndex, FString id = "", bool clear = true);
    static bool IsLabelMatch(const char* target, const char* source, bool exactMatch = false);
    static FString GetCloneName(FString oriName);
    static void LoadFrom(MultimapHelper& mmh, FString loadfrom);
    static void TrimStringIndex(FString& str);
    static void ClearComboKeepText(HWND hWnd);

    static void RegisterDropTarget(HWND hWnd, DropType type, CNewTrigger* trigger = nullptr);
    static void UpdateDropTargetRect(HWND hWnd);
    static DropTarget FindDropTarget(POINT screenPt);
    static void UpdateHoverTarget(POINT pt);
    static void UnregisterDropTarget(HWND hWnd);
    static void UnregisterDropTargetsOfWindow(HWND hMainWnd);
    static bool IsPointOnIsoViewAndNotCovered(POINT ptScreen);
    static FString GetScintillaText(HWND hScintilla);
    static void SetScintillaText(HWND hScintilla, FString& text);
    static bool HitTestListView(HWND hListView, POINT ptScreen, ListViewHitResult& out);
    static void UpdateListBoxHScroll(HWND hListBox);

    static std::vector<DropTarget> g_DropTargets;

private:
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
};

class HelpDlg
{
public:
    void CreateHelpDlg(HWND& hParent, const FString& Title, const FString& Text);
    void CloseHelpDlg();
protected:
    static BOOL CALLBACK HelpDlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    BOOL CALLBACK HandleMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    HWND hDlg;
    HWND hText;
    int origWndWidth;
    int origWndHeight;
    int minWndWidth;
    int minWndHeight;
    bool minSizeSet;
};

class TargetHighlighter {
public:
    TargetHighlighter();
    ~TargetHighlighter();

    void Attach(DropTarget target);
    void Detach();
    void UpdatePosition();

    bool IsActive() const { return highlight_hwnd_ != nullptr && target_hwnd_ != nullptr; }
    bool IsSameTarget(DropTarget target);

    void SetBorderColor(COLORREF color) { border_color_ = color; }
    void SetBorderThickness(int thickness) { border_thickness_ = thickness; }
    void SetBorderRadius(int radius) { border_radius_ = radius; }

private:
    HWND target_hwnd_ = nullptr;
    HWND highlight_hwnd_ = nullptr;
    int col_ = -1;
    int row_ = -1;

    COLORREF border_color_ = RGB(0, 180, 0);
    int      border_thickness_ = 3;
    int      border_radius_ = 0; 

    static ATOM window_class_atom_;
    static bool class_registered_;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClassIfNeeded();
    HWND CreateHighlightWindow(const RECT& rect);
    void DestroyHighlightWindow();
    void UpdateRegion(HWND hwnd, int width, int height);
};

class LabelMatcher
{
public:
    struct Pattern
    {
        std::vector<FString> atoms;
    };

    LabelMatcher(const char* source, bool exactMatch = false)
        : m_exactMatch(exactMatch)
    {
        Build(source);
    }

    bool Match(const char* target) const;

private:
    std::vector<Pattern> m_patterns;
    bool m_exactMatch;

    void Build(const char* source);
    bool MatchPattern(const FString& target, const Pattern& pattern) const;
};

class ComboBoxBatchUpdater
{
public:
    ComboBoxBatchUpdater(HWND hTarget,
        int reserveCount = 0,
        bool preserveSelection = false,
        int avgTextLen = 512,
        bool reset = true)
        : hWnd(hTarget), preserve(preserveSelection)
    {
        if (!hWnd)
            return;

        if (preserve)
        {
            char buffer[512]{ 0 };
            GetWindowText(hWnd, buffer, 511);
            oldText = buffer;
        }

        SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);

        if (reset)
        {
            SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
        }

        if (reserveCount > 0)
        {
            SendMessage(hWnd, CB_INITSTORAGE, reserveCount, avgTextLen);
        }
    }

    ~ComboBoxBatchUpdater()
    {
        if (!hWnd)
            return;

        if (preserve && !oldText.empty())
        {
            int index = (int)SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, oldText);
            if (index != CB_ERR)
            {
                SendMessage(hWnd, CB_SETCURSEL, index, 0);
            }
            else
            {
                FString::TrimIndex(oldText);
                SetWindowText(hWnd, oldText);
            }
        }

        SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hWnd, NULL, TRUE);
    }

    ComboBoxBatchUpdater(const ComboBoxBatchUpdater&) = delete;
    ComboBoxBatchUpdater& operator=(const ComboBoxBatchUpdater&) = delete;

    ComboBoxBatchUpdater(ComboBoxBatchUpdater&& other) noexcept
    {
        hWnd = other.hWnd;
        preserve = other.preserve;
        oldText = std::move(other.oldText);
        other.hWnd = nullptr;
    }

private:
    HWND hWnd = nullptr;
    bool preserve = false;
    FString oldText;
};

class VirtualComboBoxEx
{
public:
    VirtualComboBoxEx();
    ~VirtualComboBoxEx();

    enum DropWidthMode
    {
        DropWidth_FollowCombo,
        DropWidth_AutoMax
    };

    void Attach(HWND hCombo, bool* sortType = nullptr, bool allowFreeText = true);
    void SetAutoSearchRestriction(bool* restrict);
    void Detach();
    void CopyFrom(const VirtualComboBoxEx& other, 
        const std::vector<FString>* addToFront = nullptr,
        const std::vector<FString>* addToEnd = nullptr);

    void AddString(const char* str);
    void AddStrings(const std::vector<FString>& ret, const char* oriText = nullptr);
    int InsertString(int index, const char* str);
    int ReplaceString(int index, const char* str);
    int DeleteString(int index);
    void Clear();

    int GetCurSel() const;
    void SetCurSel(int idx);
    int GetCount() const;
    int GetFilteredCount() const;

    void SetEditText(const char* text) const;
    const char* GetEditText() const;
    const char* GetItemText(int index) const;
    const char* GetFilteredText(int index) const;
    const char* GetSelectedText(bool allowEdit) const;

    int FindStringExact(const char* str) const;
    int FindString(const char* str) const;

    static void SetWindowHeight(HWND hwnd, LPARAM lParam);
    void SetDropWidthMode(DropWidthMode mode);

    void SortItems(int* pSelIndex = nullptr);

private:
    HWND hCombo = nullptr;
    HWND hEdit = nullptr;
    HWND hList = nullptr;

    WNDPROC oldComboProc = nullptr;
    WNDPROC oldEditProc = nullptr;
    WNDPROC oldListProc = nullptr;

    std::vector<VCBItemEntry> items;
    std::vector<int> filtered;

    int curSel = -1;
    int pendingSelect = -1;
    bool m_filterActive = false;
    bool m_programmaticDropdown = false;
    bool m_programmaticPostDropdown = false;
    bool m_sortByLabelKey = false;
    bool* m_sortType = nullptr;
    bool m_nextDropSort = false;
    bool m_allowFreeText = false;
    bool* m_allowFilter = nullptr;
    bool m_needFixSelection = false;
    bool m_inFixSelection = false;

    DropWidthMode m_dropWidthMode = DropWidth_AutoMax;
    int m_cachedMaxWidth = 0;

private:
    void Filter(const char* text);
    void EnsureFilteredAll();
    void SyncListCount();
    int CalcMaxItemWidth();
    int CalcItemWidth(int index);
    void UpdateDropWidth();

    static LRESULT CALLBACK ComboProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK ListProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT OnComboMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};