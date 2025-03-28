#include "ScriptSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"
#include "../../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../../ExtraWindow/CNewScript/CNewScript.h"
#include <CFinalSunDlg.h>
#include "../../../Helpers/Translations.h"

ScriptSort ScriptSort::Instance;
std::vector<ppmfc::CString> ScriptSort::TreeViewTexts;
std::vector<std::vector<ppmfc::CString>> ScriptSort::TreeViewTextsVector;
bool ScriptSort::CreateFromScriptSort = false;

void ScriptSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();
    TreeViewTexts.clear();
    TreeViewTextsVector.clear();
    // TODO : 
    // Optimisze the efficiency
    if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            this->AddTrigger(pair.second);
        }
    }
    ExtConfigs::InitializeMap = true;
}

void ScriptSort::Clear()
{
    TreeView_DeleteAllItems(this->GetHwnd());
}

BOOL ScriptSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto pID = reinterpret_cast<const char*>(lpNmTreeView->itemNew.lParam))
        {

            if (strlen(pID) && ExtConfigs::InitializeMap )//&& valid)
            {
                bool Success = false;
                if (IsWindowVisible(CNewScript::GetHandle()))
                {
                    auto pStr = CINI::CurrentDocument->GetString(pID, "Name");
                    ppmfc::CString space1 = " (";
                    ppmfc::CString space2 = ")";

                    int idx = SendMessage(CNewScript::hSelectedScript, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2).m_pchData);
                    if (idx != CB_ERR)
                    {
                        SendMessage(CNewScript::hSelectedScript, CB_SETCURSEL, idx, NULL);
                        CNewScript::OnSelchangeScript();
                        Success = true;
                    }
                }
                //else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                //{
                //    auto pStr = CINI::CurrentDocument->GetString(pID, "Name");
                //    ppmfc::CString space1 = " (";
                //    ppmfc::CString space2 = ")";
                //
                //    int idx = SendMessage(CNewTeamTypes::hScript, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2).m_pchData);
                //    if (idx != CB_ERR)
                //    {
                //        SendMessage(CNewTeamTypes::hScript, CB_SETCURSEL, idx, NULL);
                //        CNewTeamTypes::OnSelchangeScript();
                //        Success = true;
                //    }
                //    Success = true;
                //}
                return Success;
            }
        }
        break;
    default:
        break;
    }

    return FALSE;
}

BOOL ScriptSort::OnMessage(PMSG pMsg)
{
    switch (pMsg->message)
    {
    case WM_RBUTTONDOWN:
        this->ShowMenu(pMsg->pt);
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

void ScriptSort::Create(HWND hParent)
{
    RECT rect;
    ::GetClientRect(hParent, &rect);

    this->m_hWnd = CreateWindowEx(NULL, "SysTreeView32", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | 
        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
}

void ScriptSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    ::MoveWindow(this->GetHwnd(), 2, 29, rect.right - rect.left - 6, rect.bottom - rect.top - 35, FALSE);
}

void ScriptSort::ShowWindow(bool bShow) const
{
    ::ShowWindow(this->GetHwnd(), bShow ? SW_SHOW : SW_HIDE);
}

void ScriptSort::ShowWindow() const
{
    this->ShowWindow(true);
}

void ScriptSort::HideWindow() const
{
    this->ShowWindow(false);
}

void ScriptSort::ShowMenu(POINT pt) const
{
    HMENU hPopupMenu = ::CreatePopupMenu();
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::AddTrigger,
        Translations::TranslateOrDefault("ScriptSortNewScript", "New Script from this group"));
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::Refresh, Translations::TranslateOrDefault("Refresh","Refresh"));
    ::TrackPopupMenu(hPopupMenu, TPM_VERTICAL | TPM_HORIZONTAL, pt.x, pt.y, NULL, this->GetHwnd(), nullptr);
}

bool ScriptSort::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool ScriptSort::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

void ScriptSort::Menu_AddTrigger()
{
    HTREEITEM hItem = TreeView_GetSelection(this->GetHwnd());
    std::string prefix = "";
    if (hItem != NULL)
    {
        const char* pID = nullptr;
        while (true)
        {
            TVITEM tvi;
            tvi.hItem = hItem;
            TreeView_GetItem(this->GetHwnd(), &tvi);
            if (pID = reinterpret_cast<const char*>(tvi.lParam))
                break;
            hItem = TreeView_GetChild(this->GetHwnd(), hItem);
            if (hItem == NULL)
            {
                this->m_strPrefix = prefix.c_str();
                return;
            }
        }

        ppmfc::CString buffer;
        prefix += "[";
        for (auto& group : this->GetGroup(pID, buffer))
            prefix += group + ".";
        if (prefix[prefix.length() - 1] == '.')
        {
            prefix[prefix.length() - 1] = ']';
            if (prefix.length() == 2)
                prefix = "";
        }
        else
            prefix = "";
    }
    this->m_strPrefix = prefix.c_str();
}

const ppmfc::CString& ScriptSort::GetCurrentPrefix() const
{
    return this->m_strPrefix;
}

HWND ScriptSort::GetHwnd() const
{
    return this->m_hWnd;
}

ScriptSort::operator HWND() const
{
    return this->GetHwnd();
}

HTREEITEM ScriptSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    TVITEM tvi;
    char chLabel[0x200];

    for (tvi.hItem = TreeView_GetChild(this->GetHwnd(), hItemParent); tvi.hItem;
        tvi.hItem = TreeView_GetNextSibling(this->GetHwnd(), tvi.hItem))
    {
        tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
        tvi.pszText = chLabel;
        tvi.cchTextMax = _countof(chLabel);
        if (TreeView_GetItem(this->GetHwnd(), &tvi))
        {
            if (strcmp(tvi.pszText, pszLabel) == 0)
                return tvi.hItem;
            if (tvi.cChildren)
            {
                HTREEITEM hChildSearch = this->FindLabel(tvi.hItem, pszLabel);
                if (hChildSearch) 
                    return hChildSearch;
            }
        }
    }
    return NULL;
}

std::vector<ppmfc::CString> ScriptSort::GetGroup(ppmfc::CString triggerId, ppmfc::CString& name) const
{
    //dont change this
    auto name2 = std::string(CINI::CurrentDocument->GetString(triggerId, "Name", ""));
    ppmfc::CString pSrc = name2.c_str();

    auto ret = std::vector<ppmfc::CString>{};
    //pSrc = ret[2];
    int nStart = pSrc.Find('[');
    int nEnd = pSrc.Find(']');
    if (nStart < nEnd && nStart == 0)
    {
        name = pSrc.Mid(nEnd + 1);
        pSrc = pSrc.Mid(nStart + 1, nEnd - nStart - 1);
        ret = STDHelpers::SplitString(pSrc, ".");
        return ret;
    }
    else
        name = pSrc;
    
    ret.clear();
    return ret;
}

void ScriptSort::AddTrigger(std::vector<ppmfc::CString> group, ppmfc::CString name, ppmfc::CString id) const
{
    TreeViewTextsVector.push_back(group);
    HTREEITEM hParent = TVI_ROOT;
    for (auto& node : TreeViewTextsVector.back())
    {
        if (HTREEITEM hNode = this->FindLabel(hParent, node))
        {
            hParent = hNode;
            continue;
        }
        else
        {
            TVINSERTSTRUCT tvis;
            tvis.hInsertAfter = TVI_SORT;
            tvis.hParent = hParent;
            tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvis.item.lParam = NULL;
            tvis.item.pszText = node.m_pchData;
            hParent = TreeView_InsertItem(this->GetHwnd(), &tvis);
        }
    }

    if (HTREEITEM hNode = this->FindLabel(hParent, name))
    {
        TVITEM item;
        item.hItem = hNode;
        if (TreeView_GetItem(this->GetHwnd(), &item))
        {
            ppmfc::CString text = item.pszText;
            text += " (" + id + ")";
            TreeViewTexts.push_back(text);
            item.pszText = TreeViewTexts.back().m_pchData;
            TreeViewTexts.push_back(id);
            item.lParam = (LPARAM)TreeViewTexts.back().m_pchData;
            TreeView_SetItem(this->GetHwnd(), &item);
        }
    }
    else
    {
        TVINSERTSTRUCT tvis;
        tvis.hInsertAfter = TVI_SORT;
        tvis.hParent = hParent;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
        ppmfc::CString text = name;
        text += " (" + id + ")";
        TreeViewTexts.push_back(text);
        tvis.item.pszText = TreeViewTexts.back().m_pchData;
        TreeViewTexts.push_back(id);
        tvis.item.lParam = (LPARAM)TreeViewTexts.back().m_pchData;
        TreeView_InsertItem(this->GetHwnd(), &tvis);
    }
}

void ScriptSort::AddTrigger(ppmfc::CString triggerId) const
{
    if (this->IsVisible())
    {
        ppmfc::CString name;
        auto group = this->GetGroup(triggerId, name);

        this->AddTrigger(group, name, triggerId);
    }
}

void ScriptSort::DeleteTrigger(ppmfc::CString triggerId, HTREEITEM hItemParent) const
{
    if (this->IsVisible())
    {
        TVITEM tvi;

        for (tvi.hItem = TreeView_GetChild(this->GetHwnd(), hItemParent); tvi.hItem;
            tvi.hItem = TreeView_GetNextSibling(this->GetHwnd(), tvi.hItem))
        {
            tvi.mask = TVIF_PARAM | TVIF_CHILDREN;
            if (TreeView_GetItem(this->GetHwnd(), &tvi))
            {
                if (tvi.lParam && strcmp((const char*)tvi.lParam, triggerId) == 0)
                {
                    TreeView_DeleteItem(this->GetHwnd(), tvi.hItem);
                    return;
                }
                if (tvi.cChildren)
                    this->DeleteTrigger(triggerId, tvi.hItem);
            }
        }
    }
}

