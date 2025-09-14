#include "Body.h"

#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/FString.h"
#include "../../Miscs/TheaterInfo.h"
#include "../../FA2sp.h"
#include <CINI.h>
#include <CInputMessageBox.h>
#include <CMapData.h>
#include <CIsoView.h>
#include <CTileTypeClass.h>
#include <CLoading.h>
#include "../CTileSetBrowserFrame/Body.h"
#include "../CMapData/Body.h"
#include "../../Miscs/MultiSelection.h"
#include "../../ExtraWindow/CTileManager/CTileManager.h"
#include "../../ExtraWindow/CAnnotationDlg/CAnnotationDlg.h"
#include "../CLoading/Body.h"
#include <Miscs/Miscs.h>
#include <filesystem>
#include "../../Miscs/StringtableLoader.h"

namespace fs = std::filesystem;

std::array<HTREEITEM, CViewObjectsExt::Root_Count> CViewObjectsExt::ExtNodes;
std::unordered_set<FString> CViewObjectsExt::IgnoreSet;
std::unordered_set<FString> CViewObjectsExt::ForceName;
std::unordered_map<FString, FString> CViewObjectsExt::RenameString;
std::unordered_set<FString> CViewObjectsExt::ExtSets[Set_Count];
std::unordered_map<FString, int[10]> CViewObjectsExt::KnownItem;
std::unordered_map<FString, int> CViewObjectsExt::Owners;
std::unordered_set<FString> CViewObjectsExt::AddOnceSet;
int CViewObjectsExt::AddedItemCount;
int CViewObjectsExt::RedrawCalledCount = 0;
bool CViewObjectsExt::IsOpeningAnnotationDlg = false;
int CViewObjectsExt::InsertingTileIndex = -1;
int CViewObjectsExt::InsertingOverlay = -1;
int CViewObjectsExt::InsertingOverlayData = 0;
bool CViewObjectsExt::InsertingSpecialBitmap = false;
FString CViewObjectsExt::InsertingObjectID;
CBitmap CViewObjectsExt::SpecialBitmap;
CImageList CViewObjectsExt::m_ImageList;

std::unique_ptr<CPropertyBuilding> CViewObjectsExt::BuildingBrushDlg;
std::unique_ptr<CPropertyInfantry> CViewObjectsExt::InfantryBrushDlg;
std::unique_ptr<CPropertyUnit> CViewObjectsExt::VehicleBrushDlg;
std::unique_ptr<CPropertyAircraft> CViewObjectsExt::AircraftBrushDlg;

std::unique_ptr<CPropertyBuilding> CViewObjectsExt::BuildingBrushDlgBF;
std::unique_ptr<CPropertyInfantry> CViewObjectsExt::InfantryBrushDlgF;
std::unique_ptr<CPropertyUnit> CViewObjectsExt::VehicleBrushDlgF;
std::unique_ptr<CPropertyAircraft> CViewObjectsExt::AircraftBrushDlgF;
std::unique_ptr<CPropertyBuilding> CViewObjectsExt::BuildingBrushDlgBNF;

bool CViewObjectsExt::BuildingBrushBools[14];
bool CViewObjectsExt::InfantryBrushBools[10];
bool CViewObjectsExt::VehicleBrushBools[11];
bool CViewObjectsExt::AircraftBrushBools[9];

bool CViewObjectsExt::BuildingBrushBoolsBF[14];
bool CViewObjectsExt::InfantryBrushBoolsF[10];
bool CViewObjectsExt::VehicleBrushBoolsF[11];
bool CViewObjectsExt::AircraftBrushBoolsF[9];
bool CViewObjectsExt::BuildingBrushBoolsBNF[14];

std::vector<FString> CViewObjectsExt::ObjectFilterB;
std::vector<FString> CViewObjectsExt::ObjectFilterA;
std::vector<FString> CViewObjectsExt::ObjectFilterI;
std::vector<FString> CViewObjectsExt::ObjectFilterV;
std::vector<FString> CViewObjectsExt::ObjectFilterBN;
std::vector<FString> CViewObjectsExt::ObjectFilterCT;
std::map<int, int> CViewObjectsExt::WallDamageStages;

bool CViewObjectsExt::InitPropertyDlgFromProperty{ false };
int CViewObjectsExt::PlacingWall;
int CViewObjectsExt::PlacingRandomRock;
int CViewObjectsExt::PlacingRandomSmudge;
int CViewObjectsExt::PlacingRandomTerrain;
int CViewObjectsExt::PlacingRandomInfantry;
int CViewObjectsExt::PlacingRandomVehicle;
int CViewObjectsExt::PlacingRandomStructure;
int CViewObjectsExt::PlacingRandomAircraft;
bool CViewObjectsExt::PlacingRandomRandomFacing;
bool CViewObjectsExt::PlacingRandomStructureAIRepairs;
bool CViewObjectsExt::NeedChangeTreeViewSelect = true;
MoveBaseNode CViewObjectsExt::MoveBaseNode_SelectedObj = { "","","",-1,-1 };

const char* playersAtX[8]
{
    "<Player @ A>",
    "<Player @ B>",
    "<Player @ C>",
    "<Player @ D>",
    "<Player @ E>",
    "<Player @ F>",
    "<Player @ G>",
    "<Player @ H>"
};

HTREEITEM CViewObjectsExt::InsertString(const char* pString, DWORD dwItemData,
    HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    AddedItemCount++;
    auto item =  this->GetTreeCtrl().InsertItem(TVIF_TEXT | TVIF_PARAM, pString, 0, 0, 0, 0, dwItemData, hParent, hInsertAfter);
    
    if (ExtConfigs::TreeViewCameo_Display)
    {
        std::string pics = CFinalSunApp::ExePath();
        pics += "\\pics";
        if (fs::exists(pics) && fs::is_directory(pics))
        {
            pics += "\\";
            pics += pString;
            pics += ".bmp";
            if (fs::exists(pics))
            {
                CBitmap cBitmap;
                if (CLoadingExt::LoadBMPToCBitmap(pics.c_str(), cBitmap))
                {
                    int index = m_ImageList.Add(&cBitmap, RGB(255, 255, 255));
                    this->GetTreeCtrl().SetItemImage(item, index, index);
                    return item;
                }
            }
        }       
        if (InsertingSpecialBitmap)
        {
            CIsoViewExt::ScaleBitmap(&SpecialBitmap, ExtConfigs::TreeViewCameo_Size, RGB(255, 0, 255), true, false);
            int index = m_ImageList.Add(&SpecialBitmap, RGB(255, 255, 255));
            this->GetTreeCtrl().SetItemImage(item, index, index);
            return item;
        }
        if (CMapData::Instance->MapWidthPlusHeight)
        {
            if (InsertingTileIndex > -1)
            {
                auto& tile = CMapDataExt::TileData[InsertingTileIndex];
                auto& subTile = tile.TileBlockDatas[0];

                FString fileName;
                fileName.Format("%s-tile-%d-%d.bmp", CINI::CurrentDocument->GetString("Map", "Theater"),
                    InsertingTileIndex, ExtConfigs::TreeViewCameo_Size);

                std::string path = CFinalSunApp::ExePath();
                path += "\\thumbnails";
                if (!fs::exists(path) || !fs::is_directory(path))
                {
                    fs::create_directories(path);
                }
                path += "\\";
                path += fileName;
                if (fs::exists(path))
                {
                    CBitmap cBitmap;
                    if (CLoadingExt::LoadBMPToCBitmap(path.c_str(), cBitmap))
                    {
                        int index = m_ImageList.Add(&cBitmap, RGB(255, 0, 255));
                        this->GetTreeCtrl().SetItemImage(item, index, index);
                        return item;
                    }
                }

                ImageDataClass data;
                data.pImageBuffer = subTile.ImageData;
                data.FullHeight = subTile.BlockHeight;
                data.FullWidth = subTile.BlockWidth;
                data.pPalette = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[InsertingTileIndex].TileSet];

                CBitmap cBitmap;
                CLoadingExt::LoadShpToBitmap(&data, cBitmap);
                CIsoViewExt::ScaleBitmap(&cBitmap, ExtConfigs::TreeViewCameo_Size, RGB(255, 0, 255));
                int index = m_ImageList.Add(&cBitmap, RGB(255, 0, 255));
                this->GetTreeCtrl().SetItemImage(item, index, index);
                CLoadingExt::SaveCBitmapToFile(&cBitmap, path.c_str(), RGB(255, 0, 255));
                return item;
            }

            FString imageName;
            FString fileID;
            auto eItemType = CLoadingExt::GetExtension()->GetItemType(InsertingObjectID);
            switch (eItemType)
            {
            case CLoadingExt::ObjectType::Infantry:
                imageName = CLoadingExt::GetImageName(InsertingObjectID, 5);
                fileID = CLoadingExt::GetExtension()->GetInfantryFileID(InsertingObjectID);
                break;
            case CLoadingExt::ObjectType::Terrain:
            case CLoadingExt::ObjectType::Smudge:
                imageName = CLoadingExt::GetImageName(InsertingObjectID, 0);
                fileID = FString(CINI::CurrentDocument->GetString("Map", "Theater")) + "-" +
                    CLoadingExt::GetExtension()->GetTerrainOrSmudgeFileID(InsertingObjectID);
                break;
            case CLoadingExt::ObjectType::Vehicle:
            case CLoadingExt::ObjectType::Aircraft:
                fileID = CLoadingExt::GetExtension()->GetVehicleOrAircraftFileID(InsertingObjectID);
                break;
            case CLoadingExt::ObjectType::Building:
            {
                imageName = CLoadingExt::GetBuildingImageName(InsertingObjectID, 0, 0);
                fileID = CLoadingExt::GetExtension()->GetBuildingFileID(InsertingObjectID);

                // some buildings share the same image while have different anims
                FString ArtID = CLoadingExt::GetArtID(InsertingObjectID);
                if (CINI::Art->KeyExists(ArtID, "Image") && CINI::Art->GetString(ArtID, "Image") != ArtID)
                {
                    fileID += "-";
                    fileID += ArtID;
                }
                break;
            }
            case CLoadingExt::ObjectType::Unknown:
            default:

                if (InsertingOverlay < 0 && InsertingTileIndex < 0 && !InsertingSpecialBitmap)
                {
                    this->GetTreeCtrl().SetItemImage(item, 0, 0);
                    return item;
                }
                break;
            }
            std::string path = CFinalSunApp::ExePath();
            path += "\\thumbnails";
            if (!fs::exists(path) || !fs::is_directory(path))
            {
                fs::create_directories(path);
            }
            FString fileName;
            fileName.Format("%s-%d.bmp", fileID, ExtConfigs::TreeViewCameo_Size);
            if (InsertingOverlay > -1)
            {
                fileName.Format("%s-overlay-%d-%d-%d.bmp", CINI::CurrentDocument->GetString("Map", "Theater"),
                    InsertingOverlay, InsertingOverlayData, ExtConfigs::TreeViewCameo_Size);
            }
            path += "\\";
            path += fileName;
            if (fs::exists(path))
            {
                CBitmap cBitmap;
                if (CLoadingExt::LoadBMPToCBitmap(path.c_str(), cBitmap))
                {
                    int index = m_ImageList.Add(&cBitmap, RGB(255, 0, 255));
                    this->GetTreeCtrl().SetItemImage(item, index, index);
                    return item;
                }
            }
            if (InsertingOverlay > -1)
            {
                auto imageName = CLoadingExt::GetOverlayName(InsertingOverlay, InsertingOverlayData);
                auto pData = CLoadingExt::GetImageDataFromServer(imageName);
                if (!pData || !pData->pImageBuffer)
                {
                    auto obj = Variables::RulesMap.GetValueAt("OverlayTypes", InsertingOverlay);
                    if (!CLoadingExt::IsOverlayLoaded(obj))
                    {
                        bool temp = ExtConfigs::InGameDisplay_Shadow;
                        ExtConfigs::InGameDisplay_Shadow = false;
                        CLoadingExt::IsLoadingObjectView = true;
                        CLoadingExt::GetExtension()->LoadOverlay(obj, InsertingOverlay);
                        CLoadingExt::IsLoadingObjectView = false;
                        ExtConfigs::InGameDisplay_Shadow = temp;
                        pData = CLoadingExt::GetImageDataFromServer(imageName);
                    }
                }
                if (pData && pData->pImageBuffer)
                {
                    CBitmap cBitmap;
                    CLoadingExt::LoadShpToBitmap(pData, cBitmap);
                    CIsoViewExt::ScaleBitmap(&cBitmap, ExtConfigs::TreeViewCameo_Size, RGB(255, 0, 255));
                    int index = m_ImageList.Add(&cBitmap, RGB(255, 0, 255));
                    this->GetTreeCtrl().SetItemImage(item, index, index);

                    CLoadingExt::SaveCBitmapToFile(&cBitmap, path.c_str(), RGB(255, 0, 255));

                    return item;
                }
            }

            if (!CLoadingExt::IsObjectLoaded(InsertingObjectID)
                && InsertingOverlay < 0 && InsertingTileIndex < 0 && !InsertingSpecialBitmap)
            {
                bool temp = ExtConfigs::InGameDisplay_Shadow;
                bool temp2 = ExtConfigs::InGameDisplay_Deploy;
                bool temp3 = ExtConfigs::InGameDisplay_Water;
                ExtConfigs::InGameDisplay_Shadow = false;
                CLoadingExt::IsLoadingObjectView = true;
                CLoading::Instance->LoadObjects(InsertingObjectID);
                CLoadingExt::IsLoadingObjectView = false;
                ExtConfigs::InGameDisplay_Shadow = temp;
                ExtConfigs::InGameDisplay_Deploy = temp2;
                ExtConfigs::InGameDisplay_Water = temp3;
            }
            if (eItemType == CLoadingExt::ObjectType::Aircraft || eItemType == CLoadingExt::ObjectType::Vehicle)
            {
                int facings = CLoadingExt::GetAvailableFacing(InsertingObjectID);
                imageName = CLoadingExt::GetImageName(InsertingObjectID, facings / 4);
            }
            auto pData = CLoadingExt::GetImageDataFromServer(imageName);
            if (pData && pData->pImageBuffer)
            {
                CBitmap cBitmap;
                CLoadingExt::LoadShpToBitmap(pData, cBitmap);
                CIsoViewExt::ScaleBitmap(&cBitmap, ExtConfigs::TreeViewCameo_Size, RGB(255, 0, 255));
                int index = m_ImageList.Add(&cBitmap, RGB(255, 0, 255));
                this->GetTreeCtrl().SetItemImage(item, index, index);

                CLoadingExt::SaveCBitmapToFile(&cBitmap, path.c_str(), RGB(255, 0, 255));

                return item;
            }
        }
        this->GetTreeCtrl().SetItemImage(item, 0, 0);
    }

    if (!ExtConfigs::TreeViewCameo_Display)
    {
        TVITEM tvi = {};
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.hItem = item;
        tvi.iImage = I_IMAGENONE;
        tvi.iSelectedImage = I_IMAGENONE;
        this->GetTreeCtrl().SetItem(&tvi);
    }
    return item;
}

HTREEITEM CViewObjectsExt::InsertTranslatedString(const char* pOriginString, DWORD dwItemData,
    HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    FString buffer;
    bool result = Translations::GetTranslationItem(pOriginString, buffer);
    return InsertString(result ? buffer.c_str() : pOriginString, dwItemData, hParent, hInsertAfter);
}

FString CViewObjectsExt::QueryUIName(const char* pRegName, bool bOnlyOneLine)
{
    if (!bOnlyOneLine)
    {
        if (ForceName.find(pRegName) != ForceName.end())
            return Variables::RulesMap.GetString(pRegName, "Name", pRegName);
        else
            return StringtableLoader::QueryUIName(pRegName);
    }
    
    FString buffer;

    if (ForceName.find(pRegName) != ForceName.end())
        buffer = Variables::RulesMap.GetString(pRegName, "Name", pRegName);
    else if (RenameString.find(pRegName) != RenameString.end())
        buffer = RenameString[pRegName];
    else
        buffer = StringtableLoader::QueryUIName(pRegName);

    if (buffer == "MISSING")
        buffer = Variables::RulesMap.GetString(pRegName, "Name", pRegName);

    buffer.Replace("\r", "");
    int idx = buffer.Find('\n');
    return idx == -1 ? buffer : buffer.Mid(0, idx);
}

std::vector<int> SplitCommaIntArray(FString input)
{
    CString field;
    std::vector<int> result;
    int index = 0;

    while (AfxExtractSubString(field, input, index, _T(',')))
    {
        result.push_back(STDHelpers::ParseToInt(field, -1));
        ++index;
    }
    return result;
}

int CViewObjectsExt::PropagateFirstNonZeroIcon(HTREEITEM hItem)
{
    if (!hItem) return 0;
    auto& tree = this->GetTreeCtrl();

    int image = 0, selected = 0;
    tree.GetItemImage(hItem, image, selected);

    if (image != 0)
        return image;

    HTREEITEM hChild = tree.GetChildItem(hItem);
    while (hChild)
    {
        int childIcon = PropagateFirstNonZeroIcon(hChild);
        if (childIcon != 0)
        {
            tree.SetItemImage(hItem, childIcon, childIcon);
            return childIcon;
        }

        hChild = tree.GetNextSiblingItem(hChild);
    }
    return 0;
}

void CViewObjectsExt::UpdateTreeIconsForSubtree(HTREEITEM hItem)
{
    if (!hItem) return;
    auto& tree = this->GetTreeCtrl();

    HTREEITEM hChild = tree.GetChildItem(hItem);
    while (hChild)
    {
        UpdateTreeIconsForSubtree(hChild);
        hChild = tree.GetNextSiblingItem(hChild);
    }

    PropagateFirstNonZeroIcon(hItem);
}


void CViewObjectsExt::Redraw()
{
    if (ExtConfigs::TreeViewCameo_Display)
    {
        if (m_ImageList.GetSafeHandle())
            m_ImageList.DeleteImageList();

        m_ImageList.Create(ExtConfigs::TreeViewCameo_Size, ExtConfigs::TreeViewCameo_Size, ILC_COLOR24 | ILC_MASK, 4, 4);
        HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(1002),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        CBitmap cBitmap;
        cBitmap.Attach(hBmp);
        m_ImageList.Add(&cBitmap, RGB(255, 255, 255));
        this->GetTreeCtrl().SetImageList(&m_ImageList, TVSIL_NORMAL);
   
        CFinalSunDlg::Instance->MyViewFrame.SplitterWnd.SetColumnInfo(0, 300, 10);
        CFinalSunDlg::Instance->MyViewFrame.SplitterWnd.RecalcLayout();
        CFinalSunDlg::Instance->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    else
    {
        this->GetTreeCtrl().SetImageList(NULL, TVSIL_NORMAL);
        CFinalSunDlg::Instance->MyViewFrame.SplitterWnd.SetColumnInfo(0, 200, 10);
        CFinalSunDlg::Instance->MyViewFrame.SplitterWnd.RecalcLayout();
        CFinalSunDlg::Instance->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }

    Redraw_Initialize();
    Redraw_MainList();
    Redraw_Ground();
    Redraw_Owner();
    Redraw_Infantry();
    Redraw_Vehicle();
    Redraw_Aircraft();
    Redraw_Building();
    Redraw_Terrain();
    Redraw_Smudge();
    Redraw_Overlay();
    Redraw_Waypoint();
    Redraw_Celltag();
    Redraw_Basenode();
    Redraw_Tunnel();
    Redraw_PlayerLocation(); // player location is just waypoints!
    Redraw_PropertyBrush();
    Redraw_Annotation();
    //Redraw_InfantrySubCell(); // we do not need this any more!
    Redraw_ViewObjectInfo();
    Redraw_MultiSelection();
    Redraw_ConnectedTile(this);

    if (ExtConfigs::TreeViewCameo_Display)
    {
        HTREEITEM hItem = this->GetTreeCtrl().GetRootItem();
        while (hItem)
        {
            UpdateTreeIconsForSubtree(hItem);
            hItem = this->GetTreeCtrl().GetNextSiblingItem(hItem);
        }
    }

    Logger::Raw("[CViewObjectsExt] Redraw TreeView_ViewObjects done. %d labels loaded.\n", AddedItemCount);
}

void CViewObjectsExt::Redraw_Initialize()
{
    // must be here to load after tile view refresh
    if (CTerrainGenerator::GetHandle())
    {
        ::SendMessage(CTerrainGenerator::GetHandle(), 114514, 0, 0);
    }
    if (CTileManager::GetHandle())
    {
        ::SendMessage(CTileManager::GetHandle(), 114514, 0, 0);
    }
    AddedItemCount = 0;

    for (auto root : ExtNodes)
        root = NULL;
    KnownItem.clear();
    IgnoreSet.clear();
    ForceName.clear();
    RenameString.clear();
    Owners.clear();
    this->GetTreeCtrl().DeleteAllItems();

    auto& rules = CINI::Rules();
    auto& fadata = CINI::FAData();
    auto& doc = CINI::CurrentDocument();

    auto loadSet = [](const char* pTypeName, int nType)
    {
        ExtSets[nType].clear();
        auto&& section = Variables::RulesMap.GetSection(pTypeName);
        for (auto& itr : section)
            ExtSets[nType].insert(itr.second);
    };

    loadSet("BuildingTypes", Set_Building);
    loadSet("InfantryTypes", Set_Infantry);
    loadSet("VehicleTypes", Set_Vehicle);
    loadSet("AircraftTypes", Set_Aircraft);

    if (ExtConfigs::ObjectBrowser_GuessMode == 1)
    {
        auto loadOwner = []()
        {
            auto&& sides = Variables::RulesMap.ParseIndicies("Sides", true);
            for (size_t i = 0, sz = sides.size(); i < sz; ++i)
                for (auto& owner : STDHelpers::SplitString(sides[i]))
                    Owners[owner] = i;
        };
        loadOwner();
    }

    if (auto knownSection = fadata.GetSection("ForceSides"))
    {
        for (auto& item : knownSection->GetEntities())
        {
            
            auto forceSides = SplitCommaIntArray(item.second);

            for (int i = 0; i < 9; ++i) {
                if (i < forceSides.size())
                {
                    int sideIndex = forceSides[i];
                    if (sideIndex >= fadata.GetKeyCount("Sides"))
                        sideIndex = -1;
                    if (sideIndex < -1)
                        sideIndex = -1;
                    KnownItem[item.first][i] = sideIndex;
                }
                else
                {
                    KnownItem[item.first][i] = -1;
                }

            }

           
        }
    }

    if (auto ignores = fadata.GetSection("IgnoreRA2"))
        for (auto& item : ignores->GetEntities())
        {
            IgnoreSet.insert(item.second);
        }

    auto theaterIg = doc.GetString("Map", "Theater");
    if (theaterIg != "")
	{
		if (theaterIg == "NEWURBAN")
			theaterIg = "UBN";

        FString suffix = theaterIg.Mid(0, 3);
		if (auto theater_ignores = fadata.GetSection((FString)("IgnoreRA2" + suffix)))
			for (auto& item : theater_ignores->GetEntities())
				IgnoreSet.insert(item.second);
	}

    if (auto forcenames = fadata.GetSection("ForceName"))
        for (auto& item : forcenames->GetEntities())
        {
            ForceName.insert(item.second);
        }

    if (auto forcenames = fadata.GetSection("RenameString"))
        for (auto& item : forcenames->GetEntities())
        {
            RenameString[item.first] = item.second;
        }
    if (theaterIg != "")
    {
        if (theaterIg == "NEWURBAN")
            theaterIg = "UBN";

        FString suffix = theaterIg.Mid(0, 3);

        if (auto forcenames = fadata.GetSection((FString)("RenameString" + suffix)))
            for (auto& item : forcenames->GetEntities())
            {
                RenameString[item.first] = item.second;
            }
    }
}

void CViewObjectsExt::Redraw_MainList()
{
    auto LoadNodeWithCameo = [this](int node, int index, const char* name, int bmp)
        {    
            HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(bmp),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            SpecialBitmap.Attach(hBmp);
            InsertingSpecialBitmap = true;
            ExtNodes[node] = this->InsertTranslatedString(name, index);
            InsertingSpecialBitmap = false;
        };

    LoadNodeWithCameo(Root_Nothing, -2, "NothingObList", 1002);
    LoadNodeWithCameo(Root_Ground, -1, "GroundObList", 1013);
    LoadNodeWithCameo(Root_Owner, -1, "ChangeOwnerObList", 1014);
    LoadNodeWithCameo(Root_Infantry, -1, "InfantryObList", 1015);
    LoadNodeWithCameo(Root_Vehicle, -1, "VehiclesObList", 1017);
    LoadNodeWithCameo(Root_Aircraft, -1, "AircraftObList", 1016);
    LoadNodeWithCameo(Root_Building, -1, "StructuresObList", 1018);
    LoadNodeWithCameo(Root_Terrain, -1, "TerrainObList", 1019);
    LoadNodeWithCameo(Root_Smudge, -1, "SmudgesObList", 1020);
    LoadNodeWithCameo(Root_Overlay, -1, "OverlayObList", 1021);
    LoadNodeWithCameo(Root_Waypoint, -1, "WaypointsObList", 1003);
    LoadNodeWithCameo(Root_Celltag, -1, "CelltagsObList", 1004);
    LoadNodeWithCameo(Root_Basenode, -1, "BaseNodesObList", 1010);
    LoadNodeWithCameo(Root_Tunnel, -1, "TunnelObList", 1005);
    LoadNodeWithCameo(Root_PlayerLocation, -1, "StartpointsObList", 1022);
    LoadNodeWithCameo(Root_PropertyBrush, -1, "PropertyBrushObList", 1006);
    LoadNodeWithCameo(Root_Annotation, -1, "AnnotationObList", 1007);
    LoadNodeWithCameo(Root_View, Const_ViewObjectInfo + ObjectTerrainType::All, "ViewObjObList", 1008);
    if (ExtConfigs::EnableMultiSelection)
        LoadNodeWithCameo(Root_MultiSelection, -1, "MultiSelectionObjObList", 1009);
    LoadNodeWithCameo(Root_Cliff, -1, "CliffObjObList", 1012);
    LoadNodeWithCameo(Root_Delete, 10, "DelObjObList", 1011);
}

void CViewObjectsExt::Redraw_Ground()
{
    HTREEITEM& hGround = ExtNodes[Root_Ground];
    if (hGround == NULL)    return;

    auto& doc = CINI::CurrentDocument();
    auto theater = doc.GetString("Map", "Theater");
    if (theater == "NEWURBAN")
        theater = "UBN";

    FString suffix;
    if (theater != "")
        suffix = theater.Mid(0, 3);

    auto setTileIndex = [](int tileSet)
        {
            if (CMapData::Instance->MapWidthPlusHeight)
            {
                if (tileSet < 0)
                    InsertingTileIndex = -1;
                else
                    InsertingTileIndex = CMapDataExt::TileSet_starts[tileSet];
            }
        };
    auto setTileIndexByName = [setTileIndex](FString name)
        {           
            if (CMapData::Instance->MapWidthPlusHeight)
            {
                setTileIndex(CINI::CurrentTheater->GetInteger("General", name, -1));
            }
        };

    setTileIndexByName("ClearTile");
    this->InsertTranslatedString("GroundClearObList" + suffix, 61, hGround);
    if (suffix != "LUN")
    {
        setTileIndexByName("SandTile");
        this->InsertTranslatedString("GroundSandObList" + suffix, 62, hGround);
    }
    if (suffix != "URB")
    {
        setTileIndexByName("RoughTile");
        this->InsertTranslatedString("GroundRoughObList" + suffix, 63, hGround);
    }
    setTileIndexByName("GreenTile");
    this->InsertTranslatedString("GroundGreenObList" + suffix, 65, hGround);
    if (suffix != "UBN")
    {
        setTileIndexByName("PaveTile");
        this->InsertTranslatedString("GroundPaveObList" + suffix, 66, hGround);
    }
    
    if (suffix != "LUN")
    {
        setTileIndexByName("WaterSet");
        this->InsertTranslatedString("GroundWaterObList", 64, hGround);
    }
    else if (suffix == "LUN" && ExtConfigs::LoadLunarWater)
    {
        setTileIndexByName("WaterSet");
        this->InsertTranslatedString("GroundWaterObList", 64, hGround);
    }

    if (CINI::CurrentTheater)
    {
        int i = 67;
        for (auto& morphables : TheaterInfo::CurrentInfo)
        {
            auto InsertTile = [&](int nTileset)
            {
                FA2sp::Buffer.Format("TileSet%04d", nTileset);
                FA2sp::Buffer = CINI::CurrentTheater->GetString(FA2sp::Buffer, "SetName", FA2sp::Buffer);
                FString buffer;
                Translations::GetTranslationItem(FA2sp::Buffer, FA2sp::Buffer);
                setTileIndex(nTileset);
                return this->InsertString(FA2sp::Buffer, i++, hGround, TVI_LAST);
            };

            FString buffer;
            buffer.Format("TileSet%04d", morphables.Morphable);
            FString buffer2;
            buffer2.Format("TileSet%04d", morphables.Ramp);
            auto exist = CINI::CurrentTheater->GetBool(buffer, "AllowToPlace", true);
            auto exist2 = CINI::CurrentTheater->GetString(buffer, "FileName", "");
            auto exist3 = CINI::CurrentTheater->GetString(buffer2, "FileName", "");
            if (exist && strcmp(exist2, "") != 0 && strcmp(exist3, "") != 0)
                InsertTile(morphables.Morphable);
            else
                i++;
        }
        for (auto& morphables : TheaterInfo::CurrentInfoNonMorphable)
        {
            bool met = false;
            for (auto& morphables2 : TheaterInfo::CurrentInfo)
            {
                if (morphables.Morphable == morphables2.Morphable)
                    met = true;
            }
            if (met)
                continue;
            auto InsertTile = [&](int nTileset)
                {
                    FA2sp::Buffer.Format("TileSet%04d", nTileset);
                    FA2sp::Buffer = CINI::CurrentTheater->GetString(FA2sp::Buffer, "SetName", FA2sp::Buffer);
                    FString buffer;
                    Translations::GetTranslationItem(FA2sp::Buffer, FA2sp::Buffer);
                    setTileIndex(nTileset);
                    return this->InsertString(FA2sp::Buffer, i++, hGround, TVI_LAST);
                };

            FString buffer;
            buffer.Format("TileSet%04d", morphables.Morphable);
            FString buffer2;
            buffer2.Format("TileSet%04d", morphables.Ramp);
            auto exist = CINI::CurrentTheater->GetBool(buffer, "AllowToPlace", true);
            auto exist2 = CINI::CurrentTheater->GetString(buffer, "FileName", "");
            auto exist3 = CINI::CurrentTheater->GetString(buffer2, "FileName", "");
            if (exist && strcmp(exist2, "") != 0 && strcmp(exist3, "") != 0)
                InsertTile(morphables.Morphable);
            else
                i++;
        }
    }
    InsertingTileIndex = -1;
}

void CViewObjectsExt::Redraw_Owner()
{
    HTREEITEM& hOwner = ExtNodes[Root_Owner];
    if (hOwner == NULL)    return;

    auto& countries = CINI::Rules->GetSection("Countries")->GetEntities();
    FString translated;

    if (ExtConfigs::ObjectBrowser_SafeHouses)
    {
        if (CMapData::Instance->IsMultiOnly())
        {
            auto&& section = Variables::RulesMap.GetSection("Countries");
            auto itr = section.begin();
            for (size_t i = 0, sz = section.size(); i < sz; ++i, ++itr)
                if (strcmp(itr->second, "Neutral") == 0 || strcmp(itr->second, "Special") == 0)
                {
                    FString uiname = itr->second;

                    if (!ExtConfigs::NoHouseNameTranslation)
                        for (auto& pair : countries)
                        {
                            if (ExtConfigs::BetterHouseNameTranslation)
                                translated = StringtableLoader::QueryUIName(pair.second, true) + "(" + pair.second + ")";
                            else
                                translated = StringtableLoader::QueryUIName(pair.second, true);

                            uiname.Replace(pair.second, translated);
                        }

                    if (ExtConfigs::TreeViewCameo_Display)
                    {
                        InsertingSpecialBitmap = true;
                        int full = ExtConfigs::TreeViewCameo_Size;
                        int half = ExtConfigs::TreeViewCameo_Size / 2;
                        int quarter = ExtConfigs::TreeViewCameo_Size / 4;
                        SpecialBitmap.CreateBitmap(full, full, 1, 32, NULL);

                        CDC dc;
                        dc.CreateCompatibleDC(NULL);
                        CBitmap* pOldBitmap = dc.SelectObject(&SpecialBitmap);
                        dc.FillSolidRect(0, 0, full, full, RGB(255, 255, 255));
                        dc.FillSolidRect(quarter, quarter, half, half, Miscs::GetColorRef(itr->second));
                        dc.SelectObject(pOldBitmap);
                        dc.DeleteDC();
                    }
                    this->InsertString(uiname, Const_House + i, hOwner);
                    InsertingSpecialBitmap = false;
                }
            if (ExtConfigs::PlayerAtXForTechnos)
            {
                int index = 0;
                for (const auto& player : playersAtX)
                {
                    if (ExtConfigs::TreeViewCameo_Display)
                    {
                        InsertingSpecialBitmap = true;
                        int full = ExtConfigs::TreeViewCameo_Size;
                        int half = ExtConfigs::TreeViewCameo_Size / 2;
                        int quarter = ExtConfigs::TreeViewCameo_Size / 4;
                        SpecialBitmap.CreateBitmap(full, full, 1, 32, NULL);

                        CDC dc;
                        dc.CreateCompatibleDC(NULL);
                        CBitmap* pOldBitmap = dc.SelectObject(&SpecialBitmap);
                        dc.FillSolidRect(0, 0, full, full, RGB(255, 255, 255));
                        dc.FillSolidRect(quarter, quarter, half, half, Miscs::GetColorRef(player));
                        dc.SelectObject(pOldBitmap);
                        dc.DeleteDC();
                    }
                    this->InsertString(player, Const_House + 5000 + index++, hOwner);
                    InsertingSpecialBitmap = false;
                }
            }          
        }
        else
        {
            if (auto pSection = CINI::CurrentDocument->GetSection("Houses"))
            {
                auto& section = pSection->GetEntities();
                size_t i = 0;
                for (auto& itr : section)
                {
                    FString uiname = itr.second;

                    if (!ExtConfigs::NoHouseNameTranslation)
                    for (auto& pair : countries)
                    {
                        if (ExtConfigs::BetterHouseNameTranslation)
                            translated = StringtableLoader::QueryUIName(pair.second, true) + "(" + pair.second + ")";
                        else
                            translated = StringtableLoader::QueryUIName(pair.second, true);

                        uiname.Replace(pair.second, translated);
                    }
                    if (ExtConfigs::TreeViewCameo_Display)
                    {
                        InsertingSpecialBitmap = true;
                        int full = ExtConfigs::TreeViewCameo_Size;
                        int half = ExtConfigs::TreeViewCameo_Size / 2;
                        int quarter = ExtConfigs::TreeViewCameo_Size / 4;
                        SpecialBitmap.CreateBitmap(full, full, 1, 32, NULL);

                        CDC dc;
                        dc.CreateCompatibleDC(NULL);
                        CBitmap* pOldBitmap = dc.SelectObject(&SpecialBitmap);
                        dc.FillSolidRect(0, 0, full, full, RGB(255, 255, 255));
                        dc.FillSolidRect(quarter, quarter, half, half, Miscs::GetColorRef(itr.second));
                        dc.SelectObject(pOldBitmap);
                        dc.DeleteDC();
                    }
                    this->InsertString(uiname, Const_House + i, hOwner);
                    InsertingSpecialBitmap = false;
                    i++;
                }
                    
            }    
        }
    }
    else
    {
        if (CMapData::Instance->IsMultiOnly())
        {
            if (auto pSection = CINI::Rules->GetSection("Countries"))
            {
                auto& section = pSection->GetEntities();
                size_t i = 0;
                for (auto& itr : section)
                {
                    FString uiname = itr.second;

                    if (!ExtConfigs::NoHouseNameTranslation)
                    for (auto& pair : countries)
                    {
                        if (ExtConfigs::BetterHouseNameTranslation)
                            translated = StringtableLoader::QueryUIName(pair.second, true) + "(" + pair.second + ")";
                        else
                            translated = StringtableLoader::QueryUIName(pair.second, true);

                        uiname.Replace(pair.second, translated);
                    }
                    if (ExtConfigs::TreeViewCameo_Display)
                    {
                        InsertingSpecialBitmap = true;
                        int full = ExtConfigs::TreeViewCameo_Size;
                        int half = ExtConfigs::TreeViewCameo_Size / 2;
                        int quarter = ExtConfigs::TreeViewCameo_Size / 4;
                        SpecialBitmap.CreateBitmap(full, full, 1, 32, NULL);

                        CDC dc;
                        dc.CreateCompatibleDC(NULL);
                        CBitmap* pOldBitmap = dc.SelectObject(&SpecialBitmap);
                        dc.FillSolidRect(0, 0, full, full, RGB(255, 255, 255));
                        dc.FillSolidRect(quarter, quarter, half, half, Miscs::GetColorRef(itr.second));
                        dc.SelectObject(pOldBitmap);
                        dc.DeleteDC();
                    }
                    this->InsertString(uiname, Const_House + i, hOwner);
                    InsertingSpecialBitmap = false;
                    i++;
                }
                if (ExtConfigs::PlayerAtXForTechnos)
                {
                    int index = 0;
                    for (const auto& player : playersAtX)
                    {
                        if (ExtConfigs::TreeViewCameo_Display)
                        {
                            InsertingSpecialBitmap = true;
                            int full = ExtConfigs::TreeViewCameo_Size;
                            int half = ExtConfigs::TreeViewCameo_Size / 2;
                            int quarter = ExtConfigs::TreeViewCameo_Size / 4;
                            SpecialBitmap.CreateBitmap(full, full, 1, 32, NULL);

                            CDC dc;
                            dc.CreateCompatibleDC(NULL);
                            CBitmap* pOldBitmap = dc.SelectObject(&SpecialBitmap);
                            dc.FillSolidRect(0, 0, full, full, RGB(255, 255, 255));
                            dc.FillSolidRect(quarter, quarter, half, half, Miscs::GetColorRef(player));
                            dc.SelectObject(pOldBitmap);
                            dc.DeleteDC();
                        }
                        this->InsertString(player, Const_House + 5000 + index++, hOwner);
                        InsertingSpecialBitmap = false;
                    }
                }
            }
        }
        else
        {
            if (auto pSection = CINI::CurrentDocument->GetSection("Houses"))
            {
                auto& section = pSection->GetEntities();
                size_t i = 0;
                for (auto& itr : section)
                {
                    FString uiname = itr.second;

                    if (!ExtConfigs::NoHouseNameTranslation)
                    for (auto& pair : countries)
                    {
                        if (ExtConfigs::BetterHouseNameTranslation)
                            translated = StringtableLoader::QueryUIName(pair.second, true) + "(" + pair.second + ")";
                        else
                            translated = StringtableLoader::QueryUIName(pair.second, true);

                        uiname.Replace(pair.second, translated);
                    }
                    if (ExtConfigs::TreeViewCameo_Display)
                    {
                        InsertingSpecialBitmap = true;
                        int full = ExtConfigs::TreeViewCameo_Size;
                        int half = ExtConfigs::TreeViewCameo_Size / 2;
                        int quarter = ExtConfigs::TreeViewCameo_Size / 4;
                        SpecialBitmap.CreateBitmap(full, full, 1, 32, NULL);

                        CDC dc;
                        dc.CreateCompatibleDC(NULL);
                        CBitmap* pOldBitmap = dc.SelectObject(&SpecialBitmap);
                        dc.FillSolidRect(0, 0, full, full, RGB(255, 255, 255));
                        dc.FillSolidRect(quarter, quarter, half, half, Miscs::GetColorRef(itr.second));
                        dc.SelectObject(pOldBitmap);
                        dc.DeleteDC();
                    }
                    this->InsertString(uiname, Const_House + i, hOwner);
                    InsertingSpecialBitmap = false;
                    i++;
                }
            }
        }
    }
}

static std::vector<int> GetUnique(std::vector<int> input) 
{
    std::sort(input.begin(), input.end());
    auto last = std::unique(input.begin(), input.end());
    input.erase(last, input.end());
    return input;
}

void CViewObjectsExt::Redraw_Infantry()
{
    AddOnceSet.clear();
    HTREEITEM& hInfantry = ExtNodes[Root_Infantry];
    if (hInfantry == NULL)   return;

    std::map<int, HTREEITEM> subNodes;

    auto& fadata = CINI::FAData();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hInfantry);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hInfantry);
        subNodes[i++] = this->InsertString("Soviet", -1, hInfantry);
        subNodes[i++] = this->InsertString("Yuri", -1, hInfantry);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hInfantry);

    auto&& infantries = Variables::RulesMap.GetSection("InfantryTypes");
    for (auto& inf : infantries)
    {
        if (AddOnceSet.find(inf.second) != AddOnceSet.end())
            continue;
        AddOnceSet.insert(inf.second);

        if (IgnoreSet.find(inf.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(inf.first, -1);
        if (index == -1)   continue;

        InsertingObjectID = inf.second;
        auto sides = GetUnique(GuessSide(inf.second, Set_Infantry));
        if (!sides.empty())
        {
            FString display = QueryUIName(inf.second);
            if (display != inf.second)
                display += " (" + inf.second + ")";
            if (sides.size() == 1 && sides[0] == -1)
            {
                this->InsertString(
                    display,
                    Const_Infantry + index,
                    subNodes[-1]
                );
            }
            else
            {
                for (int side : sides) {
                    if (subNodes.find(side) == subNodes.end())
                        side = -1;
                    if (side == -1) continue;
                    this->InsertString(
                        display,
                        Const_Infantry + index,
                        subNodes[side]
                    );
                }
            }   
        }
        InsertingObjectID = "";
    }
    
    HTREEITEM hTemp = this->InsertTranslatedString("PlaceRandomInfantryObList", -1, hInfantry);
    if (auto pSection = CINI::FAData().GetSection("PlaceRandomInfantryObList"))
    {
        int index = RandomTechno;
        for (const auto& pKey : pSection->GetEntities())
        {
            if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
            {
                bool add = true;
                auto banned = STDHelpers::SplitString(CINI::FAData().GetString(pKey.second, "BannedTheater", ""));
                if (banned.size() > 0)
                    for (auto& ban : banned)
                        if (ban == CINI::CurrentDocument().GetString("Map", "Theater"))
                            add = false;
                if (add)
                {
                    InsertingObjectID = CINI::FAData().GetString(pKey.second, "0");
                    this->InsertString(CINI::FAData().GetString(pKey.second, "Name", "MISSING"), Const_Infantry + index, hTemp);
                    InsertingObjectID = "";
                }
                index++;
            }
        }
    }

    // Clean up
    if (ExtConfigs::ObjectBrowser_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->GetTreeCtrl().ItemHasChildren(subnode.second))
                this->GetTreeCtrl().DeleteItem(subnode.second);
        }
    }
}

void CViewObjectsExt::Redraw_Vehicle()
{
    AddOnceSet.clear();
    HTREEITEM& hVehicle = ExtNodes[Root_Vehicle];
    if (hVehicle == NULL)   return;

    std::map<int, HTREEITEM> subNodes;

    auto& fadata = CINI::FAData();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hVehicle);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hVehicle);
        subNodes[i++] = this->InsertString("Soviet", -1, hVehicle);
        subNodes[i++] = this->InsertString("Yuri", -1, hVehicle);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hVehicle);

    auto&& vehicles = Variables::RulesMap.GetSection("VehicleTypes");
    for (auto& veh : vehicles)
    {
        if (AddOnceSet.find(veh.second) != AddOnceSet.end())
            continue;
        AddOnceSet.insert(veh.second);

        if (IgnoreSet.find(veh.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(veh.first, -1);
        if (index == -1)   continue;

        InsertingObjectID = veh.second;
        auto sides = GetUnique(GuessSide(veh.second, Set_Vehicle));
        if (!sides.empty())
        {
            FString display = QueryUIName(veh.second);
            if (display != veh.second)
                display += " (" + veh.second + ")";
            if (sides.size() == 1 && sides[0] == -1)
            {
                this->InsertString(
                    display,
                    Const_Vehicle + index,
                    subNodes[-1]
                );
            }
            else
            {
                for (int side : sides) {
                    if (subNodes.find(side) == subNodes.end())
                        side = -1;
                    if (side == -1) continue;
                    this->InsertString(
                        display,
                        Const_Vehicle + index,
                        subNodes[side]
                    );
                }
            }   
        }
        InsertingObjectID = "";
    }

    HTREEITEM hTemp = this->InsertTranslatedString("PlaceRandomVehicleObList", -1, hVehicle);
    if (auto pSection = CINI::FAData().GetSection("PlaceRandomVehicleObList"))
    {
        int index = RandomTechno;
        for (const auto& pKey : pSection->GetEntities())
        {
            if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
            {
                bool add = true;
                auto banned = STDHelpers::SplitString(CINI::FAData().GetString(pKey.second, "BannedTheater", ""));
                if (banned.size() > 0)
                    for (auto& ban : banned)
                        if (ban == CINI::CurrentDocument().GetString("Map", "Theater"))
                            add = false;
                if (add)
                {
                    InsertingObjectID = CINI::FAData().GetString(pKey.second, "0");
                    this->InsertString(CINI::FAData().GetString(pKey.second, "Name", "MISSING"), Const_Vehicle + index, hTemp);
                    InsertingObjectID = "";
                }
                index++;
            }
        }
    }

    // Clear up
    if (ExtConfigs::ObjectBrowser_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->GetTreeCtrl().ItemHasChildren(subnode.second))
                this->GetTreeCtrl().DeleteItem(subnode.second);
        }
    }
}

void CViewObjectsExt::Redraw_Aircraft()
{
    AddOnceSet.clear();
    HTREEITEM& hAircraft = ExtNodes[Root_Aircraft];
    if (hAircraft == NULL)   return;

    std::map<int, HTREEITEM> subNodes;

    auto& rules = CINI::Rules();
    auto& fadata = CINI::FAData();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hAircraft);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hAircraft);
        subNodes[i++] = this->InsertString("Soviet", -1, hAircraft);
        subNodes[i++] = this->InsertString("Yuri", -1, hAircraft);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hAircraft);

    auto&& aircrafts = Variables::RulesMap.GetSection("AircraftTypes");
    for (auto& air : aircrafts)
    {
        if (AddOnceSet.find(air.second) != AddOnceSet.end())
            continue;
        AddOnceSet.insert(air.second);


        if (IgnoreSet.find(air.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(air.first, -1);
        if (index == -1)   continue;
        
        InsertingObjectID = air.second;
        auto sides = GetUnique(GuessSide(air.second, Set_Aircraft));
        if (!sides.empty())
        {
            FString display = QueryUIName(air.second);
            if (display != air.second)
                display += " (" + air.second + ")";
            if (sides.size() == 1 && sides[0] == -1)
            {
                this->InsertString(
                    display,
                    Const_Aircraft + index,
                    subNodes[-1]
                );
            }
            else
            {
                for (int side : sides) {
                    if (subNodes.find(side) == subNodes.end())
                        side = -1;
                    if (side == -1) continue;
                    this->InsertString(
                        display,
                        Const_Aircraft + index,
                        subNodes[side]
                    );
                }
            }   
        }
        InsertingObjectID = "";
    }
    HTREEITEM hTemp = this->InsertTranslatedString("PlaceRandomAircraftObList", -1, hAircraft);
    if (auto pSection = CINI::FAData().GetSection("PlaceRandomAircraftObList"))
    {
        int index = RandomTechno;
        for (const auto& pKey : pSection->GetEntities())
        {
            if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
            {
                bool add = true;
                auto banned = STDHelpers::SplitString(CINI::FAData().GetString(pKey.second, "BannedTheater", ""));
                if (banned.size() > 0)
                    for (auto& ban : banned)
                        if (ban == CINI::CurrentDocument().GetString("Map", "Theater"))
                            add = false;
                if (add)
                {
                    InsertingObjectID = CINI::FAData().GetString(pKey.second, "0");
                    this->InsertString(CINI::FAData().GetString(pKey.second, "Name", "MISSING"), Const_Aircraft + index, hTemp);
                    InsertingObjectID = "";
                }
                index++;
            }
        }
    }

    // Clear up
    if (ExtConfigs::ObjectBrowser_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->GetTreeCtrl().ItemHasChildren(subnode.second))
                this->GetTreeCtrl().DeleteItem(subnode.second);
        }
    }
}

void CViewObjectsExt::Redraw_Building()
{
    AddOnceSet.clear();
    HTREEITEM& hBuilding = ExtNodes[Root_Building];
    if (hBuilding == NULL)   return;

    std::map<int, HTREEITEM> subNodes;
    std::map<int, std::vector<std::pair<int, FString>>> foundationBuildings;

    auto& rules = CINI::Rules();
    auto& fadata = CINI::FAData();
    auto& art = CINI::Art();
    auto& doc = CINI::CurrentDocument();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hBuilding);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hBuilding);
        subNodes[i++] = this->InsertString("Soviet", -1, hBuilding);
        subNodes[i++] = this->InsertString("Yuri", -1, hBuilding);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hBuilding);
    
    auto&& buildings = Variables::RulesMap.GetSection("BuildingTypes");
    for (auto& bud : buildings)
    {
        if (AddOnceSet.find(bud.second) != AddOnceSet.end())
            continue;
        AddOnceSet.insert(bud.second);

        if (IgnoreSet.find(bud.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(bud.first, -1);
        if (index == -1)   continue;

        InsertingObjectID = bud.second;
        auto sides = GetUnique(GuessSide(bud.second, Set_Building));
        if (!sides.empty())
        {
            FString display = QueryUIName(bud.second);
            if (display != bud.second)
                display += " (" + bud.second + ")";
            if (sides.size() == 1 && sides[0] == -1)
            {
                this->InsertString(
                    display,
                    Const_Building + index,
                    subNodes[-1]
                );
            }
            else
            {
                for (int side : sides) {
                    if (subNodes.find(side) == subNodes.end())
                        side = -1;
                    if (side == -1) continue;
                    this->InsertString(
                        display,
                        Const_Building + index,
                        subNodes[side]
                    );
                }
            }
        }

        if (CMapData::Instance->MapWidthPlusHeight && ExtConfigs::ObjectBrowser_Foundation)
        {
            const int BuildingIndex = CMapData::Instance->GetBuildingTypeID(bud.second);
            const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
            foundationBuildings[DataExt.Width * 100 + DataExt.Height].push_back(std::make_pair(index, bud.second));
        }
    }
    InsertingObjectID = "";

    for (const auto& [foundation, buildingList] : foundationBuildings)
    {
        if (buildingList.empty())
            continue;
        FString name;
        name.Format("%dx%d %s", foundation / 100, foundation % 100, Translations::TranslateOrDefault("FoundationBuildingObList", "Building"));
        HTREEITEM hGroup = this->InsertString(name, -1, hBuilding);

        HTREEITEM hFirstChild = nullptr;
        for (const auto& buildingID : buildingList)
        {
            InsertingObjectID = buildingID.second;
            auto hChild = this->InsertString(
                QueryUIName(buildingID.second) + " (" + buildingID.second + ")",
                Const_Building + buildingID.first, hGroup);

            if (!hFirstChild)
                hFirstChild = hChild;
        }
        if (ExtConfigs::TreeViewCameo_Display && hFirstChild)
        {
            int image = 0, sel = 0;
            this->GetTreeCtrl().GetItemImage(hFirstChild, image, sel);
            if (image != 0)
                this->GetTreeCtrl().SetItemImage(hGroup, image, image);
        }
    }
    InsertingObjectID = "";

    HTREEITEM hTemp = this->InsertTranslatedString("PlaceRandomBuildingObList", -1, hBuilding);
    if (auto pSection = CINI::FAData().GetSection("PlaceRandomBuildingObList"))
    {
        int index = RandomTechno;
        for (const auto& pKey : pSection->GetEntities())
        {
            if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
            {
                bool add = true;
                auto banned = STDHelpers::SplitString(CINI::FAData().GetString(pKey.second, "BannedTheater", ""));
                if (banned.size() > 0)
                    for (auto& ban : banned)
                        if (ban == CINI::CurrentDocument().GetString("Map", "Theater"))
                            add = false;
                if (add)
                {
                    InsertingObjectID = CINI::FAData().GetString(pKey.second, "0");
                    this->InsertString(CINI::FAData().GetString(pKey.second, "Name", "MISSING"), Const_Building + index, hTemp);
                    InsertingObjectID = "";
                }
                index++;
            }
        }
    }

    // Clear up
    if (ExtConfigs::ObjectBrowser_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->GetTreeCtrl().ItemHasChildren(subnode.second))
                this->GetTreeCtrl().DeleteItem(subnode.second);
        }
    }
}

void CViewObjectsExt::Redraw_Terrain()
{
    HTREEITEM& hTerrain = ExtNodes[Root_Terrain];
    if (hTerrain == NULL)   return;

    std::vector<std::pair<HTREEITEM, FString>> nodes;

    if (auto pSection = CINI::FAData->GetSection("ObjectBrowser.TerrainTypes"))
    {
        std::map<int, FString> collector;

        for (auto& pair : pSection->GetIndices())
            collector[pair.second] = pair.first;

        for (auto& pair : collector)
        {
            const auto& contains = pair.second;
            const auto& translation = pSection->GetEntities().find(contains)->second;

            nodes.push_back(std::make_pair(this->InsertTranslatedString(translation, -1, hTerrain), contains));
        }
    }
    HTREEITEM hOther = this->InsertTranslatedString("OthObList", -1, hTerrain);

    auto&& terrains = Variables::RulesMap.ParseIndicies("TerrainTypes", true);
    for (size_t i = 0, sz = terrains.size(); i < sz; ++i)
    {
        if (IgnoreSet.find(terrains[i]) == IgnoreSet.end())
        {
            FA2sp::Buffer = QueryUIName(terrains[i]);
            if (FA2sp::Buffer != terrains[i])
                FA2sp::Buffer += " (" + terrains[i] + ")";
            bool bNotOther = false;
            InsertingObjectID = terrains[i];
            for (const auto& node : nodes)
            {
                if (terrains[i].Find(node.second.c_str()) >= 0)
                {
                    this->InsertString(FA2sp::Buffer, Const_Terrain + i, node.first);
                    bNotOther = true;
                }
            }
            if (!bNotOther)
                this->InsertString(FA2sp::Buffer, Const_Terrain + i, hOther);
            InsertingObjectID = "";
        }
    }

    this->InsertTranslatedString("RndTreeObList", 50999, hTerrain);


    HTREEITEM hTemp = this->InsertTranslatedString("PlaceRandomTreeObList", -1, hTerrain);
    if (auto pSection = CINI::FAData().GetSection("PlaceRandomTreeObList"))
    {
        int index = RandomTree;
        for (const auto& pKey : pSection->GetEntities())
        {
            if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
            {
                bool add = true;
                auto banned = STDHelpers::SplitString(CINI::FAData().GetString(pKey.second, "BannedTheater", ""));
                if (banned.size() > 0)
                    for (auto& ban : banned)
                        if (ban == CINI::CurrentDocument().GetString("Map", "Theater"))
                            add = false;
                if (add)
                {
                    InsertingObjectID = CINI::FAData().GetString(pKey.second, "0");
                    this->InsertString(CINI::FAData().GetString(pKey.second, "Name", "MISSING"), Const_Terrain + index, hTemp);
                    InsertingObjectID = "";
                }
                index++;
            }
        }
    }

}

void CViewObjectsExt::Redraw_Smudge()
{
    HTREEITEM& hSmudge = ExtNodes[Root_Smudge];
    if (hSmudge == NULL)   return;

    std::vector<std::pair<HTREEITEM, FString>> nodes;

    if (auto pSection = CINI::FAData->GetSection("ObjectBrowser.SmudgeTypes"))
    {
        std::map<int, FString> collector;

        for (auto& pair : pSection->GetIndices())
            collector[pair.second] = pair.first;

        for (auto& pair : collector)
        {
            const auto& contains = pair.second;
            const auto& translation = pSection->GetEntities().find(contains)->second;

            nodes.push_back(std::make_pair(this->InsertTranslatedString(translation, -1, hSmudge), contains));
        }
    }

    HTREEITEM hRandomSmudge = this->InsertTranslatedString("PlaceRandomSmudgeList", -1, hSmudge);
    if (auto pSection = CINI::FAData().GetSection("PlaceRandomSmudgeList"))
    {
        int index = random1x1crater;
        for (const auto& pKey : pSection->GetEntities())
        {
            if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
            {
                bool add = true;
                auto banned = STDHelpers::SplitString(CINI::FAData().GetString(pKey.second, "BannedTheater", ""));
                if (banned.size() > 0)
                    for (auto& ban : banned)
                        if (ban == CINI::CurrentDocument().GetString("Map", "Theater"))
                            add = false;
                if (add)
                {
                    InsertingObjectID = CINI::FAData().GetString(pKey.second, "0");
                    this->InsertString(CINI::FAData().GetString(pKey.second, "Name", "MISSING"), Const_Smudge + index, hRandomSmudge);
                    InsertingObjectID = "";
                }
                index++;
            }
        }
    }


    HTREEITEM hOther = this->InsertTranslatedString("OthObList", -1, hSmudge);

    auto&& smudges = Variables::RulesMap.ParseIndicies("SmudgeTypes", true);
    for (size_t i = 0, sz = smudges.size(); i < sz; ++i)
    {
        if (IgnoreSet.find(smudges[i]) == IgnoreSet.end())
        {
            FA2sp::Buffer = QueryUIName(smudges[i]);
            if (FA2sp::Buffer != smudges[i])
                FA2sp::Buffer += " (" + smudges[i] + ")";
            bool bNotOther = false;
            InsertingObjectID = smudges[i];
            for (const auto& node : nodes)
            {
                if (smudges[i].Find(node.second.c_str()) >= 0)
                {
                    this->InsertString(FA2sp::Buffer, Const_Smudge + i, node.first);
                    bNotOther = true;
                }
            }
            if (!bNotOther)
                this->InsertString(FA2sp::Buffer, Const_Smudge + i, hOther);
            InsertingObjectID = "";
        }
    }

}

void CViewObjectsExt::Redraw_Overlay()
{
    HTREEITEM& hOverlay = ExtNodes[Root_Overlay];
    if (hOverlay == NULL)   return;

    auto& rules = CINI::Rules();

    HTREEITEM hTemp;
    hTemp = this->InsertTranslatedString("DelOvrlObList", -1, hOverlay);
    this->InsertTranslatedString("DelOvrl0ObList", 60100, hTemp);
    this->InsertTranslatedString("DelOvrl1ObList", 60101, hTemp);
    this->InsertTranslatedString("DelOvrl2ObList", 60102, hTemp);
    this->InsertTranslatedString("DelOvrl3ObList", 60103, hTemp);

    hTemp = this->InsertTranslatedString("GrTibObList", -1, hOverlay);
    InsertingOverlay = 112;
    InsertingOverlayData = 11;
    this->InsertTranslatedString("DrawTibObList", 60210, hTemp);
    InsertingOverlay = 30;
    this->InsertTranslatedString("DrawTib2ObList", 60310, hTemp);
    InsertingOverlay = -1;

    this->InsertTranslatedString("AddOreObList", Const_Overlay + AddOre, hTemp);
    this->InsertTranslatedString("ReduceOreObList", Const_Overlay + ReduceOre, hTemp);

    hTemp = this->InsertTranslatedString("BridgesObList", -1, hOverlay);

    InsertingOverlay = 24;
    InsertingOverlayData = 0;
    this->InsertTranslatedString("BigBridgeObList", 60500, hTemp);
    InsertingOverlay = 74;
    InsertingOverlayData = 1;
    this->InsertTranslatedString("SmallBridgeObList", 60501, hTemp);
    InsertingOverlay = 237;
    InsertingOverlayData = 0;
    this->InsertTranslatedString("BigTrackBridgeObList", 60502, hTemp);
    InsertingOverlay = 205;
    InsertingOverlayData = 1;
    this->InsertTranslatedString("SmallConcreteBridgeObList", 60503, hTemp);
    InsertingOverlay = -1;

    // Walls
    HTREEITEM hWalls = this->InsertTranslatedString("WallsObList", -1, hOverlay);

    hTemp = this->InsertTranslatedString("AllObList", -1, hOverlay);

    this->InsertTranslatedString("OvrlManuallyObList", 60001, hOverlay);
    this->InsertTranslatedString("OvrlDataManuallyObList", 60002, hOverlay);

    if (!rules.SectionExists("OverlayTypes"))
        return;

    // a rough support for tracks

    InsertingOverlay = 39;
    InsertingOverlayData = 0;
    this->InsertTranslatedString("Tracks", Const_Overlay + 39, hOverlay);
    InsertingOverlay = -1;

    const auto& overlays = Variables::RulesMap.ParseIndicies("OverlayTypes", true);
    int indexWall = Wall;
    CViewObjectsExt::WallDamageStages.clear();
    for (int i = 0, sz = (ExtConfigs::ExtOverlays || CMapDataExt::NewINIFormat >= 5) ?
        overlays.size() : std::min((UINT)255, overlays.size()); i < sz; ++i)
    {
        const auto& value = overlays[i];
        FString buffer;
        buffer = QueryUIName(value);
        if (buffer != value)
            buffer += " (" + value + ")";
        FString id;
        id.Format("%03d %s", i, buffer);
        if (rules.GetBool(value, "Wall"))
        {
            int damageLevel = CINI::Art().GetInteger(value, "DamageLevels", 1);
            CViewObjectsExt::WallDamageStages[i] = damageLevel;
            InsertingOverlay = i;
            InsertingOverlayData = 5;
            auto thisWall = this->InsertString(
                QueryUIName(value),
                Const_Overlay + i * 5 + indexWall,
                hWalls
            );

            for (int s = 1; s < damageLevel + 1; s++)
            {
                FString damage;
                damage.Format("WallDamageLevelDes%d", s);
                this->InsertString(
                    QueryUIName(value) + " " + Translations::TranslateOrDefault(damage, damage),
                    Const_Overlay + i * 5 + s + indexWall,
                    thisWall
                );
                InsertingOverlayData += 16;
            }
            InsertingOverlay = -1;
            if (damageLevel > 1)
            {
                this->InsertString(
                    QueryUIName(value) + " " + Translations::TranslateOrDefault("WallDamageLevelDes4", "Random"),
                    Const_Overlay + i * 5 + 4 + indexWall,
                    thisWall);
            }
        }

        if (IgnoreSet.find(value) == IgnoreSet.end())
        {
            InsertingOverlay = i;
            if (CMapDataExt::IsOre((byte)i))
                InsertingOverlayData = 11;
            else
                InsertingOverlayData = 0;
            this->InsertString(id, Const_Overlay + i, hTemp);
        }
        InsertingOverlay = -1;
    }


    HTREEITEM hTemp2 = this->InsertTranslatedString("PlaceRandomOverlayList", -1, hOverlay);
    if (auto pSection = CINI::FAData().GetSection("PlaceRandomOverlayList"))
    {
        int index = RandomRock;
        for (const auto& pKey : pSection->GetEntities())
        {
            if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
            {
                bool add = true;
                auto banned = STDHelpers::SplitString(CINI::FAData().GetString(pKey.second, "BannedTheater", ""));
                if (banned.size() > 0)
                    for (auto& ban : banned)
                        if (ban == CINI::CurrentDocument().GetString("Map", "Theater"))
                            add = false;
                if (add)
                {
                    InsertingOverlay = CINI::FAData().GetInteger(pKey.second, "0");
                    InsertingOverlayData = 0;
                    this->InsertString(CINI::FAData().GetString(pKey.second, "Name", "MISSING"), Const_Overlay + index, hTemp2);
                    InsertingOverlay = -1;
                }
                index++;
            }
        }
    }
}

void CViewObjectsExt::Redraw_Waypoint()
{
    HTREEITEM& hWaypoint = ExtNodes[Root_Waypoint];
    if (hWaypoint == NULL)   return;

    this->InsertTranslatedString("CreateWaypObList", 20, hWaypoint);
    this->InsertTranslatedString("DelWaypObList", 21, hWaypoint);
    this->InsertTranslatedString("CreateSpecWaypObList", 22, hWaypoint);
}

void CViewObjectsExt::Redraw_Celltag()
{
    HTREEITEM& hCellTag = ExtNodes[Root_Celltag];
    if (hCellTag == NULL)   return;

    this->InsertTranslatedString("CreateCelltagObList", 36, hCellTag);
    this->InsertTranslatedString("DelCelltagObList", 37, hCellTag);
    this->InsertTranslatedString("CelltagPropObList", 38, hCellTag);
}

void CViewObjectsExt::Redraw_Basenode()
{
    HTREEITEM& hBasenode = ExtNodes[Root_Basenode];
    if (hBasenode == NULL)   return;

    this->InsertTranslatedString("CreateNodeNoDelObList", 40, hBasenode);
    this->InsertTranslatedString("CreateNodeDelObList", 41, hBasenode);
    this->InsertTranslatedString("DelNodeObList", 42, hBasenode);

    this->InsertTranslatedString("NodeMoveUP", Const_BaseNode + MoveUp, hBasenode);
    this->InsertTranslatedString("NodeMoveDown", Const_BaseNode + MoveDown, hBasenode);
    //this->InsertTranslatedString("NodeMove", Const_BaseNode + Move, hBasenode);

}

void CViewObjectsExt::Redraw_Tunnel()
{
    HTREEITEM& hTunnel = ExtNodes[Root_Tunnel];
    if (hTunnel == NULL)   return;

    if (CINI::FAData->GetBool("Debug", "AllowTunnels"))
    {
        this->InsertTranslatedString("NewTunnelObList", 50, hTunnel);
        this->InsertTranslatedString("NewUnidirectionalTunnelObList", 52, hTunnel);
        this->InsertTranslatedString("DelTunnelObList", 51, hTunnel);
    }
}

void CViewObjectsExt::Redraw_PlayerLocation()
{
    HTREEITEM& hPlayerLocation = ExtNodes[Root_PlayerLocation];
    if (hPlayerLocation == NULL)   return;

    this->InsertTranslatedString("StartpointsDelete", 21, hPlayerLocation);

    if (CMapData::Instance->IsMultiOnly())
    {
        for (int i = 0; i < 8; ++i)
        {
            FA2sp::Buffer.Format("Player %d", i);
            this->InsertString(FA2sp::Buffer, 23 + i, hPlayerLocation);
        }
    }
}

void CViewObjectsExt::Redraw_PropertyBrush()
{
    HTREEITEM& hPropertyBrush = ExtNodes[Root_PropertyBrush];
    if (hPropertyBrush == NULL)    return;

    this->InsertTranslatedString("PropertyBrushBuilding", Const_PropertyBrush + Set_Building, hPropertyBrush);
    this->InsertTranslatedString("PropertyBrushInfantry", Const_PropertyBrush + Set_Infantry, hPropertyBrush);
    this->InsertTranslatedString("PropertyBrushVehicle", Const_PropertyBrush + Set_Vehicle, hPropertyBrush);
    this->InsertTranslatedString("PropertyBrushAircraft", Const_PropertyBrush + Set_Aircraft, hPropertyBrush);
}

void CViewObjectsExt::Redraw_InfantrySubCell()
{
    HTREEITEM& hInfantrySubCell = ExtNodes[Root_InfantrySubCell];
    if (hInfantrySubCell == NULL)    return;
    this->InsertTranslatedString("InfantrySubCell3_1_2", Const_InfantrySubCell + i3_1_2, hInfantrySubCell);
    this->InsertTranslatedString("InfantrySubCell3_4_2", Const_InfantrySubCell + i3_4_2, hInfantrySubCell);
    this->InsertTranslatedString("InfantrySubCellChangeOrder", Const_InfantrySubCell + changeOrder, hInfantrySubCell);
}

void CViewObjectsExt::Redraw_Annotation()
{
    HTREEITEM& hAnnotation = ExtNodes[Root_Annotation];
    if (hAnnotation == NULL)    return;
    this->InsertTranslatedString("AnnotationObListAddChange", Const_Annotation + AnnotationsAdd, hAnnotation);
    this->InsertTranslatedString("AnnotationObListDelete", Const_Annotation + AnnotationsRemove, hAnnotation);
}

void CViewObjectsExt::Redraw_ViewObjectInfo()
{
    HTREEITEM& hView = ExtNodes[Root_View];
    if (hView == NULL)    return;

    HTREEITEM hObjects = this->InsertTranslatedString("ViewObjectInfo", Const_ViewObjectInfo + ObjectTerrainType::Object, hView);

    this->InsertTranslatedString("ViewInfantryInfo", Const_ViewObjectInfo + ObjectTerrainType::Infantry, hObjects);
    this->InsertTranslatedString("ViewVehicleInfo", Const_ViewObjectInfo + ObjectTerrainType::Vehicle, hObjects);
    this->InsertTranslatedString("ViewAircrafInfo", Const_ViewObjectInfo + ObjectTerrainType::Aircraft, hObjects);
    this->InsertTranslatedString("ViewBuildingInfo", Const_ViewObjectInfo + ObjectTerrainType::Building, hObjects);
    this->InsertTranslatedString("ViewBaseNodeInfo", Const_ViewObjectInfo + ObjectTerrainType::BaseNode, hObjects);

    HTREEITEM hAllTerrain = this->InsertTranslatedString("ViewAllTerrainInfo", Const_ViewObjectInfo + ObjectTerrainType::AllTerrain, hView);

    this->InsertTranslatedString("ViewTileInfo", Const_ViewObjectInfo + ObjectTerrainType::Tile, hAllTerrain);
    this->InsertTranslatedString("ViewOverlayInfo", Const_ViewObjectInfo + ObjectTerrainType::Overlay, hAllTerrain);
    this->InsertTranslatedString("ViewTerrainInfo", Const_ViewObjectInfo + ObjectTerrainType::Terrain, hAllTerrain);
    this->InsertTranslatedString("ViewSmudgeInfo", Const_ViewObjectInfo + ObjectTerrainType::Smudge, hAllTerrain);
    
    this->InsertTranslatedString("ViewWaypointInfo", Const_ViewObjectInfo + ObjectTerrainType::Waypoints, hView);
    this->InsertTranslatedString("ViewCelltagInfo", Const_ViewObjectInfo + ObjectTerrainType::Celltag, hView);
    this->InsertTranslatedString("ViewHouseInfo", Const_ViewObjectInfo + ObjectTerrainType::House, hView);

    HTREEITEM hRange = this->InsertTranslatedString("ViewRangeInfo", Const_ViewObjectInfo + AllRange, hView);

    this->InsertTranslatedString("ViewWeaponRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::WeaponRange, hRange);
    this->InsertTranslatedString("ViewSecondaryWeaponRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::SecondaryWeaponRange, hRange);
    this->InsertTranslatedString("ViewDeathWeaponRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::DeathWeaponRange, hRange);
    this->InsertTranslatedString("ViewGuardRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::GuardRange, hRange);
    this->InsertTranslatedString("ViewSightRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::SightRange, hRange);
    this->InsertTranslatedString("ViewGapRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::GapRange, hRange);
    this->InsertTranslatedString("ViewSensorsRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::SensorsRange, hRange);
    this->InsertTranslatedString("ViewCloakRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::CloakRange, hRange);
    this->InsertTranslatedString("ViewPsychicRangeInfo", Const_ViewObjectInfo + ObjectTerrainType::PsychicRange, hRange);

}

void CViewObjectsExt::Redraw_MultiSelection()
{
    HTREEITEM& hMultiSelection = ExtNodes[Root_MultiSelection];
    if (hMultiSelection == NULL)    return;

    this->InsertTranslatedString("MultiSelectionAdd", Const_MultiSelection + Add, hMultiSelection);
    this->InsertTranslatedString("MultiSelectionDelete", Const_MultiSelection + Delete, hMultiSelection);
    this->InsertTranslatedString("MultiSelectionBatchAdd", Const_MultiSelection + batchAdd, hMultiSelection);
    this->InsertTranslatedString("MultiSelectionBatchDelete", Const_MultiSelection + batchDelete, hMultiSelection);
    this->InsertTranslatedString("MultiSelectionSquareBatchAdd", Const_MultiSelection + squareBatchAdd, hMultiSelection);
    this->InsertTranslatedString("MultiSelectionSquareBatchDelete", Const_MultiSelection + squareBatchDelete, hMultiSelection);

    this->InsertTranslatedString("MultiSelectionCustomAdd", Const_MultiSelection + ConnectedAdd, hMultiSelection);
    this->InsertTranslatedString("MultiSelectionCustomDelete", Const_MultiSelection + ConnectedDelete, hMultiSelection);

    this->InsertTranslatedString("MultiSelectionHide", Const_MultiSelection + ReplaceHide, hMultiSelection);
    this->InsertTranslatedString("MultiSelectionAllDelete", Const_MultiSelection + AllDelete, hMultiSelection);

}

bool CViewObjectsExt::DoPropertyBrush_Building()
{
    if (this->BuildingBrushDlg.get() == nullptr)
        this->BuildingBrushDlg = std::make_unique<CPropertyBuilding>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->BuildingBrushBools)
        v = false;

    return this->BuildingBrushDlg->ppmfc::CDialog::DoModal() == IDOK;
}

bool CViewObjectsExt::DoPropertyBrush_Aircraft()
{
    if (this->AircraftBrushDlg.get() == nullptr)
        this->AircraftBrushDlg = std::make_unique<CPropertyAircraft>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->AircraftBrushBools)
        v = false;

    return this->AircraftBrushDlg->ppmfc::CDialog::DoModal() == IDOK;
}

bool CViewObjectsExt::DoPropertyBrush_Vehicle()
{
    if (this->VehicleBrushDlg.get() == nullptr)
        this->VehicleBrushDlg = std::make_unique<CPropertyUnit>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->VehicleBrushBools)
        v = false;

    return this->VehicleBrushDlg->ppmfc::CDialog::DoModal() == IDOK;
}

bool CViewObjectsExt::DoPropertyBrush_Infantry()
{
    if (this->InfantryBrushDlg.get() == nullptr)
        this->InfantryBrushDlg = std::make_unique<CPropertyInfantry>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->InfantryBrushBools)
        v = false;

    return this->InfantryBrushDlg->ppmfc::CDialog::DoModal() == IDOK;
}

void CViewObjectsExt::BatchAddMultiSelection(int X, int Y, bool add)
{
    MultiSelection::IsSquareSelecting = false;
    if (MultiSelection::LastAddedCoord.X > -1)
    {
        int x1, x2, y1, y2;

        if (MultiSelection::LastAddedCoord.X < X)
        {
            x1 = MultiSelection::LastAddedCoord.X;
            x2 = X;
        }
        else
        {
            x1 = X;
            x2 = MultiSelection::LastAddedCoord.X;
        }
        if (MultiSelection::LastAddedCoord.Y < Y)
        {
            y1 = MultiSelection::LastAddedCoord.Y;
            y2 = Y;
        }
        else
        {
            y1 = Y;
            y2 = MultiSelection::LastAddedCoord.Y;
        }

        for (int i = x1; i <= x2; i++)
        {
            for (int j = y1; j <= y2; j++)
            {
                if (add)
                {
                    MultiSelection::AddCoord(i, j);
                }
                else
                {
                    MultiSelection::RemoveCoord(i, j);
                }
            }
        }

        MultiSelection::LastAddedCoord.X = -1;
        MultiSelection::LastAddedCoord.Y = -1;
    }
    else
        MultiSelection::LastAddedCoord = { X,Y };
        
    CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CViewObjectsExt::SquareBatchAddMultiSelection(int X, int Y, bool add)
{
    MultiSelection::IsSquareSelecting = true;
    if (MultiSelection::LastAddedCoord.X > -1)
    {
        int& x1 = X;
        int& x2 = MultiSelection::LastAddedCoord.X;
        int& y1 = Y;
        int& y2 = MultiSelection::LastAddedCoord.Y;

        int top, bottom, left, right;
        top = x1 + y1;
        bottom = x2 + y2;
        left = y1 - x1;
        right = y2 - x2;
        if (top > bottom)
        {
            int tmp = top;
            top = bottom;
            bottom = tmp;
        }
        if (left > right)
        {
            int tmp = left;
            left = right;
            right = tmp;
        }
        auto IsCoordInSelect = [&](int X, int Y)
            {
                return
                    X + Y >= top &&
                    X + Y <= bottom &&
                    Y - X >= left &&
                    Y - X <= right;
            };

        for (int i = 0; i <= CMapData::Instance->MapWidthPlusHeight; i++)
        {
            for (int j = 0; j <= CMapData::Instance->MapWidthPlusHeight; j++)
            {
                if (IsCoordInSelect(i, j))
                {
                    if (add)
                    {
                        MultiSelection::AddCoord(i, j);
                    }
                    else
                    {
                        MultiSelection::RemoveCoord(i, j);
                    }
                }
            }
        }

        MultiSelection::LastAddedCoord.X = -1;
        MultiSelection::LastAddedCoord.Y = -1;
    }
    else
        MultiSelection::LastAddedCoord = { X,Y };

    CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CViewObjectsExt::ModifyOre(int X, int Y)
{
    const int ORE_COUNT = 12;
    auto pExt = CMapDataExt::GetExtension();
    int pos = pExt->GetCoordIndex(X, Y);
    auto ovr = pExt->GetOverlayAt(pos);
    int ovrd = pExt->GetOverlayDataAt(pos);

    auto getValidOreData = [ORE_COUNT](int data)
        {
            if (data < 0)
                data = -1;
            if (data >= ORE_COUNT)
                data = ORE_COUNT - 1;
            return data;
        };
    auto setOreDataAt = [ovr, ovrd, pExt, getValidOreData](int x, int y, int data)
        {            
            data = getValidOreData(data);
            int moneyDelta = 0;
            int olyPos = y + x * 512;
            int pos = pExt->GetCoordIndex(x, y);
            if (data >= 0)
            {
                moneyDelta = pExt->GetOreValue(ovr, data) - pExt->GetOreValue(ovr, ovrd);
                pExt->OverlayData[olyPos] = data;
                pExt->CellDatas[pos].OverlayData = data;
            }
            else
            {
                moneyDelta = -pExt->GetOreValue(ovr, ovrd);
                pExt->Overlay[olyPos] = 0xFF;
                pExt->NewOverlay[olyPos] = 0xFFFF;
                pExt->OverlayData[olyPos] = 0;
                pExt->CellDatas[pos].Overlay = 0xFF;
                pExt->CellDataExts[pos].NewOverlay = 0xFFFF;
                pExt->CellDatas[pos].OverlayData = 0;
            }

            pExt->MoneyCount += moneyDelta;
        };
    if (CMapDataExt::IsOre(ovr))
    {
        if (!CIsoViewExt::HistoryRecord_IsHoldingLButton)
        {
            CIsoViewExt::HistoryRecord_IsHoldingLButton = true;
            pExt->SaveUndoRedoData(true, 0, 0, 0, 0);
        }
        if (CIsoView::CurrentCommand->Type == 0)
        {
            setOreDataAt(X, Y, ovrd + 1);
        }
        else if (CIsoView::CurrentCommand->Type == 1)
        {
            setOreDataAt(X, Y, ovrd - 1);
        }
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void CViewObjectsExt::AddAnnotation(int X, int Y)
{
    FString key;
    key.Format("%d", X * 1000 + Y);
    CAnnotationDlg dlg;

    bool folded = false;
    if (CINI::CurrentDocument->KeyExists("Annotations", key))
    {
        auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString("Annotations", key), 6);
        dlg.m_Text = atoms[5];
        for (int i = 6; i < atoms.size() - 1; i++)
        {
             dlg.m_Text += ",";
             dlg.m_Text += atoms[i];
        }
        dlg.m_FontSize = atoms[0];
        dlg.m_Bold = STDHelpers::IsTrue(atoms[1]);
        folded = STDHelpers::IsTrue(atoms[2]);
        dlg.m_TextColor = atoms[3];
        dlg.m_BgColor = atoms[4];
    }

    IsOpeningAnnotationDlg = true;
    dlg.DoModal();
    IsOpeningAnnotationDlg = false;
    dlg.m_Text.Replace("\n", "\\n");
    dlg.m_Text.Replace("\r", "\\n");
    if (STDHelpers::IsNullOrWhitespaceOrReturn(dlg.m_Text))
        return;

    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Annotation);
    int size = std::min(100, atoi(dlg.m_FontSize));
    size = std::max(10, size);
    auto textColor = STDHelpers::HexStringToColorRefRGB(dlg.m_TextColor);
    auto bgColor = STDHelpers::HexStringToColorRefRGB(dlg.m_BgColor);
    bool bold = dlg.m_Bold == TRUE;

    FString value;
    value.Format("%d,%s,%s,%s,%s,%s,END", size, bold ? "yes" : "no", folded ? "yes" : "no",
        STDHelpers::ColorRefRGBToHexString(textColor), STDHelpers::ColorRefRGBToHexString(bgColor),
        dlg.m_Text);

    CINI::CurrentDocument->WriteString("Annotations", key, value);
    auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
    cellExt.HasAnnotation = true;

    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CViewObjectsExt::RemoveAnnotation(int X, int Y)
{
    FString key;
    key.Format("%d", X * 1000 + Y);

    auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
    cellExt.HasAnnotation = false;
    if (CINI::CurrentDocument->KeyExists("Annotations", key))
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Annotation);
        CINI::CurrentDocument->DeleteKey("Annotations", key);
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }

    return;
}

void CViewObjectsExt::DeleteTube(int X, int Y)
{
    bool once = false;
    for (int i = 0; i < CMapDataExt::Tubes.size(); ++i)
    {
        auto& tube = CMapDataExt::Tubes[i];
        for (auto& coord : tube.PathCoords)
        {
            if (coord.X == X && coord.Y == Y)
            {
                if (!once)
                {
                    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Tunnel);
                    once = true;
                }
                CINI::CurrentDocument->DeleteKey("Tubes", tube.key);
                break;
            }
        }
    }
    CMapData::Instance->UpdateFieldTubeData(false);
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CViewObjectsExt::MoveBaseNodeOrder(int X, int Y)
{
    auto& ini = CMapData::Instance->INI;
    auto cell = CMapData::Instance->GetCellAt(X, Y);
    int exchangeIndex = -1;
    int targetIndex = -1;

    if (cell->BaseNode.BasenodeID < 0)
        return;

    char key[10];
    sprintf(key, "%03d", cell->BaseNode.BasenodeID);
    auto&& atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString(cell->BaseNode.House, key), 2);
    const auto& ID = atoms[0];
    targetIndex = cell->BaseNode.BasenodeID;

    if (CIsoView::CurrentCommand->Type == MoveUp)
    {
        exchangeIndex = targetIndex - 1;
        char key[10];
        sprintf(key, "%03d", exchangeIndex);
        auto value = ini.GetString(cell->BaseNode.House, key, "");
        if (value == "")
            return;
    }
    if (CIsoView::CurrentCommand->Type == MoveDown)
    {
        exchangeIndex = targetIndex + 1;
        char key[10];
        sprintf(key, "%03d", exchangeIndex);
        auto value = ini.GetString(cell->BaseNode.House, key, "");
        if (value == "")
            return;
    }
    char targetBuffer[10];
    char exchangeBuffer[10];
    sprintf(targetBuffer, "%03d", targetIndex);
    sprintf(exchangeBuffer, "%03d", exchangeIndex);

    auto targetValue = ini.GetString(cell->BaseNode.House, targetBuffer, "");
    auto exchangeValue = ini.GetString(cell->BaseNode.House, exchangeBuffer, "");

    ini.WriteString(cell->BaseNode.House, targetBuffer, exchangeValue);
    ini.WriteString(cell->BaseNode.House, exchangeBuffer, targetValue);
    CMapData::Instance->UpdateFieldBasenodeData(false);
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}


void CViewObjectsExt::MoveBaseNode(int X, int Y)
{
    if (CIsoView::CurrentCommand->Type != Move)
        return;

    auto& ini = CMapData::Instance->INI;
    auto cell = CMapData::Instance->GetCellAt(X, Y);

    if (MoveBaseNode_SelectedObj.X < 0)
    {
        if (cell->BaseNode.BasenodeID < 0)
            return;

        char key[10];
        sprintf(key, "%03d", cell->BaseNode.BasenodeID);
        auto&& atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString(cell->BaseNode.House, key), 2);
        const auto& ID = atoms[0];
        int bnX = atoi(atoms[2]);
        int bnY = atoi(atoms[1]);

        MoveBaseNode_SelectedObj.House = cell->BaseNode.House;
        MoveBaseNode_SelectedObj.ID = ID;
        MoveBaseNode_SelectedObj.Key = key;
        MoveBaseNode_SelectedObj.X = bnX;
        MoveBaseNode_SelectedObj.Y = bnY;
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return;
    }
    else
    {
        FString value;
        value.Format("%s,%d,%d", MoveBaseNode_SelectedObj.ID, Y, X);
        ini.WriteString(MoveBaseNode_SelectedObj.House, MoveBaseNode_SelectedObj.Key, value);
        MoveBaseNode_SelectedObj.House = "";
        MoveBaseNode_SelectedObj.ID = "";
        MoveBaseNode_SelectedObj.Key = "";
        MoveBaseNode_SelectedObj.X = -1;
        MoveBaseNode_SelectedObj.Y = -1;
        CMapData::Instance->UpdateFieldBasenodeData(false);
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return;
    }
}


void CViewObjectsExt::ApplyInfantrySubCell(int X, int Y)
{
    int nIndex = CMapData::Instance->GetCoordIndex(X, Y);
    auto& CellData = CMapData::Instance->CellDatas[nIndex];

    CInfantryData data0;
    CInfantryData data1;
    CInfantryData data2;
    std::vector<CInfantryData> datas;
    std::vector<int> order;
    data0.SubCell = "-1";
    data1.SubCell = "-1";
    data2.SubCell = "-1";
    int infCount = 0;
    if (CellData.Infantry[0] != -1)
    {
        CMapData::Instance->GetInfantryData(CellData.Infantry[0], data0);
        CMapData::Instance->DeleteInfantryData(CellData.Infantry[0]);
        infCount++;
    }
    if (CellData.Infantry[1] != -1)
    {
        CMapData::Instance->GetInfantryData(CellData.Infantry[1], data1);
        CMapData::Instance->DeleteInfantryData(CellData.Infantry[1]);
        infCount++;
    }
    if (CellData.Infantry[2] != -1)
    {
        CMapData::Instance->GetInfantryData(CellData.Infantry[2], data2);
        CMapData::Instance->DeleteInfantryData(CellData.Infantry[2]);
        infCount++;
    }
    datas.push_back(data0);
    datas.push_back(data1);
    datas.push_back(data2);

    auto ApplyValue = [&]()
        {
            int i = 0;
            for (int j = 0; j < 3; j++)
            {
                if (datas[j].SubCell == "-1")
                    continue;
                for (int k = 0; k < 3; k++)
                {
                    int subcell = atoi(datas[j].SubCell);
                    if (subcell == 0)
                        subcell = 1;
                    if (CIsoView::CurrentCommand->Type == i3_1_2)
                        if (subcell == 4)
                            subcell = 1;
                    if (CIsoView::CurrentCommand->Type == i3_4_2)
                        if (subcell == 1)
                            subcell = 4;
                    if (order[k] == subcell)
                    {
                        datas[j].SubCell = std::to_string(order[k + 1]).c_str();
                        CMapData::Instance->SetInfantryData(&datas[j], nullptr, nullptr, 0, -1);
                        i++;
                        break;
                    }
                }
            }
        };

    if (CIsoView::CurrentCommand->Type == i3_1_2)
    {
        order.push_back(3);
        order.push_back(1);
        order.push_back(2);
        order.push_back(3);
        ApplyValue();
    }

    if (CIsoView::CurrentCommand->Type == i3_4_2)
    {
        order.push_back(3);
        order.push_back(4);
        order.push_back(2);
        order.push_back(3);
        ApplyValue();
    }
    if (CIsoView::CurrentCommand->Type == changeOrder)
    {
        int count = 0;
        for (int j = 0; j < 3; j++)
        {
            if (datas[j].SubCell != "-1")
                count++;
        }
        if (count == 1)
        {
            for (int j = 0; j < 3; j++)
            {
                if (datas[j].SubCell != "-1")
                    CMapData::Instance->SetInfantryData(&datas[j], nullptr, nullptr, 0, -1);
            }
        }
        else if (count == 2)
        {
            if (datas[0].SubCell == "-1")
            {
                auto& temp = datas[1].SubCell;
                datas[1].SubCell = datas[2].SubCell;
                datas[2].SubCell = temp;

                CMapData::Instance->SetInfantryData(&datas[1], nullptr, nullptr, 0, -1);
                CMapData::Instance->SetInfantryData(&datas[2], nullptr, nullptr, 0, -1);
            }
            else if (datas[1].SubCell == "-1")
            {
                auto& temp = datas[0].SubCell;
                datas[0].SubCell = datas[2].SubCell;
                datas[2].SubCell = temp;

                CMapData::Instance->SetInfantryData(&datas[0], nullptr, nullptr, 0, -1);
                CMapData::Instance->SetInfantryData(&datas[2], nullptr, nullptr, 0, -1);
            }
            else if (datas[2].SubCell == "-1")
            {
                auto& temp = datas[0].SubCell;
                datas[0].SubCell = datas[1].SubCell;
                datas[1].SubCell = temp;

                CMapData::Instance->SetInfantryData(&datas[0], nullptr, nullptr, 0, -1);
                CMapData::Instance->SetInfantryData(&datas[1], nullptr, nullptr, 0, -1);
            }
        }
        else if (count == 3)
        {
            auto& temp = datas[1].SubCell;
            datas[1].SubCell = datas[2].SubCell;
            datas[2].SubCell = temp;

            CMapData::Instance->SetInfantryData(&datas[0], nullptr, nullptr, 0, -1);
            CMapData::Instance->SetInfantryData(&datas[1], nullptr, nullptr, 0, -1);
            CMapData::Instance->SetInfantryData(&datas[2], nullptr, nullptr, 0, -1);
        }

    }


    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void CViewObjectsExt::ApplyChangeOwner(int X, int Y)
{
    auto makeOrAppendRecord = [](int recordType)
        {
            if (!ObjectRecord::ObjectRecord_HoldingPtr)
                ObjectRecord::ObjectRecord_HoldingPtr = CMapDataExt::MakeObjectRecord(recordType, true);
            else
                ObjectRecord::ObjectRecord_HoldingPtr->appendRecord(recordType);
        };

    auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
    auto pMap = CMapDataExt::GetExtension();

    // infantry subcell case
    if (ExtConfigs::InfantrySubCell_Edit &&
        pIsoView->BrushSizeX == 1 && pIsoView->BrushSizeY == 1)
    {
        int idx = CIsoViewExt::GetSelectedSubcellInfantryIdx(X, Y);
        if (idx != -1)
        {
            makeOrAppendRecord(ObjectRecord::RecordType::Infantry);
            CInfantryData infantry;
            CMapData::Instance->GetInfantryData(idx, infantry);
            CMapData::Instance->DeleteInfantryData(idx);
            infantry.House = CIsoView::CurrentCommand->ObjectID;
            CMapData::Instance->SetInfantryData(&infantry, nullptr, nullptr, 0, -1);
            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
            return;
        }
    }

    bool changed = false;
    for (int gx = X - pIsoView->BrushSizeX / 2; gx <= X + pIsoView->BrushSizeX / 2; gx++)
    {
        for (int gy = Y - pIsoView->BrushSizeY / 2; gy <= Y + pIsoView->BrushSizeY / 2; gy++)
        {
            if (!CMapDataExt::IsCoordInFullMap(gx, gy))
                continue;

            int nIndex = CMapData::Instance->GetCoordIndex(gx, gy);
            const auto& CellData = CMapData::Instance->CellDatas[nIndex];

            if (CellData.Structure != -1)
            {
                makeOrAppendRecord(ObjectRecord::RecordType::Building);
                CBuildingData structure;
                pMap->GetBuildingData(CellData.Structure, structure);
                pMap->DeleteBuildingData(CellData.Structure);
                structure.House = CIsoView::CurrentCommand->ObjectID;
                pMap->SetBuildingData(&structure, nullptr, nullptr, 0, "");
                changed = true;
            }

            for (int z = 0; z < 3; ++z)
            {
                if (CellData.Infantry[z] != -1)
                {
                    makeOrAppendRecord(ObjectRecord::RecordType::Infantry);
                    CInfantryData infantry;
                    CMapData::Instance->GetInfantryData(CellData.Infantry[z], infantry);
                    CMapData::Instance->DeleteInfantryData(CellData.Infantry[z]);
                    infantry.House = CIsoView::CurrentCommand->ObjectID;
                    CMapData::Instance->SetInfantryData(&infantry, nullptr, nullptr, 0, -1);
                    changed = true;
                }
            }

            if (CellData.Unit != -1)
            {
                makeOrAppendRecord(ObjectRecord::RecordType::Unit);
                CUnitData unit;
                pMap->GetUnitData(CellData.Unit, unit);
                pMap->DeleteUnitData(CellData.Unit);
                unit.House = CIsoView::CurrentCommand->ObjectID;
                pMap->SetUnitData(&unit, nullptr, nullptr, 0, "");
                changed = true;
            }

            if (CellData.Aircraft != -1)
            {
                makeOrAppendRecord(ObjectRecord::RecordType::Aircraft);
                CAircraftData air;
                pMap->GetAircraftData(CellData.Aircraft, air);
                pMap->DeleteAircraftData(CellData.Aircraft);
                air.House = CIsoView::CurrentCommand->ObjectID;
                pMap->SetAircraftData(&air, nullptr, nullptr, 0, "");
                changed = true;
            }
        }
    }
    if (changed)
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void CViewObjectsExt::ApplyDragFacing(int X, int Y)
{
    auto pIsoView = CIsoView::GetInstance();
    auto Map = &CMapData::Instance();
    auto& m_id = pIsoView->CurrentCellObjectIndex;
    auto& m_type = pIsoView->CurrentCellObjectType;
    MapCoord point = { X,Y };

    //order: inf unit air str
    if (m_type == 0)
    {
        ppmfc::CString oldFacing;
        CInfantryData infantry;
        Map->GetInfantryData(m_id, infantry);
        oldFacing = infantry.Facing;
        auto oldMapCoord = MapCoord{ atoi(infantry.X), atoi(infantry.Y) };
        infantry.Facing = CMapDataExt::GetFacing(oldMapCoord, point, infantry.Facing, ExtConfigs::ExtFacings_Drag ? 32 : 8);
        Map->InfantryDatas[m_id] = infantry;
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        infantry.Facing = oldFacing;
        Map->InfantryDatas[m_id] = infantry;
    }
    else if (m_type == 3)
    {
        auto key = Map->INI.GetKeyAt("Units", m_id);
        FString value = Map->INI.GetString("Units", key);
        FString oldValue = value;
        auto atoms = FString::SplitString(value, 13);
        auto oldMapCoord = MapCoord{ atoi(atoms[4]), atoi(atoms[3]) };
        value.SetParam(5, CMapDataExt::GetFacing(oldMapCoord, point, atoms[5], ExtConfigs::ExtFacings_Drag ? 32 : 8));
        Map->INI.WriteString("Units", key, value);
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        Map->INI.WriteString("Units", key, oldValue);
    }
    else if (m_type == 2)
    {
        auto key = Map->INI.GetKeyAt("Aircraft", m_id);
        FString value = Map->INI.GetString("Aircraft", key);
        FString oldValue = value;
        auto atoms = FString::SplitString(value, 11);
        auto oldMapCoord = MapCoord{ atoi(atoms[4]), atoi(atoms[3]) };
        value.SetParam(5, CMapDataExt::GetFacing(oldMapCoord, point, atoms[5], ExtConfigs::ExtFacings_Drag ? 32 : 8));
        Map->INI.WriteString("Aircraft", key, value);
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        Map->INI.WriteString("Aircraft", key, oldValue);
    }
    else if (m_type == 1)
    {
        int iniIndex = CMapDataExt::StructureIndexMap[m_id];
        auto& renderData = CMapDataExt::BuildingRenderDatasFix[iniIndex];
        auto oldMapCoord = MapCoord{ renderData.X, renderData.Y };
        short oldFacing = renderData.Facing;
        renderData.Facing = atoi(CMapDataExt::GetFacing(oldMapCoord, point, STDHelpers::IntToString(oldFacing), ExtConfigs::ExtFacings_Drag ? 32 : 8));
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        renderData.Facing = oldFacing;
    }
}

void CViewObjectsExt::ApplyPropertyBrush(int X, int Y)
{
    auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();

    // infantry subcell case
    if (ExtConfigs::InfantrySubCell_Edit &&
        CIsoView::CurrentCommand->Type == Set_Infantry &&
        pIsoView->BrushSizeX == 1 && pIsoView->BrushSizeY == 1)
    {
        int idx = CIsoViewExt::GetSelectedSubcellInfantryIdx(X, Y);
        if (idx != -1)
        {
            ApplyPropertyBrush_Infantry(idx);
            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        }
        return;
    }

    for (int gx = X - pIsoView->BrushSizeX / 2; gx <= X + pIsoView->BrushSizeX / 2; gx++)
    {
        for (int gy = Y - pIsoView->BrushSizeY / 2; gy <= Y + pIsoView->BrushSizeY / 2; gy++)
        {
            if (!CMapDataExt::IsCoordInFullMap(gx, gy))
                continue;

            int nIndex = CMapData::Instance->GetCoordIndex(gx, gy);
            const auto& CellData = CMapData::Instance->CellDatas[nIndex];

            if (CIsoView::CurrentCommand->Type == Set_Building)
            {
                if (CellData.Structure != -1)
                    ApplyPropertyBrush_Building(CellData.Structure);
            }
            else if (CIsoView::CurrentCommand->Type == Set_Infantry)
            {
                if (CellData.Infantry[0] != -1)
                    ApplyPropertyBrush_Infantry(CellData.Infantry[0]);
                if (CellData.Infantry[1] != -1)
                    ApplyPropertyBrush_Infantry(CellData.Infantry[1]);
                if (CellData.Infantry[2] != -1)
                    ApplyPropertyBrush_Infantry(CellData.Infantry[2]);
            }
            else if (CIsoView::CurrentCommand->Type == Set_Vehicle)
            {
                if (CellData.Unit != -1)
                    ApplyPropertyBrush_Vehicle(CellData.Unit);
            }
            else if (CIsoView::CurrentCommand->Type == Set_Aircraft)
            {
                if (CellData.Aircraft != -1)
                    ApplyPropertyBrush_Aircraft(CellData.Aircraft);
            }
        }
    }  
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void CViewObjectsExt::ApplyPropertyBrush_Building(int nIndex)
{
    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building, true);
    CBuildingData data;
    CMapData::Instance->GetBuildingData(nIndex, data);
    
    ApplyPropertyBrush_Building(data);
    CMapData::Instance->DeleteBuildingData(nIndex);
    CMapData::Instance->SetBuildingData(&data, nullptr, nullptr, 0, "");
}

void CViewObjectsExt::ApplyPropertyBrush_Infantry(int nIndex)
{
    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry, true);
    CInfantryData data;
    CMapData::Instance->GetInfantryData(nIndex, data);

    ApplyPropertyBrush_Infantry(data);

    CMapData::Instance->DeleteInfantryData(nIndex);
    CMapData::Instance->SetInfantryData(&data, nullptr, nullptr, 0, -1);
}

void CViewObjectsExt::ApplyPropertyBrush_Aircraft(int nIndex)
{
    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft, true);
    CAircraftData data;
    CMapData::Instance->GetAircraftData(nIndex, data);

    ApplyPropertyBrush_Aircraft(data);

    CMapData::Instance->DeleteAircraftData(nIndex);
    CMapData::Instance->SetAircraftData(&data, nullptr, nullptr, 0, "");
}

void CViewObjectsExt::ApplyPropertyBrush_Vehicle(int nIndex)
{
    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit, true);
    CUnitData data;
    CMapData::Instance->GetUnitData(nIndex, data);

    ApplyPropertyBrush_Vehicle(data);

    CMapData::Instance->DeleteUnitData(nIndex);
    CMapData::Instance->SetUnitData(&data, nullptr, nullptr, 0, "");
}

void CViewObjectsExt::ApplyPropertyBrush_Building(CBuildingData& data)
{
    if (!BuildingBrushDlg)
        return;

    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (BuildingBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, BuildingBrushDlg->CString_House, data.House);
    ApplyValue(1301, BuildingBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, BuildingBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1303, BuildingBrushDlg->CString_Sellable, data.AISellable);
    ApplyValue(1304, BuildingBrushDlg->CString_Rebuildable, data.AIRebuildable);
    ApplyValue(1305, BuildingBrushDlg->CString_EnergySupport, data.PoweredOn);
    ApplyValue(1306, BuildingBrushDlg->CString_UpgradeCount, data.Upgrades);
    ApplyValue(1307, BuildingBrushDlg->CString_Spotlight, data.SpotLight);
    ApplyValue(1308, BuildingBrushDlg->CString_Upgrade1, data.Upgrade1);
    ApplyValue(1309, BuildingBrushDlg->CString_Upgrade2, data.Upgrade2);
    ApplyValue(1310, BuildingBrushDlg->CString_Upgrade3, data.Upgrade3);
    ApplyValue(1311, BuildingBrushDlg->CString_AIRepairs, data.AIRepairable);
    ApplyValue(1312, BuildingBrushDlg->CString_ShowName, data.Nominal);
    ApplyValue(1313, BuildingBrushDlg->CString_Tag, data.Tag);
}

void CViewObjectsExt::ApplyPropertyBrush_Infantry(CInfantryData& data)
{
    if (!InfantryBrushDlg)
        return;

    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (InfantryBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, InfantryBrushDlg->CString_House, data.House);
    ApplyValue(1301, InfantryBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, InfantryBrushDlg->CString_State, data.Status);
    ApplyValue(1303, InfantryBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1304, InfantryBrushDlg->CString_VerteranStatus, data.VeterancyPercentage);
    ApplyValue(1305, InfantryBrushDlg->CString_Group, data.Group);
    ApplyValue(1306, InfantryBrushDlg->CString_OnBridge, data.IsAboveGround);
    ApplyValue(1307, InfantryBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType);
    ApplyValue(1308, InfantryBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType);
    ApplyValue(1309, InfantryBrushDlg->CString_Tag, data.Tag);
}

void CViewObjectsExt::ApplyPropertyBrush_Aircraft(CAircraftData& data)
{
    if (!AircraftBrushDlg)
        return;

    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (AircraftBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, AircraftBrushDlg->CString_House, data.House);
    ApplyValue(1301, AircraftBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, AircraftBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1303, AircraftBrushDlg->CString_Status, data.Status);
    ApplyValue(1304, AircraftBrushDlg->CString_VeteranLevel, data.VeterancyPercentage);
    ApplyValue(1305, AircraftBrushDlg->CString_Group, data.Group);
    ApplyValue(1306, AircraftBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType);
    ApplyValue(1307, AircraftBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType);
    ApplyValue(1308, AircraftBrushDlg->CString_Tag, data.Tag);
}

void CViewObjectsExt::ApplyPropertyBrush_Vehicle(CUnitData& data)
{
    if (!VehicleBrushDlg)
        return;

    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (VehicleBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, VehicleBrushDlg->CString_House, data.House);
    ApplyValue(1301, VehicleBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, VehicleBrushDlg->CString_State, data.Status);
    ApplyValue(1303, VehicleBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1304, VehicleBrushDlg->CString_VeteranLevel, data.VeterancyPercentage);
    ApplyValue(1305, VehicleBrushDlg->CString_Group, data.Group);
    ApplyValue(1306, VehicleBrushDlg->CString_OnBridge, data.IsAboveGround);
    ApplyValue(1307, VehicleBrushDlg->CString_FollowerID, data.FollowsIndex);
    ApplyValue(1308, VehicleBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType);
    ApplyValue(1309, VehicleBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType);
    ApplyValue(1310, VehicleBrushDlg->CString_Tag, data.Tag);
}

bool CViewObjectsExt::IsIgnored(const char* pItem)
{
    return IgnoreSet.find(pItem) != IgnoreSet.end();
}

int CViewObjectsExt::GuessType(const char* pRegName)
{
    if (ExtSets[Set_Building].find(pRegName) != ExtSets[Set_Building].end())
        return Set_Building;
    if (ExtSets[Set_Infantry].find(pRegName) != ExtSets[Set_Infantry].end())
        return Set_Infantry;
    if (ExtSets[Set_Vehicle].find(pRegName) != ExtSets[Set_Vehicle].end())
        return Set_Vehicle;
    if (ExtSets[Set_Aircraft].find(pRegName) != ExtSets[Set_Aircraft].end())
        return Set_Aircraft;
    return -1;
}

std::vector<int> CViewObjectsExt::GuessSide(const char* pRegName, int nType)
{
    auto&& knownIterator = KnownItem.find(pRegName);
    std::vector<int> result(9, -1);
    if (knownIterator != KnownItem.end())
    {
        for (int i = 0; i < 9; ++i) {
            result[i] = (knownIterator->second[i]);
        }
        return result;
    }
        
    switch (nType)
    {
    case -1:
    default:
        break;
    case Set_Building:
        result[0] = GuessBuildingSide(pRegName);
        break;
    case Set_Infantry:
        result[0] = GuessGenericSide(pRegName, Set_Infantry);
        break;
    case Set_Vehicle:
        result[0] = GuessGenericSide(pRegName, Set_Vehicle);
        break;
    case Set_Aircraft:
        result[0] = GuessGenericSide(pRegName, Set_Aircraft);
        break;
    }
    for (int i = 0; i < 9; ++i) {
        KnownItem[pRegName][i] = result[i];
    }
    return result;
}

int CViewObjectsExt::GuessBuildingSide(const char* pRegName)
{
    auto& rules = CINI::Rules();

    int planning;
    planning = rules.GetInteger(pRegName, "AIBasePlanningSide", -1);
    if (planning >= rules.GetKeyCount("Sides"))
        return -1;
    if (planning >= 0)
        return planning > ExtConfigs::ObjectBrowser_GuessMax ? -1 : planning;
    auto&& cons = STDHelpers::SplitString(rules.GetString("AI", "BuildConst"));
    int i;
    for (i = 0; i < cons.size(); ++i)
    {
        if (cons[i] == pRegName)
            return i > ExtConfigs::ObjectBrowser_GuessMax ? -1 : i;
    }
    if (i >= rules.GetKeyCount("Sides"))
        return -1;
    return GuessGenericSide(pRegName, Set_Building);
}

int CViewObjectsExt::GuessGenericSide(const char* pRegName, int nType)
{
    const auto& set = ExtSets[nType];

    if (set.find(pRegName) == set.end())
        return -1;

    switch (ExtConfigs::ObjectBrowser_GuessMode)
    {
    default:
    case 0:
    {
        for (auto& prep : STDHelpers::SplitString(Variables::RulesMap.GetString(pRegName, "Prerequisite")))
        {
            int guess = -1;
            for (auto& subprep : STDHelpers::SplitString(Variables::RulesMap.GetString("GenericPrerequisites", prep)))
            {
                if (subprep == pRegName) // Avoid build myself crash
                    return -1;
                guess = GuessSide(subprep, GuessType(subprep))[0];
                if (guess != -1)
                    return guess > ExtConfigs::ObjectBrowser_GuessMax ? -1 : guess;
            }
            if (prep == pRegName) // Avoid build myself crash
                return -1;
            guess = GuessSide(prep, GuessType(prep))[0];
            if (guess != -1)
                return guess > ExtConfigs::ObjectBrowser_GuessMax ? -1 : guess;
        }
        return -1;
    }
    case 1:
    {
        auto&& owners = STDHelpers::SplitString(Variables::RulesMap.GetString(pRegName, "Owner"));
        if (owners.size() <= 0)
            return -1;
        auto&& itr = Owners.find(owners[0]);
        if (itr == Owners.end())
            return -1;
        return itr->second > ExtConfigs::ObjectBrowser_GuessMax ? -1 : itr->second;
    }
    }
}

// CViewObjectsExt::OnSelectChanged
void CViewObjectsExt::OnExeTerminate()
{
    IgnoreSet.clear();
    ForceName.clear();
    for (auto& set : ExtSets)
        set.clear();
    KnownItem.clear();
    Owners.clear();
}
void CViewObjectsExt::InitializeOnUpdateEngine()
{
    CViewObjectsExt::PlacingRandomRock = -1;
    CViewObjectsExt::PlacingRandomSmudge = -1;
    CViewObjectsExt::PlacingRandomTerrain = -1;
    CViewObjectsExt::PlacingRandomInfantry = -1;
    CViewObjectsExt::PlacingRandomVehicle = -1;
    CViewObjectsExt::PlacingRandomStructure = -1;
    CViewObjectsExt::PlacingRandomAircraft = -1;
    CViewObjectsExt::PlacingWall = -1;
    CViewObjectsExt::PlacingRandomRandomFacing = false;
    CViewObjectsExt::NeedChangeTreeViewSelect = true;

    CViewObjectsExt::CliffConnectionCoord.X = -1;
    CViewObjectsExt::CliffConnectionCoord.Y = -1;
    CViewObjectsExt::CliffConnectionHeight = -1;
    CViewObjectsExt::LastCTTile = -1;
    CViewObjectsExt::CliffConnectionCoordRecords.clear();
    CViewObjectsExt::LastPlacedCT.Index = -1;
    CViewObjectsExt::LastSuccessfulIndex = -1;
    CViewObjectsExt::NextCTHeightOffset = 0;
    CViewObjectsExt::HeightChanged = false;
    CViewObjectsExt::LastPlacedCTRecords.clear();
    CViewObjectsExt::LastCTTileRecords.clear();
    CViewObjectsExt::LastHeightRecords.clear();
    CViewObjectsExt::LastPlacedCT.HeightAdjust = 0;
    CViewObjectsExt::CliffConnectionHeightAdjust = 0;
}

bool CViewObjectsExt::UpdateEngine(int nData)
{
    InitializeOnUpdateEngine();

    do
    {
        int nMorphable = nData - 67;
        if (nMorphable >= 0 && nMorphable < TheaterInfo::CurrentInfo.size())
        {
            int i;
            for (i = 0; i < *CTileTypeClass::InstanceCount; ++i)
                if ((*CTileTypeClass::Instance)[i].TileSet == TheaterInfo::CurrentInfo[nMorphable].Morphable)
                {
                    CIsoView::CurrentCommand->Param = 0;
                    CIsoView::CurrentCommand->Height = 0;
                    CIsoView::CurrentCommand->Type = i;
                    CIsoView::CurrentCommand->Command = FACurrentCommand::TileDraw;
                    CBrushSize::UpdateBrushSize(i);
                    return true;
                }
        }
        else if (nMorphable >= 0 && nMorphable < (TheaterInfo::CurrentInfo.size() + TheaterInfo::CurrentInfoNonMorphable.size()))
        {
            int i;
            for (i = 0; i < *CTileTypeClass::InstanceCount; ++i)
                if ((*CTileTypeClass::Instance)[i].TileSet == TheaterInfo::CurrentInfoNonMorphable[nMorphable- TheaterInfo::CurrentInfo.size()].Morphable)
                {
                    CIsoView::CurrentCommand->Param = 0;
                    CIsoView::CurrentCommand->Height = 0;
                    CIsoView::CurrentCommand->Type = i;
                    CIsoView::CurrentCommand->Command = FACurrentCommand::TileDraw;
                    CBrushSize::UpdateBrushSize(i);
                    return true;
                }
        }
    } while (false);



    if (nData == 50) // add tube
    {
        CIsoView::CurrentCommand->Command = 0x22;
        CIsoView::CurrentCommand->Type = 0;
        return true;
    }
    if (nData == 52) // add unidirectional tube
    {
        CIsoView::CurrentCommand->Command = 0x22;
        CIsoView::CurrentCommand->Type = 1;
        return true;
    }
    if (nData  >= Const_House + 5000 && nData < Const_House + 5008) // multiplayer locations
    {
        CIsoView::CurrentCommand->Command = 1;
        CIsoView::CurrentCommand->Type = 7;
        CIsoView::CurrentCommand->ObjectID = playersAtX[nData - Const_House - 5000];
        CIsoView::CurrentHouse() = playersAtX[nData - Const_House - 5000];
        return true;
    }

    int nCode = nData / 10000;
    nData %= 10000;

    if (nCode == 0) // main list
    {
        CIsoView::CurrentCommand->Command = 0x00;
        CIsoView::CurrentCommand->Type = 0;
    }
    if (nCode == 1) // Infantry
    {
        if (auto pSection = CINI::FAData().GetSection("PlaceRandomInfantryObList"))
        {
            int index = RandomTechno;
            for (const auto& pKey : pSection->GetEntities())
            {
                if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
                {
                    if (nData == index)
                    {
                        CIsoView::CurrentCommand->Command = 0x1;
                        CViewObjectsExt::PlacingRandomInfantry = index - RandomTechno;
                        return true;
                    }
                    index++;
                }
            }
        }
    }
    if (nCode == 2) // Building
    {
        if (auto pSection = CINI::FAData().GetSection("PlaceRandomBuildingObList"))
        {
            int index = RandomTechno;
            for (const auto& pKey : pSection->GetEntities())
            {
                if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
                {
                    if (nData == index)
                    {
                        CIsoView::CurrentCommand->Command = 0x1;
                        CViewObjectsExt::PlacingRandomStructure = index - RandomTechno;
                        return true;
                    }
                    index++;
                }
            }
        }
    }
    if (nCode == 3) // Aircraft
    {
        if (auto pSection = CINI::FAData().GetSection("PlaceRandomAircraftObList"))
        {
            int index = RandomTechno;
            for (const auto& pKey : pSection->GetEntities())
            {
                if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
                {
                    if (nData == index)
                    {
                        CIsoView::CurrentCommand->Command = 0x1;
                        CViewObjectsExt::PlacingRandomAircraft = index - RandomTechno;
                        return true;
                    }
                    index++;
                }
            }
        }
    }
    if (nCode == 4) // Vehicle
    {
        if (auto pSection = CINI::FAData().GetSection("PlaceRandomVehicleObList"))
        {
            int index = RandomTechno;
            for (const auto& pKey : pSection->GetEntities())
            {
                if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
                {
                    if (nData == index)
                    {
                        CIsoView::CurrentCommand->Command = 0x1;
                        CViewObjectsExt::PlacingRandomVehicle = index - RandomTechno;
                        return true;
                    }
                    index++;
                }
            }
        }
    }
    if (nCode == 5) // Terrain
    {
        if (auto pSection = CINI::FAData().GetSection("PlaceRandomTreeObList"))
        {
            int index = RandomTree;
            for (const auto& pKey : pSection->GetEntities())
            {
                if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
                {
                    if (nData == index)
                    {
                        CIsoView::CurrentCommand->Command = 0x1;
                        CViewObjectsExt::PlacingRandomTerrain = index - RandomTree;
                        return true;
                    }
                    index++;
                }
            }
        }
    }
    if (nCode == 6) // overlay
    {
        if (nData - 3000 >= Wall && nData - 3000 < WallEnd)
        {
            CIsoView::CurrentCommand->Command = 0x1;
            PlacingWall = nData - 3000 - Wall;
            return true;
        }
        else if (nData - 3000 == AddOre)
        {
            CIsoView::CurrentCommand->Command = 0x20; // Add / Reduce Ore
            CIsoView::CurrentCommand->Type = 0;
            return true;
        }
        else if (nData - 3000 == ReduceOre)
        {
            CIsoView::CurrentCommand->Command = 0x20; // Add / Reduce Ore
            CIsoView::CurrentCommand->Type = 1;
            return true;
        }
        else if (nData == 1) // Set overlay
        {
            FString message = Translations::TranslateOrDefault("ManualOverlayMessage", 
                "Please enter the value (0-254) of the overlay. Don't exceed this range.");
            FString title = Translations::TranslateOrDefault("ManualOverlayTitle",
                "Set overlay manually");

            bool enableExtOverlay = CMapDataExt::NewINIFormat >= 5
                || (ExtConfigs::ExtOverlays && Variables::RulesMap.ParseIndicies("OverlayTypes").size() >= 255);
            if (enableExtOverlay)
            {
                message = Translations::TranslateOrDefault("ManualOverlayMessageExt",
                    "Please enter the value (0-65534) of the overlay. Don't exceed this range.");
            }

            CIsoView::CurrentCommand->Command = 1;
            CIsoView::CurrentCommand->Type = 6;
            CIsoView::CurrentCommand->Param = 31;
            CIsoView::CurrentCommand->Overlay = 
                atoi(CInputMessageBox::GetString(message, title));
            CIsoView::CurrentCommand->Overlay = std::max(0, CIsoView::CurrentCommand->Overlay);
            
            if (CIsoView::CurrentCommand->Overlay >= 0xFF && !enableExtOverlay)
            {
                CIsoView::CurrentCommand->Overlay = 0;
            }
            
            return true;
        }
        else if (nData == 2) // Set overlay data
        {
            FString message = Translations::TranslateOrDefault("ManualOverlayDataMessage", 
                "Please enter the value (0-59) of the overlay data. Don't exceed this range.");
            FString title = Translations::TranslateOrDefault("ManualOverlayDataTitle",
                "Set overlay data manually");

            CIsoView::CurrentCommand->Command = 1;
            CIsoView::CurrentCommand->Type = 6;
            CIsoView::CurrentCommand->Param = 32;
            CIsoView::CurrentCommand->Overlay =
                atoi(CInputMessageBox::GetString(message, title));
            CIsoView::CurrentCommand->Overlay = std::clamp(CIsoView::CurrentCommand->Overlay, 0, 59);
            return true;
        }
        else if (auto pSection = CINI::FAData().GetSection("PlaceRandomOverlayList"))
        {
            int index = RandomRock;
            for (const auto& pKey : pSection->GetEntities())
            {
                if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
                {
                    if (nData - 3000 == index)
                    {
                        CIsoView::CurrentCommand->Command = 0x1;
                        CViewObjectsExt::PlacingRandomRock = index - RandomRock;
                        CFinalSunDlg::Instance->BrushSize.nCurSel = 0;
                        CFinalSunDlg::Instance->BrushSize.UpdateData(FALSE);
                        CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeX = 1;
                        CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeY = 1;
                        return true;
                    }
                    index++;
                }
            }
        }
    }
    if (nCode == 8) // Smudge
    {
        if (auto pSection = CINI::FAData().GetSection("PlaceRandomSmudgeList"))
        {
            int index = random1x1crater;
            for (const auto& pKey : pSection->GetEntities())
            {
                if (auto pSection2 = CINI::FAData().GetSection(pKey.second))
                {
                    if (nData == index)
                    {
                        CIsoView::CurrentCommand->Command = 0x1;
                        CViewObjectsExt::PlacingRandomSmudge  = index - random1x1crater;
                        return true;
                    }
                    index++;
                }
            }
        }
    }

    if (nCode == 9) // PropertyBrush
    {
        if (nData == Set_Building)
        {
            CViewObjectsExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Building())
            {
                CIsoView::CurrentCommand->Command = 0x17; // PropertyBrush
                CIsoView::CurrentCommand->Type = Set_Building;
            }
            else
                CIsoView::CurrentCommand->Command = FACurrentCommand::Nothing;

            CViewObjectsExt::InitPropertyDlgFromProperty = false;

            return true;
        }
        else if (nData == Set_Infantry)
        {
            CViewObjectsExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Infantry())
            {
                CIsoView::CurrentCommand->Command = 0x17;
                CIsoView::CurrentCommand->Type = Set_Infantry;
            }
            else
                CIsoView::CurrentCommand->Command = FACurrentCommand::Nothing;

            CViewObjectsExt::InitPropertyDlgFromProperty = false;

            return true;
        }
        else if (nData == Set_Vehicle)
        {
            CViewObjectsExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Vehicle())
            {
                CIsoView::CurrentCommand->Command = 0x17;
                CIsoView::CurrentCommand->Type = Set_Vehicle;
            }
            else
                CIsoView::CurrentCommand->Command = FACurrentCommand::Nothing;

            CViewObjectsExt::InitPropertyDlgFromProperty = false;

            return true;
        }
        else if (nData == Set_Aircraft)
        {
            CViewObjectsExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Aircraft())
            {
                CIsoView::CurrentCommand->Command = 0x17;
                CIsoView::CurrentCommand->Type = Set_Aircraft;
            }
            else
                CIsoView::CurrentCommand->Command = FACurrentCommand::Nothing;

            CViewObjectsExt::InitPropertyDlgFromProperty = false;

            return true;
        }
    }

    if (nCode == 10) // InfantrySubCell
    {
        if (nData == i1_2_3)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i1_2_3;
            return true;
        }
        if (nData == i1_3_2)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i1_3_2;
            return true;
        }
        if (nData == i2_1_3)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i2_1_3;
            return true;
        }
        if (nData == i2_3_1)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i2_3_1;
            return true;
        }
        if (nData == i3_1_2)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i3_1_2;
            return true;
        }
        if (nData == i3_2_1)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i3_2_1;
            return true;
        }
        if (nData == i4_2_3)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i4_2_3;
            return true;
        }
        if (nData == i4_3_2)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i4_3_2;
            return true;
        }
        if (nData == i2_4_3)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i2_4_3;
            return true;
        }
        if (nData == i2_3_4)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i2_3_4;
            return true;
        }
        if (nData == i3_4_2)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i3_4_2;
            return true;
        }
        if (nData == i3_2_4)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = i3_2_4;
            return true;
        }        
        if (nData == changeOrder)
        {

            CIsoView::CurrentCommand->Command = 0x18; // InfantrySubCell
            CIsoView::CurrentCommand->Type = changeOrder;
            return true;
        }
    }

    if (nCode == 11) // BaseNode
    {
        if (nData == MoveUp)
        {
            CIsoView::CurrentCommand->Command = 0x1A; // BaseNode
            CIsoView::CurrentCommand->Type = MoveUp;

            return true;
        }
        if (nData == MoveDown)
        {
            CIsoView::CurrentCommand->Command = 0x1A; // BaseNode
            CIsoView::CurrentCommand->Type = MoveDown;
            return true;
        }
        if (nData == Move)
        {
            CIsoView::CurrentCommand->Command = 0x1A; // BaseNode
            CIsoView::CurrentCommand->Type = Move;
            return true;
        }
    }
    if (nCode == 12) // view object
    {
        if (nData == ObjectTerrainType::All)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::All;

            return true;
        }
        if (nData == ObjectTerrainType::Infantry)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Infantry;

            return true;
        }
        if (nData == ObjectTerrainType::Vehicle)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Vehicle;

            return true;
        }
        if (nData == ObjectTerrainType::Aircraft)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Aircraft;

            return true;
        }
        if (nData == ObjectTerrainType::Building)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Building;

            return true;
        }
        if (nData == ObjectTerrainType::Object)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Object;

            return true;
        }
        if (nData == ObjectTerrainType::Tile)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Tile;

            return true;
        }
        if (nData == ObjectTerrainType::BaseNode)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::BaseNode;

            return true;
        }
        if (nData == ObjectTerrainType::Terrain)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Terrain;

            return true;
        }
        if (nData == ObjectTerrainType::Smudge)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Smudge;

            return true;
        }
        if (nData == ObjectTerrainType::Celltag)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Celltag;

            return true;
        }
        if (nData == ObjectTerrainType::Overlay)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Overlay;

            return true;
        }
        if (nData == ObjectTerrainType::Waypoints)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::Waypoints;

            return true;
        }
        if (nData == ObjectTerrainType::AllTerrain)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::AllTerrain;

            return true;
        }
        if (nData == ObjectTerrainType::House)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::House;

            return true;
        }
        if (nData == ObjectTerrainType::WeaponRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::WeaponRange;

            return true;
        }
        if (nData == ObjectTerrainType::SecondaryWeaponRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::SecondaryWeaponRange;

            return true;
        }
        if (nData == ObjectTerrainType::DeathWeaponRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::DeathWeaponRange;

            return true;
        }
        if (nData == ObjectTerrainType::GapRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::GapRange;

            return true;
        }
        if (nData == ObjectTerrainType::SensorsRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::SensorsRange;

            return true;
        }
        if (nData == ObjectTerrainType::CloakRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::CloakRange;

            return true;
        }
        if (nData == ObjectTerrainType::PsychicRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::PsychicRange;

            return true;
        }
        if (nData == ObjectTerrainType::GuardRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::GuardRange;

            return true;
        }
        if (nData == ObjectTerrainType::SightRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::SightRange;

            return true;
        }
        if (nData == ObjectTerrainType::AllRange)
        {
            CIsoView::CurrentCommand->Command = 0x1B; // view object
            CIsoView::CurrentCommand->Type = ObjectTerrainType::AllRange;

            return true;
        }
    }
    if (nCode == 13) // MultiSelection
    {
        if (nData == AllDelete)
        {
            CIsoView::CurrentCommand->Command = 0;
            CIsoView::CurrentCommand->Type = 0;
            MultiSelection::Clear();
            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
            return true;
        }
        else if (nData == ReplaceHide)
        {
            MultiSelection::LastAddedCoord.X = -1;
            MultiSelection::LastAddedCoord.Y = -1;
            CIsoView::CurrentCommand->Command = 0;
            CIsoView::CurrentCommand->Type = 0;
            for (auto& coord : MultiSelection::SelectedCoords)
            {
                if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
                {
                    auto cell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
                    cell->Flag.IsHiddenCell = true;
                }
            }
            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
            return true;
        }
        else if (nData == ConnectedAdd || nData == ConnectedDelete)
        {
            MultiSelection::dlg.DoModal();
            MultiSelection::LastAddedCoord.X = -1;
            MultiSelection::LastAddedCoord.Y = -1;
            CIsoView::CurrentCommand->Command = 0x1D; // MultiSelection
            CIsoView::CurrentCommand->Type = nData;
            return true;
        }
        else
        {
            MultiSelection::LastAddedCoord.X = -1;
            MultiSelection::LastAddedCoord.Y = -1;
            CIsoView::CurrentCommand->Command = 0x1D; // MultiSelection
            CIsoView::CurrentCommand->Type = nData;
            return true;
        }
    }
    if (nCode == 14) // Cliff
    {
        CIsoView::CurrentCommand->Command = 0x1E; // Cliff
        CIsoView::CurrentCommand->Type = nData;

        return true;
    }
    if (nCode == 15) // Annotation
    {
        CIsoView::CurrentCommand->Command = 0x21; // Annotation
        CIsoView::CurrentCommand->Type = nData;
        return true;
    }
    // 0x1F Terrain Generator
    // 0x20 Modify Ore
    // 0x21 Annotation
    // 0x22 Tube
    // 0x23 Lua Script
    return false;
}
