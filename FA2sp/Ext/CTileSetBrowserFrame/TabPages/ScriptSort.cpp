#include "ScriptSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"
#include "../../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../../ExtraWindow/CNewScript/CNewScript.h"
#include <CFinalSunDlg.h>
#include "../../../Helpers/Translations.h"
#include "../../../Miscs/DialogStyle.h"

ScriptSort ScriptSort::Instance;
bool ScriptSort::CreateFromScriptSort = false;

void ScriptSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();
    // TODO : 
    // Optimisze the efficiency
    SendMessage(this->GetHwnd(), WM_SETREDRAW, FALSE, 0);
    if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            this->AddTrigger(pair.second);
        }
    }
    
    SendMessage(this->GetHwnd(), WM_SETREDRAW, TRUE, 0);
    InvalidateRect(this->GetHwnd(), NULL, TRUE);
    ExtConfigs::InitializeMap = true;
}

void ScriptSort::Clear()
{
    TreeViewHelper::ClearTreeView(this->GetHwnd());
    this->IndexClear();
}

BOOL ScriptSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto data = TreeViewHelper::GetTreeItemData(this->GetHwnd(), lpNmTreeView->itemNew.hItem))
        {
            if (data->isParent) break;
            auto& pID = data->param;
            if (strlen(pID) && ExtConfigs::InitializeMap )//&& valid)
            {
                bool Success = false;
                if (IsWindowVisible(CNewScript::GetHandle()))
                {
                    auto pStr = CINI::CurrentDocument->GetString(pID, "Name");
                    FString space1 = " (";
                    FString space2 = ")";

                    int idx = SendMessage(CNewScript::hSelectedScript, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2));
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
                //    FString space1 = " (";
                //    FString space2 = ")";
                //
                //    int idx = SendMessage(CNewTeamTypes::hScript, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2).GetString());
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

    if (ExtConfigs::EnableDarkMode && this->m_hWnd)
    {
        ::SendMessage(this->m_hWnd, TVM_SETBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(this->m_hWnd, TVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));
    }
}

void ScriptSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    int tabPageheight = 20 * CFinalSunAppExt::ProgramScaleFactor;
    ::MoveWindow(this->GetHwnd(), 2, tabPageheight, rect.right - rect.left - 6, rect.bottom - rect.top - 6 - tabPageheight, FALSE);
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
    FString prefix = "";
    TreeViewHelper::TreeItemData* data = nullptr;
    if (hItem != NULL)
    {
        const char* pID = nullptr;
        while (true)
        {
            TVITEM tvi;
            tvi.hItem = hItem;
            TreeView_GetItem(this->GetHwnd(), &tvi);
            if (data = TreeViewHelper::GetTreeItemData(this->GetHwnd(), tvi.hItem))
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
        if (data->isParent)
        {
            if (strlen(pID) > 0)
            {
                prefix += pID;
                prefix += "]";
            }
            else
            {
                prefix = "";
            }
        }
        else
        {
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
    }
    this->m_strPrefix = prefix;
}

const FString& ScriptSort::GetCurrentPrefix() const
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

std::string ScriptSort::MakeLabelKey(HTREEITEM hParent, LPCSTR pszLabel)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%p:", hParent);
    return std::string(buf) + pszLabel;
}

void ScriptSort::IndexAdd(HTREEITEM hParent, LPCSTR pszLabel, HTREEITEM hItem) const
{
    if (hParent && pszLabel && pszLabel[0])
        m_labelIndex[MakeLabelKey(hParent, pszLabel)] = hItem;
}

void ScriptSort::IndexRemove(HTREEITEM hParent, LPCSTR pszLabel) const
{
    if (hParent && pszLabel && pszLabel[0])
        m_labelIndex.erase(MakeLabelKey(hParent, pszLabel));
}

void ScriptSort::IndexClear() const
{
    m_labelIndex.clear();
}

HTREEITEM ScriptSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    auto it = m_labelIndex.find(MakeLabelKey(hItemParent, pszLabel));
    if (it != m_labelIndex.end())
        return it->second;
    return NULL;
}

std::vector<FString> ScriptSort::GetGroup(FString triggerId, FString& name) const
{
    FString pSrc = CINI::CurrentDocument->GetString(triggerId, "Name", "");

    auto ret = std::vector<FString>{};
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

void ScriptSort::AddTrigger(std::vector<FString> group, FString name, FString id) const
{
    HTREEITEM hParent = TVI_ROOT;
    std::vector<FString> currentNodes;
    for (auto& node : group)
    {
        currentNodes.push_back(node);
        if (HTREEITEM hNode = this->FindLabel(hParent, node))
        {
            hParent = hNode;
            continue;
        }
        else
        {
            FString nodeCombo = FString::Join(currentNodes, ".");
            auto hOldParent = hParent;
            hParent = TreeViewHelper::InsertTreeItem(this->GetHwnd(), node, nodeCombo, hOldParent, true);
            this->IndexAdd(hOldParent, node, hParent);
        }
    }

    if (HTREEITEM hNode = this->FindLabel(hParent, name))
    {
        TVITEM item;
        item.hItem = hNode;
        if (TreeView_GetItem(this->GetHwnd(), &item))
        {
            auto* pOldData = TreeViewHelper::GetTreeItemData(this->GetHwnd(), item.hItem);
            if (pOldData)
                this->IndexRemove(hParent, pOldData->label);
            FString text = item.pszText;
            text += " (" + id + ")";
            TreeViewHelper::UpdateTreeItem(this->GetHwnd(), hNode, text, id);
            this->IndexAdd(hParent, text, item.hItem);
        }
    }
    else
    {
        auto hItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), name + " (" + id + ")", id, hParent);
        this->IndexAdd(hParent, name + " (" + id + ")", hItem);
    }
}

void ScriptSort::AddTrigger(FString triggerId) const
{
    if (this->IsVisible())
    {
        FString name;
        auto group = this->GetGroup(triggerId, name);

        this->AddTrigger(group, name, triggerId);
    }
}
