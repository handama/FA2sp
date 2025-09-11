#include "TriggerSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"

#include <CFinalSunDlg.h>
#include "TeamSort.h"
#include "WaypointSort.h"
#include "../../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../../Helpers/Translations.h"
#include "../../CMapData/Body.h"
#include "../../../Miscs/DialogStyle.h"
using namespace std;

TriggerSort TriggerSort::Instance;
std::unordered_map<FString, FString> TriggerSort::TriggerTags;
std::unordered_map<FString, std::vector<FString>> TriggerSort::TriggerTagsParent;
std::unordered_set<FString> TriggerSort::attachedTriggers;
bool TriggerSort::CreateFromTriggerSort = false;

void TriggerSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();

    TriggerTags.clear();
    TriggerTagsParent.clear();
    for (auto& triggerPair : CMapDataExt::Triggers)
    {
        auto& trigger = triggerPair.second;

        if (trigger->AttachedTrigger != "<none>")
        {
            if (auto atri = CMapDataExt::GetTrigger(trigger->AttachedTrigger))
            {
                TriggerTags[trigger->ID] = trigger->AttachedTrigger;
            }
            TriggerTagsParent[trigger->AttachedTrigger].push_back(trigger->ID);
        }
    }
    for (auto& triggerPair : CMapDataExt::Triggers)
    {
        auto& trigger = triggerPair.second;
        this->AddTrigger(trigger->ID);
    }
    ExtConfigs::InitializeMap = true;
}

void TriggerSort::Clear()
{
    TreeViewHelper::ClearTreeView(this->GetHwnd());
}

BOOL TriggerSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto data = TreeViewHelper::GetTreeItemData(this->GetHwnd(), lpNmTreeView->itemNew.hItem))
        {
            auto& pID = data->param;
            if (strlen(pID) && ExtConfigs::InitializeMap)
            {
                if (IsWindowVisible(CNewTrigger::GetHandle()))
                {
                    FString pStr = CINI::CurrentDocument->GetString("Triggers", pID);
                    auto results = FString::SplitString(pStr);
                    if (results.size() <= 3)
                        return FALSE;
                    pStr = results[2];
                    //if (ExtConfigs::DisplayTriggerID)
                    {
                        FString tmp = pStr;
                        pStr.Format("%s (%s)", pID, tmp.c_str());
                    }
                    auto idx = SendMessage(CNewTrigger::hSelectedTrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)pStr);
                    if (idx == CB_ERR)
                        return FALSE;

                    SendMessage(CNewTrigger::hSelectedTrigger, CB_SETCURSEL, idx, NULL);
                    CNewTrigger::OnSelchangeTrigger();
                    return TRUE;
                }
                else
                    return FALSE;
            }
        }
        break;
    default:
        break;
    }

    return FALSE;
}

BOOL TriggerSort::OnMessage(PMSG pMsg)
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

void TriggerSort::Create(HWND hParent)
{
    RECT rect;
    ::GetClientRect(hParent, &rect);

    this->m_hWnd = CreateWindowEx(NULL, "SysTreeView32", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | 
        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);

    if (ExtConfigs::EnableDarkMode && this->m_hWnd)
    {
        ::SendMessage(this->m_hWnd, TVM_SETBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(this->m_hWnd, TVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));
    }
}

void TriggerSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    ::MoveWindow(this->GetHwnd(), 2, 29, rect.right - rect.left - 6, rect.bottom - rect.top - 35, FALSE);
}

void TriggerSort::ShowWindow(bool bShow) const
{
    ::ShowWindow(this->GetHwnd(), bShow ? SW_SHOW : SW_HIDE);
}

void TriggerSort::ShowWindow() const
{
    this->ShowWindow(true);
}

void TriggerSort::HideWindow() const
{
    this->ShowWindow(false);
}

void TriggerSort::ShowMenu(POINT pt) const
{
    HMENU hPopupMenu = ::CreatePopupMenu();
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::AddTrigger, 
        Translations::TranslateOrDefault("TriggerSortNewTrigger", "New Trigger from this group"));
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::Refresh, Translations::TranslateOrDefault("Refresh", "Refresh"));
    ::TrackPopupMenu(hPopupMenu, TPM_VERTICAL | TPM_HORIZONTAL, pt.x, pt.y, NULL, this->GetHwnd(), nullptr);
}

bool TriggerSort::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool TriggerSort::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

void TriggerSort::Menu_AddTrigger()
{
    HTREEITEM hItem = TreeView_GetSelection(this->GetHwnd());
    FString prefix = "";
    if (hItem != NULL)
    {
        const char* pID = nullptr;
        while (true)
        {
            TVITEM tvi;
            tvi.hItem = hItem;
            TreeView_GetItem(this->GetHwnd(), &tvi);
            if (auto data = TreeViewHelper::GetTreeItemData(this->GetHwnd(), tvi.hItem))
            {
                pID = data->param.c_str();
                break;
            }
            hItem = TreeView_GetChild(this->GetHwnd(), hItem);
            if (hItem == NULL)
            {
                this->m_strPrefix = prefix;
                return;
            }
        }

        FString buffer;
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
    this->m_strPrefix = prefix;
}

const FString& TriggerSort::GetCurrentPrefix() const
{
    return this->m_strPrefix;
}

HWND TriggerSort::GetHwnd() const
{
    return this->m_hWnd;
}

TriggerSort::operator HWND() const
{
    return this->GetHwnd();
}

HTREEITEM TriggerSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    TVITEM tvi;
    char chLabel[0x200] = { 0 };

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
void TriggerSort::AddAttachedTrigger(HTREEITEM hParent, FString triggerID, FString parentName) const
{
    if (attachedTriggers.find(TriggerTags[triggerID]) != attachedTriggers.end())
    {
        if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
        {
            FString pTrigger2 = Translations::TranslateOrDefault("Sort.DetectedLoopedTrigger", "Detected Looped Trigger!");
            hParent = hNode;
            TreeViewHelper::InsertTreeItem(this->GetHwnd(), pTrigger2, pTrigger2, hParent);
            return;
        }
    }

    
    if (TriggerTags[triggerID] != "")
        if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
        {        
            FString pTrigger2 = CINI::CurrentDocument->GetString("Triggers", TriggerTags[triggerID], "");
            auto RET2 = FString::SplitString(pTrigger2);
            if (RET2.size() > 2)
            {
                pTrigger2 = RET2[2];
                FString pszText = FString(Translations::TranslateOrDefault("Sort.AttachedTrigger", "Attached Trigger:")) + " " + pTrigger2 + " (" + TriggerTags[triggerID] + ")";
                hParent = hNode;
                TreeViewHelper::InsertTreeItem(this->GetHwnd(), pszText, TriggerTags[triggerID], hParent);             
                attachedTriggers.insert(TriggerTags[triggerID]);
                AddAttachedTrigger(hParent, TriggerTags[triggerID], pszText);
            }
        }
}

void TriggerSort::AddAttachedTriggerReverse(HTREEITEM hParent, FString triggerID, FString parentName) const
{
    auto hParent2 = hParent;
    if (TriggerTagsParent[triggerID].size() > 0)
        for (auto& parentTrigger : TriggerTagsParent[triggerID])
        {
            if (HTREEITEM hNode = this->FindLabel(hParent2, parentName))
            {
                if (attachedTriggers.find(parentTrigger) != attachedTriggers.end())
                {
                    if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
                    {
                        FString pTrigger2 = Translations::TranslateOrDefault("Sort.DetectedLoopedTrigger", "Detected Looped Trigger!");
                        hParent = hNode;
                        TreeViewHelper::InsertTreeItem(this->GetHwnd(), pTrigger2, pTrigger2, hParent);
                        return;
                    }
                }

                FString pTrigger2 = CINI::CurrentDocument->GetString("Triggers", parentTrigger, "");
                auto RET2 = FString::SplitString(pTrigger2);
                if (RET2.size() > 2)
                {
                    pTrigger2 = RET2[2];
                    FString pszText = FString(Translations::TranslateOrDefault("Sort.TriggerAttachedTo", "Trigger Attached To:")) + " " + pTrigger2 + " (" + parentTrigger + ")";
                    hParent = hNode;
                    TreeViewHelper::InsertTreeItem(this->GetHwnd(), pszText, parentTrigger, hParent);
                    attachedTriggers.insert(parentTrigger);
                    AddAttachedTriggerReverse(hParent, parentTrigger, pszText);
                }
            }
        }

}

std::vector<FString> TriggerSort::GetGroup(FString triggerId, FString& name) const
{
    FString pSrc = CINI::CurrentDocument->GetString("Triggers", triggerId, "");

    auto ret = FString::SplitString(pSrc, 2);
    pSrc = ret[2];
    int nStart = pSrc.Find('[');
    int nEnd = pSrc.Find(']');
    if (nStart < nEnd && nStart == 0)
    {
        name = pSrc.Mid(nEnd + 1);
        pSrc = pSrc.Mid(nStart + 1, nEnd - nStart - 1);
        ret = FString::SplitString(pSrc, ".");
        return ret;
    }
    else
        name = pSrc;
    
    ret.clear();
    return ret;
}

void TriggerSort::AddTrigger(std::vector<FString> group, FString name, FString id) const
{
    HTREEITEM hParent = TVI_ROOT;
    for (auto& node : group)
    {
        if (HTREEITEM hNode = this->FindLabel(hParent, node))
        {
            hParent = hNode;
            continue;
        }
        else
        {
            hParent = TreeViewHelper::InsertTreeItem(this->GetHwnd(), node, "", hParent);
        }
    }

    if (HTREEITEM hNode = this->FindLabel(hParent, name))
    {
        TVITEM item;
        item.hItem = hNode;
        if (TreeView_GetItem(this->GetHwnd(), &item))
        {
            FString text = item.pszText;
            text += " (" + id + ")";
            TreeViewHelper::UpdateTreeItem(this->GetHwnd(), hNode, text, id);
        }
    }
    else
    {
        FString text = name;
        text += " (" + id + ")";
        TreeViewHelper::InsertTreeItem(this->GetHwnd(), text, id, hParent);
        if (HTREEITEM hNode = this->FindLabel(hParent, text))
        {
            auto hParent2 = hParent;
            attachedTriggers.clear();
            attachedTriggers.insert(id);
            AddAttachedTrigger(hParent2, id, text);

            hParent2 = hParent;
            attachedTriggers.clear();
            attachedTriggers.insert(id);
            AddAttachedTriggerReverse(hParent2, id, text);
        }
    }
}

void TriggerSort::AddTrigger(FString triggerId) const
{
    if (this->IsVisible())
    {
        FString name;
        auto group = this->GetGroup(triggerId, name);
        this->AddTrigger(group, name, triggerId);
    }
}

void TriggerSort::DeleteTrigger(FString triggerId, HTREEITEM hItemParent) const
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

// just mannual update
//DEFINE_HOOK(4FA450, CTriggerFrame_Update_TriggerSort, 7)
//{
//    
    //if (TriggerSort::Instance.IsVisible())
    //{
    //    TriggerSort::Instance.LoadAllTriggers();
    //}
    //    
    //if (TeamSort::Instance.IsVisible())
    //{
    //    TeamSort::Instance.LoadAllTriggers();
    //}
    //if (WaypointSort::Instance.IsVisible())
    //{
    //    WaypointSort::Instance.LoadAllTriggers();
    //}

//    return 0;
//}