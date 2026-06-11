#include "TagSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"

#include <CFinalSunDlg.h>
#include "TeamSort.h"
#include "WaypointSort.h"
#include "../../CMapData/Body.h"
#include "../../../Helpers/Translations.h"
#include "../../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../../ExtraWindow/CNewTag/CNewTag.h"
#include "../../../Miscs/StringtableLoader.h"
#include "../../../Miscs/DialogStyle.h"

TagSort TagSort::Instance;
FHashSet TagSort::attachedTriggers;
FHashMap<std::vector<FString>> TagSort::BuildingTags;
FHashMap<std::vector<FString>> TagSort::AircraftTags;
FHashMap<std::vector<FString>> TagSort::UnitTags;
FHashMap<std::vector<FString>> TagSort::InfantryTags;
FHashMap<FString> TagSort::TagTriggers;
FHashMap<FString> TagSort::TriggerTags;
FHashMap<std::vector<FString>> TagSort::TriggerTagsParent;
FHashMap<std::vector<FString>> TagSort::CellTagTags;
FHashMap<std::vector<FString>> TagSort::TeamTags;
bool TagSort::CreateFromTagSort = false;

enum FindType { Aircraft = 0, Infantry, Structure, Unit };
void TagSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();
    BuildingTags.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("Structures"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            auto tag = FString::GetParam(pair.second, 6);
            if (tag != "<none>" && !tag.IsEmpty())
                BuildingTags[tag].push_back(pair.second);
        }
    }
    AircraftTags.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("Aircraft"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            auto tag = FString::GetParam(pair.second, 7);
            if (tag != "<none>" && !tag.IsEmpty())
                AircraftTags[tag].push_back(pair.second);
        }
    }
    UnitTags.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("Units"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            auto tag = FString::GetParam(pair.second, 7);
            if (tag != "<none>" && !tag.IsEmpty())
                UnitTags[tag].push_back(pair.second);
        }
    }
    InfantryTags.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("Infantry"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            auto tag = FString::GetParam(pair.second, 8);
            if (tag != "<none>" && !tag.IsEmpty())
                InfantryTags[tag].push_back(pair.second);
        }
    }
    TeamTags.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("TeamTypes"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            if (CINI::CurrentDocument->SectionExists(pair.second))
            {
                auto tag = CINI::CurrentDocument->GetString(pair.second, "Tag");
                TeamTags[tag].push_back(pair.second);
            }
        }
    }
    CellTagTags.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("CellTags"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            CellTagTags[pair.second].push_back(pair.first);
        }
    }
    TriggerTags.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("Triggers"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second);
            if (atoms.size() > 2)
            {
                if (atoms[1] != "<none>")
                {
                    for (auto& pair2 : pObjSection->GetEntities())
                    {
                        if (atoms[1] == pair2.first)
                        {
                            TriggerTags[pair.first] = pair2.first;
                            break;
                        }   
                    } 
                }    
            }
        }
    }
    TriggerTagsParent.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("Triggers"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second);
            if (atoms.size() > 2)
            {
                if (atoms[1]!= "<none>")
                    TriggerTagsParent[atoms[1]].push_back(pair.first);
            }
        }
    }
    TagTriggers.clear();
    if (auto pObjSection = CINI::CurrentDocument->GetSection("Tags"))
    {
        for (auto& pair : pObjSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second);
            if (atoms.size() > 2)
            {
                if (auto pObjSection = CINI::CurrentDocument->GetSection("Triggers"))
                {
                    for (auto& pair2 : pObjSection->GetEntities())
                    {
                        if (atoms[2] == pair2.first)
                        {
                            TagTriggers[pair.first] = pair2.first;
                            break;
                        }
                    }
                }
            }
        }
    }

    SendMessage(this->GetHwnd(), WM_SETREDRAW, FALSE, 0);
    if (auto pSection = CINI::CurrentDocument->GetSection("Tags"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            this->AddTrigger(pair.first);
        }
    }
    SendMessage(this->GetHwnd(), WM_SETREDRAW, TRUE, 0);
    InvalidateRect(this->GetHwnd(), NULL, TRUE);
    ExtConfigs::InitializeMap = true;
}

void TagSort::Clear()
{
    TreeViewHelper::ClearTreeView(this->GetHwnd());
    this->IndexClear();
}

BOOL TagSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto data = TreeViewHelper::GetTreeItemData(this->GetHwnd(), lpNmTreeView->itemNew.hItem))
        {
            if (data->isParent) break;
            auto& pID = data->param;
            bool finished = false;
            if (strlen(pID) && ExtConfigs::InitializeMap)
            {
				if (IsWindowVisible(CNewTag::GetHandle()))
				{
					auto text = ExtraWindow::GetTagDisplayName(pID);
					auto idx = CNewTag::vcbSelectedTag.FindStringExact(text);
					if (idx != CB_ERR)
					{
                        CNewTag::vcbSelectedTag.SetCurSel(idx);
                        CNewTag::OnSelchangeTag();
                        finished = true;
                    }
				}
				if (IsWindowVisible(CNewTrigger::GetFirstValidInstance().GetHandle()))
                {
                    FString pStr = CINI::CurrentDocument->GetString("Triggers", pID);
                    auto results = FString::SplitString(pStr);
                    if (results.size() > 3)
                    {
                        pStr = results[2];
                        FString tmp = pStr;
                        pStr.Format("%s (%s)", pID, tmp);
                        auto idx = SendMessage(CNewTrigger::GetFirstValidInstance().hSelectedTrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)pStr);
                        if (idx != CB_ERR)
                        {
                            SendMessage(CNewTrigger::GetFirstValidInstance().hSelectedTrigger, CB_SETCURSEL, idx, NULL);
                            CNewTrigger::GetFirstValidInstance().OnSelchangeTrigger();
                            finished = true;
                        }
                    }
                }
                if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    FString pStr = CINI::CurrentDocument->GetString(pID, "Name");
                    FString space1 = " (";
                    FString space2 = ")";

                    int idx = SendMessage(CNewTeamTypes::hSelectedTeam, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2));
                    if (idx != CB_ERR)
                    {
                        SendMessage(CNewTeamTypes::hSelectedTeam, CB_SETCURSEL, idx, NULL);
                        CNewTeamTypes::OnSelchangeTeamtypes();
                        finished = true;
                    }

                }

                if (!finished)
                {
                    auto atoms = FString::SplitString(pID);
                    if (atoms.size() >= 11)
                    {
                        int X = atoi(atoms[4]);
                        int Y = atoi(atoms[3]);
    
                        if (CMapData::Instance->IsCoordInMap(X, Y))
                        {
                            CMapDataExt::CellDataExt_FindCell.X = X;
                            CMapDataExt::CellDataExt_FindCell.Y = Y;
                            CMapDataExt::CellDataExt_FindCell.drawCell = true;
    
                            CIsoViewExt::MoveToMapCoord(X, Y);
							finished = true;

							CMapDataExt::CellDataExt_FindCell.drawCell = false;
                        }
                    }
                }

                if (!finished)
                {
                    if (auto pSection = CINI::CurrentDocument->GetSection("CellTags"))
                    {
                        for (auto& pairObj : pSection->GetEntities())
                        {
    
                            if (pairObj.first == pID)
                            {
                                int X = atoi(pairObj.first) / 1000;
                                int Y = atoi(pairObj.first) % 1000;
    
                                if (CMapData::Instance->IsCoordInMap(X, Y))
                                {
                                    CMapDataExt::CellDataExt_FindCell.X = X;
                                    CMapDataExt::CellDataExt_FindCell.Y = Y;
                                    CMapDataExt::CellDataExt_FindCell.drawCell = true;
    
                                    CIsoViewExt::MoveToMapCoord(X, Y);
                                    finished = true;
    
                                    CMapDataExt::CellDataExt_FindCell.drawCell = false;
                                }
                            }
                        }
                    }
                }
            }
            if (finished)
                return TRUE;
            else
                return FALSE;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

BOOL TagSort::OnMessage(PMSG pMsg)
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

void TagSort::Create(HWND hParent)
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

void TagSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    int tabPageheight = 20 * CFinalSunAppExt::ProgramScaleFactor;
    ::MoveWindow(this->GetHwnd(), 2, tabPageheight, rect.right - rect.left - 6, rect.bottom - rect.top - 6 - tabPageheight, FALSE);
}

void TagSort::ShowWindow(bool bShow) const
{
    ::ShowWindow(this->GetHwnd(), bShow ? SW_SHOW : SW_HIDE);
}

void TagSort::ShowWindow() const
{
    this->ShowWindow(true);
}

void TagSort::HideWindow() const
{
    this->ShowWindow(false);
}

void TagSort::ShowMenu(POINT pt) const
{
    HMENU hPopupMenu = ::CreatePopupMenu();
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::AddTrigger,
        Translations::TranslateOrDefault("TagSortNewTag", "New Tag from this group"));
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::Refresh, Translations::TranslateOrDefault("Refresh", "Refresh"));
    ::TrackPopupMenu(hPopupMenu, TPM_VERTICAL | TPM_HORIZONTAL, pt.x, pt.y, NULL, this->GetHwnd(), nullptr);
}

bool TagSort::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool TagSort::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

void TagSort::Menu_AddTrigger()
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

const FString& TagSort::GetCurrentPrefix() const
{
    return this->m_strPrefix;
}

HWND TagSort::GetHwnd() const
{
    return this->m_hWnd;
}

TagSort::operator HWND() const
{
    return this->GetHwnd();
}

std::string TagSort::MakeLabelKey(HTREEITEM hParent, LPCSTR pszLabel)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%p:", hParent);
    return std::string(buf) + pszLabel;
}

void TagSort::IndexAdd(HTREEITEM hParent, LPCSTR pszLabel, HTREEITEM hItem) const
{
    if (hParent && pszLabel && pszLabel[0])
    {    
        m_labelIndex[MakeLabelKey(hParent, pszLabel)] = hItem;
	}
}

void TagSort::IndexRemove(HTREEITEM hParent, LPCSTR pszLabel) const
{
    if (hParent && pszLabel && pszLabel[0])
        m_labelIndex.erase(MakeLabelKey(hParent, pszLabel));
}

void TagSort::IndexClear() const
{
    m_labelIndex.clear();
}

HTREEITEM TagSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    auto key = MakeLabelKey(hItemParent, pszLabel);
    auto it = m_labelIndex.find(key);
    if (it != m_labelIndex.end())
        return it->second;

	return NULL;
}

std::vector<FString> TagSort::GetGroup(FString triggerId, FString& name) const
{
    FString pSrc = CINI::CurrentDocument->GetString("Tags", triggerId, "");

    auto ret = FString::SplitString(pSrc, 1);
    pSrc = ret[1];
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

void TagSort::AddAttachedTrigger(HTREEITEM hParent, FString triggerID, FString parentName) const
{

    if (attachedTriggers.find(TriggerTags[triggerID]) != attachedTriggers.end())
    {
        if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
        {
            FString pTrigger2 = Translations::TranslateOrDefault("Sort.DetectedLoopedTrigger", "Detected Looped Trigger!");
            hParent = hNode;
            auto hItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), pTrigger2, pTrigger2, hParent);
            this->IndexAdd(hParent, pTrigger2, hItem);
            return;
        }  
    }
    
    if (TriggerTags[triggerID] != "")
        if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
        {
            FString pTrigger2 = CINI::CurrentDocument->GetString("Triggers", TriggerTags[triggerID], "");
            auto RET2 = FString::SplitString(pTrigger2, 2);
            pTrigger2 = RET2[2];
            pTrigger2 = FString(Translations::TranslateOrDefault("Sort.AttachedTrigger", "Attached Trigger:")) + " " + pTrigger2;
            hParent = hNode;
            pTrigger2 += " (" + TriggerTags[triggerID] + ")";
            auto hItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), pTrigger2, TriggerTags[triggerID], hParent);
            this->IndexAdd(hParent, pTrigger2, hItem);
            attachedTriggers.insert(TriggerTags[triggerID]);
            AddAttachedTrigger(hParent, TriggerTags[triggerID], pTrigger2);
        }


}

void TagSort::AddAttachedTriggerReverse(HTREEITEM hParent, FString triggerID, FString parentName) const
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
                        auto hItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), pTrigger2, pTrigger2, hParent);
                        this->IndexAdd(hParent, pTrigger2, hItem);
                        return;
                    }
                }

                FString pTrigger2 = CINI::CurrentDocument->GetString("Triggers", parentTrigger, "");
                auto RET2 = FString::SplitString(pTrigger2, 2);
                pTrigger2 = RET2[2];

                pTrigger2 = FString(Translations::TranslateOrDefault("Sort.TriggerAttachedTo", "Trigger Attached To:")) + " " + pTrigger2;
                hParent = hNode;
                pTrigger2 += " (" + parentTrigger + ")";
                auto hItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), pTrigger2, parentTrigger, hParent);
                this->IndexAdd(hParent, pTrigger2, hItem);
                attachedTriggers.insert(parentTrigger);
                AddAttachedTriggerReverse(hParent, parentTrigger, pTrigger2);
            }
        }
                

}

void TagSort::AddTrigger(std::vector<FString> group, FString name, FString id) const
{
    HTREEITEM hParent = TVI_ROOT;
    bool first = true;
    bool attached = false;
    if (AircraftTags[id].size() > 0 || UnitTags[id].size() > 0 
        || InfantryTags[id].size() > 0 || BuildingTags[id].size() > 0 
        || TeamTags[id].size() > 0 || CellTagTags[id].size() > 0)
        attached = true;

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
            if (attached)
            {
                text += " ";
                text += Translations::TranslateOrDefault("Sort.HasAttachedObject", "**Has Attached Object**");
            }
            TreeViewHelper::UpdateTreeItem(this->GetHwnd(), item.hItem, text, id);
            this->IndexAdd(hParent, text, item.hItem);
        }
    }
    else
    {
        FString text = name;
        text += " (" + id + ")";
        if (attached)
        {
            text += " ";
            text += Translations::TranslateOrDefault("Sort.HasAttachedObject", "**Has Attached Object**");
        }
        auto hTagItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), text, id, hParent);
        this->IndexAdd(hParent, text, hTagItem);

        auto tag = CINI::CurrentDocument->GetString("Tags", id);
        auto atoms = FString::SplitString(tag);
        if (atoms.size() >= 3)
        {
            auto& triggerID = atoms[2];
            if (!CINI::CurrentDocument->SectionExists("Triggers"))
                return;

            FString pSrc = CINI::CurrentDocument->GetString("Triggers", triggerID, "");

            if (pSrc == "")
                return;
            auto ret = FString::SplitString(pSrc, 2);
            pSrc = ret[2];

            pSrc = FString(Translations::TranslateOrDefault("Sort.Trigger", "Trigger:")) + " " + pSrc;

            if (HTREEITEM hNode = this->FindLabel(hParent, text))
            {
                hParent = hNode;
                attachedTriggers.clear();
                if (TagTriggers[id] != "")
                {
                    pSrc += " (" + TagTriggers[id] + ")";
                    auto hTriggerItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), pSrc, TagTriggers[id], hParent);
                    this->IndexAdd(hParent, pSrc, hTriggerItem);
                    attachedTriggers.insert(triggerID);
                    AddAttachedTrigger(hParent, triggerID, pSrc);
                }

                hParent = hNode;
                attachedTriggers.clear();

                attachedTriggers.insert(triggerID);
                AddAttachedTriggerReverse(hParent, triggerID, pSrc);

                first = true;
                FString objList = Translations::TranslateOrDefault("Sort.Aircraft", "Aircraft");
                HTREEITEM hParentObj = hParent;
                if (AircraftTags[id].size() > 0)
                {
                    for (auto& pairObj : AircraftTags[id])
                    {
                        auto atomsObj = FString::SplitString(pairObj);

                        if (first)
                        {
                            hParentObj = hNode;
                            hParent = hNode;
                            auto hCatItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), objList, objList, hParent);
                            this->IndexAdd(hParent, objList, hCatItem);
                            first = false;
                        }

                        if (HTREEITEM hNode = this->FindLabel(hParentObj, objList))
                        {
                            FString uiname = CViewObjectsExt::QueryUIName(atomsObj[1]) + " (" + atomsObj[1] + "), " + Translations::TranslateOrDefault("Sort.Coord", "Coordinate") + ": " + atomsObj[3] + ", " + atomsObj[4];
                            hParent = hNode;
                            auto hObjItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), uiname, pairObj, hParent);
                            this->IndexAdd(hParent, uiname, hObjItem);
                        }
                    }
                }
                first = true;
                objList = Translations::TranslateOrDefault("Sort.Structure", "Structure");
                hParentObj = hParent;
                if (BuildingTags[id].size() > 0)
                {
                    for (auto& pairObj : BuildingTags[id])
                    {
                        auto atomsObj = FString::SplitString(pairObj);

                        if (first)
                        {
                            hParentObj = hNode;
                            hParent = hNode;
                            auto hCatItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), objList, objList, hParent);
                            this->IndexAdd(hParent, objList, hCatItem);
                            first = false;
                        }

                        if (HTREEITEM hNode = this->FindLabel(hParentObj, objList))
                        {
                            FString uiname = CViewObjectsExt::QueryUIName(atomsObj[1]) + " (" + atomsObj[1] + "), " + Translations::TranslateOrDefault("Sort.Coord", "Coordinate") + ": " + atomsObj[3] + ", " + atomsObj[4];
                            hParent = hNode;
                            auto hObjItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), uiname, pairObj, hParent);
                            this->IndexAdd(hParent, uiname, hObjItem);
                        }
                    }
                }

                first = true;
                objList = Translations::TranslateOrDefault("Sort.Infantry", "Infantry");
                hParentObj = hParent;
                if (InfantryTags[id].size() > 0)
                {
                    for (auto& pairObj : InfantryTags[id])
                    {
                        auto atomsObj = FString::SplitString(pairObj);

                        if (first)
                        {
                            hParentObj = hNode;
                            hParent = hNode;
                            auto hCatItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), objList, objList, hParent);
                            this->IndexAdd(hParent, objList, hCatItem);
                            first = false;
                        }

                        if (HTREEITEM hNode = this->FindLabel(hParentObj, objList))
                        {
                            FString uiname = CViewObjectsExt::QueryUIName(atomsObj[1]) + " (" + atomsObj[1] + "), " + Translations::TranslateOrDefault("Sort.Coord", "Coordinate") + ": " + atomsObj[3] + ", " + atomsObj[4];
                            hParent = hNode;
                            auto hObjItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), uiname, pairObj, hParent);
                            this->IndexAdd(hParent, uiname, hObjItem);
                        }
                    }
                }
                   
                first = true;
                objList = Translations::TranslateOrDefault("Sort.Vehicle", "Vehicle");
                hParentObj = hParent;
                if (UnitTags[id].size() > 0)
                {
                    for (auto& pairObj : UnitTags[id])
                    {
                        auto atomsObj = FString::SplitString(pairObj);

                        if (first)
                        {
                            hParentObj = hNode;
                            hParent = hNode;
                            auto hCatItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), objList, objList, hParent);
                            this->IndexAdd(hParent, objList, hCatItem);

                            first = false;
                        }

                        if (HTREEITEM hNode = this->FindLabel(hParentObj, objList))
                        {
                            FString uiname = CViewObjectsExt::QueryUIName(atomsObj[1]) + " (" + atomsObj[1] + "), " + Translations::TranslateOrDefault("Sort.Coord", "Coordinate") + ": " + atomsObj[3] + ", " + atomsObj[4];
                            hParent = hNode;
                            auto hObjItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), uiname, pairObj, hParent);
                            this->IndexAdd(hParent, uiname, hObjItem);
                        }
                    }
                }


                first = true;
                objList = Translations::TranslateOrDefault("Sort.Team", "Team");
                HTREEITEM hParentTeam = hParent;

                if (TeamTags[id].size() > 0)
                    for (auto& teamID : TeamTags[id])
                    {
                        if (first)
                        {
                            hParentTeam = hNode;
                            hParent = hNode;
                            auto hCatItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), objList, objList, hParent);
                            this->IndexAdd(hParent, objList, hCatItem);
                            first = false;
                        }

                        if (HTREEITEM hNode = this->FindLabel(hParentTeam, objList))
                        {
                            FString uiname = FString(CINI::CurrentDocument->GetString(teamID, "Name")) + " (" + teamID + ")";

                            hParent = hNode;
                            auto hObjItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), uiname, teamID, hParent);
                            this->IndexAdd(hParent, uiname, hObjItem);
                        }
                    }

                first = true;
                objList = Translations::TranslateOrDefault("Sort.CellTag", "Cell Tag");
                hParentObj = hParent;

                if (CellTagTags[id].size() > 0)
                    for (auto& celltag : CellTagTags[id])
                    {

                        if (first)
                        {
                            hParentObj = hNode;
                            hParent = hNode;
                            auto hCatItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), objList, objList, hParent);
                            this->IndexAdd(hParent, objList, hCatItem);
                            first = false;
                        }

                        int X = atoi(celltag) % 1000;
                        int Y = atoi(celltag) / 1000;


                        if (HTREEITEM hNode = this->FindLabel(hParentObj, objList))
                        {
                            FString uiname;
                            uiname.Format(Translations::TranslateOrDefault("Sort.CellTagCoord", "Coordinate: %d, %d"), X, Y);
                            hParent = hNode;
                            auto hObjItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), uiname, celltag, hParent);
                            this->IndexAdd(hParent, uiname, hObjItem);
                        }
                    }
            }
        }
    }
}

void TagSort::AddTrigger(FString triggerId) const
{
    if (this->IsVisible())
    {
        FString name;
        auto group = this->GetGroup(triggerId, name);
        
        this->AddTrigger(group, name, triggerId);
    }
}
