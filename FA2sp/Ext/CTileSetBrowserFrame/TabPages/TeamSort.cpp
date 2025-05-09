#include "TeamSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"

#include <CFinalSunDlg.h>
#include "TaskForceSort.h"
#include "ScriptSort.h"
#include "../../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../../Helpers/Translations.h"
TeamSort TeamSort::Instance;
std::vector<ppmfc::CString> TeamSort::TreeViewTexts;
std::vector<std::vector<ppmfc::CString>> TeamSort::TreeViewTextsVector;
bool TeamSort::CreateFromTeamSort = false;

void TeamSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();
    TreeViewTexts.clear();
    TreeViewTextsVector.clear();
    // TODO : 
    // Optimisze the efficiency
    if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            this->AddTrigger(pair.second);
        }
    }
    ExtConfigs::InitializeMap = true;
}

void TeamSort::Clear()
{
    TreeView_DeleteAllItems(this->GetHwnd());
}

BOOL TeamSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto pID = reinterpret_cast<const char*>(lpNmTreeView->itemNew.lParam))
        {
            
            if (strlen(pID) && ExtConfigs::InitializeMap)
            {
                bool Success = false;
                if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    auto pStr = CINI::CurrentDocument->GetString(pID, "Name");
                    ppmfc::CString space1 = " (";
                    ppmfc::CString space2 = ")";

                    int idx = SendMessage(CNewTeamTypes::hSelectedTeam, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2).m_pchData);
                    if (idx != CB_ERR)
                    {
                        SendMessage(CNewTeamTypes::hSelectedTeam, CB_SETCURSEL, idx, NULL);
                        CNewTeamTypes::OnSelchangeTeamtypes();
                        Success = true;
                    }

                }
                //else if (IsWindowVisible(CNewTrigger::GetHandle()))
                //{
                //    auto pStr = CINI::CurrentDocument->GetString(pID, "Name");
                //    ppmfc::CString space = " - ";
                //    for (int i = 0; i < EVENT_PARAM_COUNT; i++)
                //    {
                //        int idx = SendMessage(CNewTrigger::hEventParameter[i], CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space + pStr).m_pchData);
                //        if (idx != CB_ERR)
                //        {
                //            SendMessage(CNewTrigger::CNewTrigger::hEventParameter[i], CB_SETCURSEL, idx, NULL);
                //            CNewTrigger::OnSelchangeEventParam(i);
                //            Success = true;
                //        }
                //    }
                //    for (int i = 0; i < ACTION_PARAM_COUNT; i++)
                //    {
                //        int idx = SendMessage(CNewTrigger::hActionParameter[i], CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space + pStr).m_pchData);
                //        if (idx != CB_ERR)
                //        {
                //            SendMessage(CNewTrigger::CNewTrigger::hActionParameter[i], CB_SETCURSEL, idx, NULL);
                //            CNewTrigger::OnSelchangeActionParam(i);
                //            Success = true;
                //        }
                //    }
                //    
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

BOOL TeamSort::OnMessage(PMSG pMsg)
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

void TeamSort::Create(HWND hParent)
{
    RECT rect;
    ::GetClientRect(hParent, &rect);

    this->m_hWnd = CreateWindowEx(NULL, "SysTreeView32", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | 
        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
}

void TeamSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    ::MoveWindow(this->GetHwnd(), 2, 29, rect.right - rect.left - 6, rect.bottom - rect.top - 35, FALSE);
}

void TeamSort::ShowWindow(bool bShow) const
{
    ::ShowWindow(this->GetHwnd(), bShow ? SW_SHOW : SW_HIDE);
}

void TeamSort::ShowWindow() const
{
    this->ShowWindow(true);
}

void TeamSort::HideWindow() const
{
    this->ShowWindow(false);
}

void TeamSort::ShowMenu(POINT pt) const
{
    HMENU hPopupMenu = ::CreatePopupMenu();
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::AddTrigger,
        Translations::TranslateOrDefault("TeamSortNewTeam", "New Team from this group"));
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::Refresh, Translations::TranslateOrDefault("Refresh", "Refresh"));
    ::TrackPopupMenu(hPopupMenu, TPM_VERTICAL | TPM_HORIZONTAL, pt.x, pt.y, NULL, this->GetHwnd(), nullptr);
}

bool TeamSort::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool TeamSort::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

void TeamSort::Menu_AddTrigger()
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

const ppmfc::CString& TeamSort::GetCurrentPrefix() const
{
    return this->m_strPrefix;
}

HWND TeamSort::GetHwnd() const
{
    return this->m_hWnd;
}

TeamSort::operator HWND() const
{
    return this->GetHwnd();
}

HTREEITEM TeamSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
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

std::vector<ppmfc::CString> TeamSort::GetGroup(ppmfc::CString triggerId, ppmfc::CString& name) const
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

void TeamSort::AddTrigger(std::vector<ppmfc::CString> group, ppmfc::CString name, ppmfc::CString id) const
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

void TeamSort::AddTrigger(ppmfc::CString triggerId) const
{
    if (this->IsVisible())
    {
        ppmfc::CString name;
        auto group = this->GetGroup(triggerId, name);

        this->AddTrigger(group, name, triggerId);
    }
}

void TeamSort::DeleteTrigger(ppmfc::CString triggerId, HTREEITEM hItemParent) const
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

