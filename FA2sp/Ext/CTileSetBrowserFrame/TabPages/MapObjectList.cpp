#include "MapObjectList.h"

#include "../../../FA2sp.h"
#include "../../../Ext/CIsoView/Body.h"
#include "../../../Ext/CMapData/Body.h"
#include "../../../ExtraWindow/Common.h"
#include "../../../ExtraWindow/CObjectSearch/CObjectSearch.h"
#include "../../../Helpers/Translations.h"
#include "../../../Miscs/DialogStyle.h"
#include "../../../Ext/CFinalSunApp/Body.h"
#include <CMapData.h>
#include <CObjectDatas.h>
#include <CIsoView.h>

#include <algorithm>
#include <commctrl.h>
#include <cstdlib>

MapObjectList MapObjectList::Instance;

namespace
{
    constexpr UINT RefreshCommand = 1;
    constexpr UINT SearchControl = 2;
    constexpr UINT ListControl = 3;
    constexpr UINT CountControl = 4;
    constexpr UINT SearchLabelControl = 5;

    int CompareText(const FString& left, const FString& right)
    {
        return _stricmp(left.c_str(), right.c_str());
    }

    FString GetTechnoName(const FString& technoID)
    {
        FString display;
        FString name = Variables::RulesMap.GetString(technoID, "Name");
        if (name.IsEmpty() || !Translations::GetTranslationItem(name, display))
        {
            display = CViewObjectsExt::QueryUIName(technoID, true);
        }
        if (display != technoID)
        {
            display.Format("%s (%s)", display, technoID);
        }   
        return display;
    }
}

void MapObjectList::Create(HWND hParent)
{
    WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = static_cast<HINSTANCE>(FA2sp::hInstance);
    wc.lpszClassName = "FA2spMapObjectList";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    RegisterClassEx(&wc);

    RECT rect{};
    GetClientRect(hParent, &rect);
    m_hWnd = CreateWindowEx(
        0, wc.lpszClassName, nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        2, 20, rect.right - 6, rect.bottom - 26,
        hParent, nullptr, wc.hInstance, this
    );
    CreateControls();
}

void MapObjectList::CreateControls()
{
    if (!m_hWnd)
        return;

    const auto s = CFinalSunAppExt::ProgramScaleFactor;

    m_hSearchLabel = CreateWindow(
        "STATIC", Translations::TranslateOrDefault("MapObjectList.Search", "Search:"),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        8, 10, 44 * s, 16 * s, m_hWnd,
        reinterpret_cast<HMENU>(SearchLabelControl),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    m_hSearch = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        56 * s, 8, 210 * s, 20 * s, m_hWnd,
        reinterpret_cast<HMENU>(SearchControl),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    m_hRefresh = CreateWindow(
        "BUTTON", Translations::TranslateOrDefault("MapObjectList.Refresh", "Refresh"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        270 * s, 8, 80 * s, 20 * s, m_hWnd,
        reinterpret_cast<HMENU>(RefreshCommand),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    m_hCount = CreateWindow(
        "STATIC", nullptr,
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        360 * s, 12, 120 * s, 15 * s, m_hWnd,
        reinterpret_cast<HMENU>(CountControl),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    m_hList = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL |
        LVS_SHOWSELALWAYS | LVS_OWNERDATA,
        4, 29 * s, 476 * s, 300 * s, m_hWnd,
        reinterpret_cast<HMENU>(ListControl),
        static_cast<HINSTANCE>(FA2sp::hInstance), nullptr
    );

    HFONT hFont = DarkTheme::GetModernDefaultGUIFont();
    SendMessage(m_hSearchLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_hSearch, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_hRefresh, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_hCount, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_hList, WM_SETFONT, (WPARAM)hFont, TRUE);

    ListView_SetExtendedListViewStyle(
        m_hList, LVS_EX_FULLROWSELECT 
    );

    if (ExtConfigs::EnableDarkMode)
    {
        DarkTheme::SetDarkTheme(m_hWnd);
        DarkTheme::SubclassAllControls(m_hWnd);

        ::SendMessage(m_hList, LVM_SETTEXTBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(m_hList, LVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));
        ::SendMessage(m_hList, 0x10C8, 0, RGB(60, 60, 60)); // LVM_SETGRIDCOLOR

        DarkTheme::SubclassListViewHeader(m_hList);
        m_pOriginalListViewProc = (WNDPROC)GetWindowLongPtr(m_hList, GWLP_WNDPROC);
        if (m_pOriginalListViewProc)
        {
            SetWindowLongPtr(m_hList, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);
        }
        InvalidateRect(m_hList, NULL, TRUE);
    }

    const char* headers[] = {
        "Index", "Type", "Name (ID)", "Coordinate", "House", "Health (%)",
        "Facing", "Status", "Tag"
    };
    const int widths[] = { 
        static_cast<int>(50 * s), 
        static_cast<int>(80 * s), 
        static_cast<int>(150 * s), 
        static_cast<int>(80 * s), 
        static_cast<int>(130 * s), 
        static_cast<int>(75 * s),
        static_cast<int>(75 * s),
        static_cast<int>(70 * s),
        static_cast<int>(240 * s)
    };
    for (int i = 0; i < Column::Count; ++i)
    {
        LVCOLUMN column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = const_cast<char*>(headers[i]);
        column.cx = widths[i];
        column.iSubItem = i;
        ListView_InsertColumn(m_hList, i, &column);
    }

    FString text;
    const char* translationKeys[] = {
        "MapObjectList.Index", "MapObjectList.Type",
        "MapObjectList.DisplayName", "MapObjectList.Coordinate",
        "MapObjectList.House", "MapObjectList.Health",
        "MapObjectList.Facing", "MapObjectList.Status", "MapObjectList.Tag"
    };
    for (int i = 0; i < Column::Count; ++i)
    {
        if (!Translations::GetTranslationItem(translationKeys[i], text))
            continue;
        LVCOLUMN column{};
        column.mask = LVCF_TEXT;
        column.pszText = const_cast<char*>(text.c_str());
        ListView_SetColumn(m_hList, i, &column);
    }

    Refresh();
}

void MapObjectList::Refresh()
{
    if (!m_hWnd)
        return;

    LoadRows();
    ApplyFilterAndSort();
    m_dataDirty = false;
}

void MapObjectList::LoadRows()
{
    m_rows.clear();

    auto loadSection = [this](const char* section, ObjectType type)
    {
        auto pSection = CMapData::Instance->INI.GetSection(section);
        if (!pSection)
            return;

        int index = 0;
        for (auto& pair : pSection->GetEntities())
        {
            auto values = FString::SplitString(pair.second);
            if (values.size() < 5)
            {
                ++index;
                continue;
            }

            Row row;
            row.Index = index++;
            row.Type = type;
            // Map INI order is OWNER, TypeID, Health, X, Y [, SubCell, ...].
            row.X = atoi(values[3]);
            row.Y = atoi(values[4]);
            row.DisplayName = GetTechnoName(values[1]);
            row.TypeID = values[1];
            row.House = Translations::ParseHouseName(values[0], true);
            row.Health = values[2];
            if (type == ObjectType::Infantry && values.size() >= 6)
                row.SubCell = atoi(values[5]);

            // Facing, Status, Tag indices vary by type
            switch (type)
            {
            case ObjectType::Infantry:
                if (values.size() >= 8) { row.Facing = atoi(values[7]); row.Status = values[6]; }
                if (values.size() >= 9) row.Tag = ExtraWindow::GetTagDisplayName(values[8]);
                break;
            case ObjectType::Building:
                if (values.size() >= 6) { row.Facing = atoi(values[5]); row.Tag = ExtraWindow::GetTagDisplayName(values[6]); }
                break;
            case ObjectType::Unit:
            case ObjectType::Aircraft:
                if (values.size() >= 7) { row.Facing = atoi(values[5]); row.Status = values[6]; }
                if (values.size() >= 8) row.Tag = ExtraWindow::GetTagDisplayName(values[7]);
                break;
            }

            m_rows.push_back(std::move(row));
        }
    };

    loadSection("Infantry", ObjectType::Infantry);
    loadSection("Units", ObjectType::Unit);
    loadSection("Structures", ObjectType::Building);
    loadSection("Aircraft", ObjectType::Aircraft);
}

void MapObjectList::ApplyFilterAndSort()
{
    char buffer[512]{};
    if (m_hSearch)
        GetWindowText(m_hSearch, buffer, sizeof(buffer) - 1);
    FString filter = buffer;

    m_visibleRows.clear();

    if (!filter.empty())
    {
        LabelMatcher matcher(filter);
        for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
        {
            FString all;
            for (int column = Column::Index; column < Column::Count; ++column)
            {
                if (!all.empty())
                    all += " ";
                all += GetCellText(m_rows[i], static_cast<Column>(column));
            }
            if (matcher.Match(all))
                m_visibleRows.push_back(i);
        }
    }
    else
    {
        for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
            m_visibleRows.push_back(i);
    }

    std::stable_sort(m_visibleRows.begin(), m_visibleRows.end(),
        [this](int leftIndex, int rightIndex)
        {
            const auto& left = m_rows[leftIndex];
            const auto& right = m_rows[rightIndex];
            if (m_sortColumn == Column::Index)
            {
                if (left.Index != right.Index)
                    return m_sortAscending ? left.Index < right.Index : left.Index > right.Index;
                return static_cast<int>(left.Type) < static_cast<int>(right.Type);
            }
            if (m_sortColumn == Column::Health)
            {
                const int l = atoi(left.Health);
                const int r = atoi(right.Health);
                if (l != r)
                    return m_sortAscending ? l < r : l > r;
            }
            else
            {
                const int result = CompareText(
                    GetCellText(left, static_cast<Column>(m_sortColumn)),
                    GetCellText(right, static_cast<Column>(m_sortColumn))
                );
                if (result != 0)
                    return m_sortAscending ? result < 0 : result > 0;
            }
            return left.Index < right.Index;
        });

    RebuildList();
}

FString MapObjectList::GetCellText(const Row& row, Column column) const
{
    FString text;
    switch (column)
    {
    case Column::Index:
        text.Format("%d", row.Index);
        break;
    case Column::Type:
        switch (row.Type)
        {
        case ObjectType::Infantry:
            text = Translations::TranslateOrDefault(
                "MapObjectList.Type.Infantry", "Infantry"
            );
            break;
        case ObjectType::Unit:
            text = Translations::TranslateOrDefault(
                "MapObjectList.Type.Unit", "Vehicle"
            );
            break;
        case ObjectType::Aircraft:
            text = Translations::TranslateOrDefault(
                "MapObjectList.Type.Aircraft", "Aircraft"
            );
            break;
        case ObjectType::Building:
            text = Translations::TranslateOrDefault(
                "MapObjectList.Type.Building", "Building"
            );
            break;
        }
        break;
    case Column::DisplayName:
        text = row.DisplayName;
        break;
    case Column::Coordinate:
        text.Format("%d, %d", row.X, row.Y);
        break;
    case Column::House:
        text = row.House;
        break;
    case Column::Health:
        text.Format("%d (%d%%)", atoi(row.Health), (atoi(row.Health) * 100 + 128) / 256);
        break;
    case Column::Facing:
        if (row.Facing >= 0 && row.Facing <= 224 && row.Facing % 32 == 0)
        {
            const char* directionKeys[] = {
                "Direction.NorthEast", "Direction.East", "Direction.SouthEast", "Direction.South",
                "Direction.SouthWest", "Direction.West", "Direction.NorthWest", "Direction.North"
            };
            const char* directionDefaults[] = {
                "North-East", "East", "South-East", "South",
                "South-West", "West", "North-West", "North"
            };
            text.Format("%s (%d)",
                Translations::TranslateOrDefault(directionKeys[row.Facing / 32],
                    directionDefaults[row.Facing / 32]), row.Facing);
        }
        else
            text.Format("%d", row.Facing);
        break;
    case Column::Status:
        text = row.Status;
        break;
    case Column::Tag:
        text = row.Tag;
        break;
    default:
        break;
    }
    return text;
}

void MapObjectList::RebuildList()
{
    if (!m_hList)
        return;

    ListView_SetItemCountEx(m_hList, m_visibleRows.size(), LVSICF_NOINVALIDATEALL);
    InvalidateRect(m_hList, NULL, TRUE);
    UpdateCount();
}

void MapObjectList::UpdateCount()
{
    if (!m_hCount)
        return;

    FString text;
    FString label = Translations::TranslateOrDefault("MapObjectList.Count", "Count");
    text.Format("%s: %d", label.c_str(), static_cast<int>(m_visibleRows.size()));
    SetWindowText(m_hCount, text);
}

void MapObjectList::SortByColumn(int column)
{
    if (m_sortColumn == column)
        m_sortAscending = !m_sortAscending;
    else
    {
        m_sortColumn = column;
        m_sortAscending = true;
    }
    ApplyFilterAndSort();
}

void MapObjectList::HandleSelection(bool doubleClick)
{
    int selected = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
    if (selected < 0)
        return;

    if (selected >= static_cast<int>(m_visibleRows.size()))
        return;

    const auto& row = m_rows[m_visibleRows[selected]];
    CObjectSearch::MoveToMapCoord(row.Y, row.X);
    if (doubleClick)
        OpenProperties(m_visibleRows[selected]);
}

void MapObjectList::OpenProperties(int rowIndex)
{
    auto getIniValue = [](const Row& target)
    {
        const char* section = nullptr;
        switch (target.Type)
        {
        case ObjectType::Infantry:
            section = "Infantry";
            break;
        case ObjectType::Unit:
            section = "Units";
            break;
        case ObjectType::Building:
            section = "Structures";
            break;
        case ObjectType::Aircraft:
            section = "Aircraft";
            break;
        }

        if (!section)
            return FString();

        auto pSection = CMapData::Instance->INI.GetSection(section);
        if (!pSection)
            return FString();

        int index = 0;
        for (auto& pair : pSection->GetEntities())
        {
            if (index++ == target.Index)
            {
                FString value;
                value = pair.second;
                return value;
            }
        }
        return FString();
    };

    auto& row = m_rows[rowIndex];
    const FString iniValue = getIniValue(row);

    // Check if cached data matches INI - if stale, full refresh instead of opening properties
    if (!iniValue.empty())
    {
        auto values = FString::SplitString(iniValue);
        bool match = false;
        if (values.size() >= 5)
        {
            int iniX = atoi(values[3]);
            int iniY = atoi(values[4]);
            FString iniTypeID = values[1];
            FString iniHouse = Translations::ParseHouseName(values[0], true);
            FString iniHealth = values[2];

            match = (row.X == iniX && row.Y == iniY &&
                     row.TypeID == iniTypeID &&
                     row.House == iniHouse &&
                     row.Health == iniHealth);

            if (match && row.Type == ObjectType::Infantry && values.size() >= 6)
                match = (row.SubCell == atoi(values[5]));

            // Compare Facing, Status, Tag by type
            if (match)
            {
				FString tagID = row.Tag;
				FString::TrimIndex(tagID);
				switch (row.Type)
                {
                case ObjectType::Infantry:
                    if (values.size() >= 8)
                        match = (row.Facing == atoi(values[7]) && row.Status == values[6]);
                    if (match && values.size() >= 9)
                        match = (tagID == values[8]);
                    break;
                case ObjectType::Building:
                    if (values.size() >= 6)
                        match = (row.Facing == atoi(values[5]));
                    if (match && values.size() >= 7)
                        match = (tagID == values[6]);
                    break;
                case ObjectType::Unit:
                case ObjectType::Aircraft:
                    if (values.size() >= 7)
                        match = (row.Facing == atoi(values[5]) && row.Status == values[6]);
                    if (match && values.size() >= 8)
                        match = (tagID == values[7]);
                    break;
                }
            }
        }

        if (!match)
        {
            Refresh();
            return;
        }
    }

    int type = static_cast<int>(row.Type);
    if (row.Type == ObjectType::Building)
        type = 100;

    CIsoView::GetInstance()->HandleProperties(row.Index, type);

    const FString after = getIniValue(row);
    if (iniValue != after)
    {
        // Re-read this row's data from INI instead of full refresh
        const char* section = nullptr;
        switch (row.Type)
        {
        case ObjectType::Infantry: section = "Infantry"; break;
        case ObjectType::Unit:     section = "Units"; break;
        case ObjectType::Building: section = "Structures"; break;
        case ObjectType::Aircraft: section = "Aircraft"; break;
        }

        if (section)
        {
            if (auto pSection = CMapData::Instance->INI.GetSection(section))
            {
                int index = 0;
                for (auto& pair : pSection->GetEntities())
                {
                    if (index++ == row.Index)
                    {
                        auto values = FString::SplitString(pair.second);
                        if (values.size() >= 5)
                        {
                            row.X = atoi(values[3]);
                            row.Y = atoi(values[4]);
                            row.DisplayName = GetTechnoName(values[1]);
                            row.TypeID = values[1];
                            row.House = Translations::ParseHouseName(values[0], true);
                            row.Health = values[2];
                            if (row.Type == ObjectType::Infantry && values.size() >= 6)
                                row.SubCell = atoi(values[5]);

                            // Re-read Facing, Status, Tag by type
                            switch (row.Type)
                            {
                            case ObjectType::Infantry:
                                if (values.size() >= 8) { row.Facing = atoi(values[7]); row.Status = values[6]; }
                                if (values.size() >= 9) row.Tag = ExtraWindow::GetTagDisplayName(values[8]);
                                break;
                            case ObjectType::Building:
                                if (values.size() >= 6) { row.Facing = atoi(values[5]); }
                                if (values.size() >= 7) row.Tag = ExtraWindow::GetTagDisplayName(values[6]);
                                break;
                            case ObjectType::Unit:
                            case ObjectType::Aircraft:
                                if (values.size() >= 7) { row.Facing = atoi(values[5]); row.Status = values[6]; }
                                if (values.size() >= 8) row.Tag = ExtraWindow::GetTagDisplayName(values[7]);
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }

        ApplyFilterAndSort();
    }
}

LRESULT CALLBACK MapObjectList::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DarkTheme::MyCallWindowProcA(Instance.m_pOriginalListViewProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MapObjectList::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<MapObjectList*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
    );
    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<MapObjectList*>(create->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hWnd = hWnd;
    }
    return self ? self->HandleMessage(hWnd, message, wParam, lParam)
                : DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT MapObjectList::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        OnSize();
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == SearchControl && HIWORD(wParam) == EN_CHANGE)
            ApplyFilterAndSort();
        else if (LOWORD(wParam) == RefreshCommand && HIWORD(wParam) == BN_CLICKED)
            Refresh();
        return 0;
    case WM_NOTIFY:
    {
        auto* header = reinterpret_cast<LPNMHDR>(lParam);
        if (header->idFrom == ListControl)
        {
            if (header->code == LVN_GETDISPINFO)
            {
                auto* plvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
                int itemIndex = plvdi->item.iItem;
                if (itemIndex >= 0 && itemIndex < static_cast<int>(m_visibleRows.size()))
                {
                    if (plvdi->item.mask & LVIF_TEXT)
                    {
                        const auto& row = m_rows[m_visibleRows[itemIndex]];
                        FString text = GetCellText(row, static_cast<Column>(plvdi->item.iSubItem));
                        lstrcpynA(plvdi->item.pszText, text.c_str(), plvdi->item.cchTextMax);
                    }
                }
                return 0;
            }
            else if (header->code == LVN_COLUMNCLICK)
            {
                auto* info = reinterpret_cast<LPNMLISTVIEW>(lParam);
                SortByColumn(info->iSubItem);
            }
            else if (header->code == LVN_ITEMCHANGED || header->code == NM_DBLCLK)
            {
                HandleSelection(header->code == NM_DBLCLK);
            }
        }
        return 0;
    }
    case 114514:
        m_dataDirty = true;
        if (!m_refreshPosted)
        {
            m_refreshPosted = true;
            ::PostMessage(hWnd, WM_APP + 1, 0, 0);
        }
        return 0;
    case WM_APP + 1:
        m_refreshPosted = false;
        if (IsVisible() && m_dataDirty)
            Refresh();
        return 0;
    case WM_DESTROY:
        m_hWnd = nullptr;
        return 0;
    case WM_ERASEBKGND:
        if (ExtConfigs::EnableDarkMode)
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, DarkTheme::g_hDarkBackgroundBrush);
            return 1;
        }
        break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        if (ExtConfigs::EnableDarkMode)
        {
            SetTextColor(hdc, DarkColors::LightText);
            SetBkColor(hdc, DarkColors::Background);
            return reinterpret_cast<LRESULT>(DarkTheme::g_hDarkBackgroundBrush);
        }
        SetBkColor(hdc, RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    }
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        if (ExtConfigs::EnableDarkMode)
        {
            SetTextColor(hdc, DarkColors::LightText);
            SetBkColor(hdc, DarkColors::Background);
            return reinterpret_cast<LRESULT>(DarkTheme::g_hDarkBackgroundBrush);
        }
        break;
    }
    default:
        break;
    }
    return DarkTheme::MyDefWindowProcA(hWnd, message, wParam, lParam);
}

void MapObjectList::OnSize() const
{
    if (!m_hWnd)
        return;

    const auto s = CFinalSunAppExt::ProgramScaleFactor;

    RECT parentRect{};
    HWND parent = GetParent(m_hWnd);
    GetClientRect(parent, &parentRect);
    int tabPageheight = CTileSetBrowserFrameExt::GetTabPageHeight();
    ::MoveWindow(
        m_hWnd, 2, tabPageheight,
        std::max(static_cast<int>(100 * s), static_cast<int>(parentRect.right) - 6),
        std::max(static_cast<int>(40 * s), static_cast<int>(parentRect.bottom) - tabPageheight - 6),
        FALSE
    );

    RECT rect{};
    GetClientRect(m_hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    ::MoveWindow(m_hSearch, 56 * s, 8, std::max(80 * s, width - 272 * s), 20 * s, FALSE);
    ::MoveWindow(m_hRefresh, std::max(88 * s, width - 210 * s), 8, 80 * s, 20 * s, FALSE);
    ::MoveWindow(m_hCount, std::max(180 * s, width - 125 * s), 12, 120 * s, 15 * s, FALSE);
    ::MoveWindow(m_hList, 4, 29 * s, std::max(100 * s, width - 8 * s), std::max(40 * s, height - 29 * s), FALSE);
}

void MapObjectList::ShowWindow(bool bShow) const
{
    ::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
}

void MapObjectList::ShowWindow() const
{
    const_cast<MapObjectList*>(this)->Refresh();
    ShowWindow(true);
}

void MapObjectList::HideWindow() const
{
    ShowWindow(false);
}

bool MapObjectList::IsValid() const
{
    return m_hWnd != nullptr;
}

bool MapObjectList::IsVisible() const
{
    return IsValid() && ::IsWindowVisible(m_hWnd);
}

HWND MapObjectList::GetHwnd() const
{
    return m_hWnd;
}

MapObjectList::operator HWND() const
{
    return GetHwnd();
}
