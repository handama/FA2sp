#include "CLuaConsole.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Sol/sol.hpp"
#include <cctype>
#include <chrono>
#include "../../Miscs/MultiSelection.h"
#include <unordered_set>
#include "../CNewComboUInputDlg/CNewComboUInputDlg.h"
#include "../CListUInputDlg/CListUInputDlg.h"
#include <CInputMessageBox.h>

namespace LuaFunctions
{
	static long long time = 0;
	static std::map<ppmfc::CString, CINI*> LoadedINIs;
	static std::unordered_set<std::string> UsedINIIndices;

	static void write_lua_console(std::string text)
	{
		text = ">> " + text + "\r\n";;
		int len = GetWindowTextLength(CLuaConsole::hOutputBox);
		SendMessage(CLuaConsole::hOutputBox, EM_SETSEL, len, len);

		SendMessage(CLuaConsole::hOutputBox, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
		SendMessage(CLuaConsole::hOutputBox, EM_SCROLLCARET, 0, 0);

		auto&& now = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (now - time > 2000)
		{
			time = now;
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	static void clear()
	{
		SendMessage(CLuaConsole::hOutputBox, WM_SETTEXT, 0, (LPARAM)"");
	}

	static void lua_print(sol::variadic_args args)
	{
		std::string output = "";
		for (auto arg : args) {
			switch (arg.get_type()) {
			case sol::type::nil:
				output += "nil ";
				break;
			case sol::type::string:
			case sol::type::number:
				output += arg.get<std::string>() + " ";
				break;
			case sol::type::boolean:
				output += (arg.as<bool>() ? "true " : "false ");
				break;
			default:
				output += "[unsupported type] ";
				break;
			}
		}
		write_lua_console(output);
	}

	static void avoid_time_out()
	{
		auto&& now = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (now - time > 2000)
		{
			time = now;
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	static void sleep(int ms)
	{
		Sleep(ms);
	}

	static int message_box(std::string text, std::string title, int format) {
		static const std::unordered_map<int, UINT> formatMap = {
			{0, MB_OK},                       // OK button
			{1, MB_OKCANCEL},                 // OK/Cancel buttons
			{2, MB_YESNO},                    // Yes/No buttons
			{3, MB_YESNOCANCEL},              // Yes/No/Cancel buttons
			{4, MB_RETRYCANCEL},              // Retry/Cancel buttons
			{5, MB_ABORTRETRYIGNORE},         // Abort/Retry/Ignore buttons
			{6, MB_CANCELTRYCONTINUE},        // Cancel/Retry/Continue buttons

			{7, MB_OK | MB_ICONINFORMATION}, // OK button with Information icon
			{8, MB_OK | MB_ICONWARNING},     // OK button with Warning icon
			{9, MB_OK | MB_ICONERROR},       // OK button with Error icon
			{10, MB_YESNO | MB_ICONQUESTION}, // Yes/No buttons with Question icon

			{11, MB_OKCANCEL | MB_ICONINFORMATION}, // OK/Cancel with Information icon
			{12, MB_OKCANCEL | MB_ICONWARNING},     // OK/Cancel with Warning icon
			{13, MB_OKCANCEL | MB_ICONERROR},       // OK/Cancel with Error icon
			{14, MB_OKCANCEL | MB_ICONQUESTION},    // OK/Cancel with Question icon

			{15, MB_RETRYCANCEL | MB_ICONERROR}, // Retry/Cancel buttons with Error icon
			{16, MB_ABORTRETRYIGNORE | MB_ICONERROR} // Abort/Retry/Ignore buttons with Error icon
		};

		static const std::unordered_map<int, std::vector<int>> returnValueMap = {
			{MB_OK,                  {IDOK}},                     // OK -> [0]
			{MB_OKCANCEL,            {IDOK, IDCANCEL}},           // OK/Cancel -> [0,1]
			{MB_YESNO,               {IDYES, IDNO}},              // Yes/No -> [0,1]
			{MB_YESNOCANCEL,         {IDYES, IDNO, IDCANCEL}},    // Yes/No/Cancel -> [0,1,2]
			{MB_RETRYCANCEL,         {IDRETRY, IDCANCEL}},        // Retry/Cancel -> [0,1]
			{MB_ABORTRETRYIGNORE,    {IDABORT, IDRETRY, IDIGNORE}}, // Abort/Retry/Ignore -> [0,1,2]
			{MB_CANCELTRYCONTINUE,   {IDCANCEL, IDTRYAGAIN, IDCONTINUE}} // Cancel/Try Again/Continue -> [0,1,2]
		};

		auto it = formatMap.find(format);
		UINT style = (it != formatMap.end()) ? it->second : MB_OK;

		int result = MessageBoxA(CLuaConsole::GetHandle(), text.c_str(), title.c_str(), style);

		auto returnIt = returnValueMap.find(style & ~(MB_ICONMASK));
		if (returnIt != returnValueMap.end()) {
			const std::vector<int>& buttons = returnIt->second;
			for (size_t i = 0; i < buttons.size(); ++i) {
				if (result == buttons[i]) {
					return static_cast<int>(i+1); // to fit lua
				}
			}
		}

		return -1;
	}

	class select_box
	{
	public:
		std::vector<std::pair<std::string, std::string>> options;
		std::string caption;
		std::string selected_key;
		std::string selected_value;

		select_box(std::string cap)
		{
			caption = cap;
		}
		void add_option(std::string key, std::string value)
		{
			options.push_back(std::make_pair(key, value));
		}
		void sort_options(bool second = false) {
			if (!second) {
				std::sort(options.begin(), options.end(), [](const auto& lhs, const auto& rhs) {
					return ExtraWindow::SortRawStrings(lhs.first, rhs.first);
					});
			}
			else {
				std::sort(options.begin(), options.end(), [](const auto& lhs, const auto& rhs) {
					return ExtraWindow::SortRawStrings(lhs.second, rhs.second);
					});
			}
		}
		std::string do_modal()
		{
			CNewComboUInputDlg dlg;
			dlg.m_type = COMBOUINPUT_ALL_CUSTOM;
			dlg.m_Caption = caption.c_str();
			for (const auto& [key, value] : options)
			{
				if (value == "")
					dlg.m_CustomStrings.push_back(key);
				else
					dlg.m_CustomStrings.push_back(key + " - " + value);
			}
			dlg.DoModal();
			for (const auto& [key, value] : options)
			{
				if (value == "")
				{
					if (dlg.m_ComboOri == key)
					{
						selected_key = key;
						selected_value = value;
						return selected_key;
					}
				}
				else
				{
					if (key + " - " + value == (std::string)dlg.m_ComboOri)
					{
						selected_key = key;
						selected_value = value;
						return selected_key;
					}
				}
			}
			return "";
		}
	};

	class multi_select_box
	{
	public:
		std::vector<std::pair<std::string, std::string>> options;
		std::string caption;
		std::vector<std::string> selected_keys;
		std::vector<std::string> selected_values;

		multi_select_box(std::string cap)
		{
			caption = cap;
		}
		void add_option(std::string key, std::string value)
		{
			options.push_back(std::make_pair(key, value));
		}
		void sort_options(bool second = false) {
			if (!second) {
				std::sort(options.begin(), options.end(), [](const auto& lhs, const auto& rhs) {
					return ExtraWindow::SortRawStrings(lhs.first, rhs.first);
					});
			}
			else {
				std::sort(options.begin(), options.end(), [](const auto& lhs, const auto& rhs) {
					return ExtraWindow::SortRawStrings(lhs.second, rhs.second);
					});
			}
		}
		std::vector<std::string> do_modal()
		{
			std::vector<std::string> ret;
			CListUInputDlg dlg;
			dlg.m_Caption = caption.c_str();
			for (const auto& [key, value] : options)
			{
				if (value == "")
					dlg.m_CustomStrings.push_back(key.c_str());
				else
					dlg.m_CustomStrings.push_back((key + " - " + value).c_str());
			}
			dlg.DoModal();
			for (const auto& text : dlg.m_selectedTexts)
			{
				for (const auto& [key, value] : options)
				{
					if (value == "")
					{
						if (key == text.m_pchData)
						{
							ret.push_back(key);
							selected_keys.push_back(key);
							selected_values.push_back(value);
							break;
						}
					}
					else
					{
						if (key + " - " + value == text.m_pchData)
						{
							ret.push_back(key);
							selected_keys.push_back(key);
							selected_values.push_back(value);
							break;
						}
					}
				}
			}
			return ret;
		}
	};

	static std::string input_box(std::string message)
	{
		return CInputMessageBox::GetString(message.c_str(), Translations::TranslateOrDefault("LuaConsole.InputBoxTitle", "Please enter")).m_pchData;
	}
	
	static std::string read_input()
	{
		GetWindowText(CLuaConsole::hInputBox, CLuaConsole::Buffer, BUFFER_SIZE);
		return CLuaConsole::Buffer;
	}

	struct TimePoint {
		int year, month, day, hour, minute, second;
		std::chrono::system_clock::time_point time;
	};

	static TimePoint getCurrentTime() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm local_time = *std::localtime(&now_c);

		return TimePoint{
			local_time.tm_year + 1900,
			local_time.tm_mon + 1,
			local_time.tm_mday,
			local_time.tm_hour,
			local_time.tm_min,
			local_time.tm_sec,
			now
		};
	}

	static std::string formatTime(const TimePoint& time) {
		std::ostringstream oss;
		oss << std::setfill('0')
			<< time.year << "-"
			<< std::setw(2) << time.month << "-"
			<< std::setw(2) << time.day << " "
			<< std::setw(2) << time.hour << ":"
			<< std::setw(2) << time.minute << ":"
			<< std::setw(2) << time.second;
		return oss.str();
	}

	static std::string formatTimeInterval(std::chrono::seconds interval) {
		int hours = interval.count() / 3600;
		int minutes = (interval.count() % 3600) / 60;
		int seconds = interval.count() % 60;

		std::ostringstream oss;
		if (hours > 0) {
			oss << hours << "h ";
		}
		oss << minutes << "m " << seconds << "s";
		return oss.str();
	}

	static std::vector<std::string> split_string(std::string s, std::string delimiter = ",") {
		char deli = delimiter[0];
		std::vector<std::string> tokens;
		std::stringstream ss(s);
		std::string token;
		while (getline(ss, token, deli)) {
			tokens.push_back(token);
		}
		return tokens;
	}

	class infantry
	{
	public:
		infantry(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			SubCell = "-1";
			Status = ExtConfigs::DefaultInfantryProperty.Status;
			Tag = ExtConfigs::DefaultInfantryProperty.Tag;
			Facing = ExtConfigs::DefaultInfantryProperty.Facing;
			VeterancyPercentage = ExtConfigs::DefaultInfantryProperty.VeterancyPercentage;
			Group = ExtConfigs::DefaultInfantryProperty.Group;
			IsAboveGround = ExtConfigs::DefaultInfantryProperty.IsAboveGround;
			AutoNORecruitType = ExtConfigs::DefaultInfantryProperty.AutoNORecruitType;
			AutoYESRecruitType = ExtConfigs::DefaultInfantryProperty.AutoYESRecruitType;
			Health = ExtConfigs::DefaultInfantryProperty.Health;
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string SubCell;
		std::string Status;
		std::string Facing;
		std::string Tag;
		std::string VeterancyPercentage;
		std::string Group;
		std::string IsAboveGround;
		std::string AutoNORecruitType;
		std::string AutoYESRecruitType;
		bool operator==(const infantry& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				X == another.X &&
				Y == another.Y &&
				SubCell == another.SubCell &&
				Status == another.Status &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				VeterancyPercentage == another.VeterancyPercentage &&
				Group == another.Group &&
				IsAboveGround == another.IsAboveGround &&
				AutoNORecruitType == another.AutoNORecruitType &&
				AutoYESRecruitType == another.AutoYESRecruitType;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(X, Y))
				return;
			CInfantryData inf;
			inf.House = this->House.c_str();
			inf.TypeID = this->TypeID.c_str();
			inf.Health = this->Health.c_str();
			inf.Y.Format("%d", this->Y);
			inf.X.Format("%d", this->X);
			inf.SubCell = this->SubCell.c_str();
			inf.Status = this->Status.c_str();
			inf.Facing = this->Facing.c_str();
			inf.Tag = this->Tag.c_str();
			inf.VeterancyPercentage = this->VeterancyPercentage.c_str();
			inf.Group = this->Group.c_str();
			inf.IsAboveGround = this->IsAboveGround.c_str();
			inf.AutoNORecruitType = this->AutoNORecruitType.c_str();
			inf.AutoYESRecruitType = this->AutoYESRecruitType.c_str();
			CMapData::Instance->SetInfantryData(&inf, nullptr, nullptr, 0, -1);
			CLuaConsole::needRedraw = true;
		}
		static infantry convert(CInfantryData& obj)
		{
			infantry ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.SubCell = obj.SubCell.m_pchData;
			ret.Status = obj.Status.m_pchData;
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.VeterancyPercentage = obj.VeterancyPercentage.m_pchData;
			ret.Group = obj.Group.m_pchData;
			ret.IsAboveGround = obj.IsAboveGround.m_pchData;
			ret.AutoNORecruitType = obj.AutoNORecruitType.m_pchData;
			ret.AutoYESRecruitType = obj.AutoYESRecruitType.m_pchData;
			return ret;
		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Infantry"); ++i)
			{
				CInfantryData obj;
				CMapData::Instance->GetInfantryData(i, obj);
				if (*this == convert(obj))
				{
					//CINI::CurrentDocument->DeleteKey("Infantry", CINI::CurrentDocument->GetKeyAt("Infantry", i));
					CMapData::Instance->DeleteInfantryData(i);
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateInfantry = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
	};
	
	class unit
	{
	public:
		unit(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			Facing = ExtConfigs::DefaultUnitProperty.Facing;
			Health = ExtConfigs::DefaultUnitProperty.Health;
			Status = ExtConfigs::DefaultUnitProperty.Status;
			Tag = ExtConfigs::DefaultUnitProperty.Tag;
			VeterancyPercentage = ExtConfigs::DefaultUnitProperty.VeterancyPercentage;
			Group = ExtConfigs::DefaultUnitProperty.Group;
			IsAboveGround = ExtConfigs::DefaultUnitProperty.IsAboveGround;
			FollowsIndex = ExtConfigs::DefaultUnitProperty.FollowsIndex;
			AutoNORecruitType = ExtConfigs::DefaultUnitProperty.AutoNORecruitType;
			AutoYESRecruitType = ExtConfigs::DefaultUnitProperty.AutoYESRecruitType;
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string FollowsIndex;
		std::string Status;
		std::string Facing;
		std::string Tag;
		std::string VeterancyPercentage;
		std::string Group;
		std::string IsAboveGround;
		std::string AutoNORecruitType;
		std::string AutoYESRecruitType;

		bool operator==(const unit& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				X == another.X &&
				Y == another.Y &&
				FollowsIndex == another.FollowsIndex &&
				Status == another.Status &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				VeterancyPercentage == another.VeterancyPercentage &&
				Group == another.Group &&
				IsAboveGround == another.IsAboveGround &&
				AutoNORecruitType == another.AutoNORecruitType &&
				AutoYESRecruitType == another.AutoYESRecruitType;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(this->X, this->Y))
				return;
			CUnitData unit;
			unit.House = this->House.c_str();
			unit.TypeID = this->TypeID.c_str();
			unit.Health = this->Health.c_str();
			unit.Y.Format("%d", this->Y);
			unit.X.Format("%d", this->X);
			unit.FollowsIndex = this->FollowsIndex.c_str();
			unit.Status = this->Status.c_str();
			unit.Facing = this->Facing.c_str();
			unit.Tag = this->Tag.c_str();
			unit.VeterancyPercentage = this->VeterancyPercentage.c_str();
			unit.Group = this->Group.c_str();
			unit.IsAboveGround = this->IsAboveGround.c_str();
			unit.AutoNORecruitType = this->AutoNORecruitType.c_str();
			unit.AutoYESRecruitType = this->AutoYESRecruitType.c_str();
			CMapData::Instance->SetUnitData(&unit, nullptr, nullptr, 0, "");
			CLuaConsole::needRedraw = true;
		}
		static unit convert(CUnitData& obj)
		{
			unit ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.FollowsIndex = obj.FollowsIndex.m_pchData;
			ret.Status = obj.Status.m_pchData;
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.VeterancyPercentage = obj.VeterancyPercentage.m_pchData;
			ret.Group = obj.Group.m_pchData;
			ret.IsAboveGround = obj.IsAboveGround.m_pchData;
			ret.AutoNORecruitType = obj.AutoNORecruitType.m_pchData;
			ret.AutoYESRecruitType = obj.AutoYESRecruitType.m_pchData;
			return ret;
		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Units"); ++i)
			{
				CUnitData obj;
				CMapData::Instance->GetUnitData(i, obj);
				if (*this == convert(obj))
				{
					CINI::CurrentDocument->DeleteKey("Units", CINI::CurrentDocument->GetKeyAt("Units", i));
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateUnit = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
	};
	
	class aircraft
	{
	public:
		aircraft(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			Facing = ExtConfigs::DefaultAircraftProperty.Facing;
			Health = ExtConfigs::DefaultAircraftProperty.Health;
			Status = ExtConfigs::DefaultAircraftProperty.Status;
			Tag = ExtConfigs::DefaultAircraftProperty.Tag;
			VeterancyPercentage = ExtConfigs::DefaultAircraftProperty.VeterancyPercentage;
			Group = ExtConfigs::DefaultAircraftProperty.Group;
			AutoNORecruitType = ExtConfigs::DefaultAircraftProperty.AutoNORecruitType;
			AutoYESRecruitType = ExtConfigs::DefaultAircraftProperty.AutoYESRecruitType;
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string Status;
		std::string Facing;
		std::string Tag;
		std::string VeterancyPercentage;
		std::string Group;
		std::string AutoNORecruitType;
		std::string AutoYESRecruitType;

		bool operator==(const aircraft& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				X == another.X &&
				Y == another.Y &&
				Status == another.Status &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				VeterancyPercentage == another.VeterancyPercentage &&
				Group == another.Group &&
				AutoNORecruitType == another.AutoNORecruitType &&
				AutoYESRecruitType == another.AutoYESRecruitType;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(this->X, this->Y))
				return;
			CAircraftData air;
			air.House = this->House.c_str();
			air.TypeID = this->TypeID.c_str();
			air.Health = this->Health.c_str();
			air.Y.Format("%d", this->Y);
			air.X.Format("%d", this->X);
			air.Status = this->Status.c_str();
			air.Facing = this->Facing.c_str();
			air.Tag = this->Tag.c_str();
			air.VeterancyPercentage = this->VeterancyPercentage.c_str();
			air.Group = this->Group.c_str();
			air.AutoNORecruitType = this->AutoNORecruitType.c_str();
			air.AutoYESRecruitType = this->AutoYESRecruitType.c_str();
			CMapData::Instance->SetAircraftData(&air, nullptr, nullptr, 0, "");
			CLuaConsole::needRedraw = true;
		}
		static aircraft convert(CAircraftData& obj)
		{
			aircraft ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.Status = obj.Status.m_pchData;
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.VeterancyPercentage = obj.VeterancyPercentage.m_pchData;
			ret.Group = obj.Group.m_pchData;
			ret.AutoNORecruitType = obj.AutoNORecruitType.m_pchData;
			ret.AutoYESRecruitType = obj.AutoYESRecruitType.m_pchData;
			return ret;
		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Aircraft"); ++i)
			{
				CAircraftData obj;
				CMapData::Instance->GetAircraftData(i, obj);
				if (*this == convert(obj))
				{
					CINI::CurrentDocument->DeleteKey("Aircraft", CINI::CurrentDocument->GetKeyAt("Aircraft", i));
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateAircraft = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
	};
	
	class building
	{
	public:
		building(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			Facing = ExtConfigs::DefaultBuildingProperty.Facing;
			Health = ExtConfigs::DefaultBuildingProperty.Health;
			Tag = ExtConfigs::DefaultBuildingProperty.Tag;
			AISellable = ExtConfigs::DefaultBuildingProperty.AISellable;
			AIRebuildable = ExtConfigs::DefaultBuildingProperty.AIRebuildable;
			PoweredOn = ExtConfigs::DefaultBuildingProperty.PoweredOn;
			Upgrades = ExtConfigs::DefaultBuildingProperty.Upgrades;
			SpotLight = ExtConfigs::DefaultBuildingProperty.SpotLight;
			Upgrade1 = ExtConfigs::DefaultBuildingProperty.Upgrade1;
			Upgrade2 = ExtConfigs::DefaultBuildingProperty.Upgrade2;
			Upgrade3 = ExtConfigs::DefaultBuildingProperty.Upgrade3;
			AIRepairable = ExtConfigs::DefaultBuildingProperty.AIRepairable;
			Nominal = ExtConfigs::DefaultBuildingProperty.Nominal;
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string Facing;
		std::string Tag;
		std::string AISellable;
		std::string AIRebuildable;
		std::string PoweredOn;
		std::string Upgrades;
		std::string SpotLight;
		std::string Upgrade1;
		std::string Upgrade2;
		std::string Upgrade3;
		std::string AIRepairable;
		std::string Nominal;

		bool operator==(const building& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				Y == another.Y &&
				X == another.X &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				AISellable == another.AISellable &&
				AIRebuildable == another.AIRebuildable &&
				PoweredOn == another.PoweredOn &&
				Upgrades == another.Upgrades &&
				SpotLight == another.SpotLight &&
				Upgrade1 == another.Upgrade1 &&
				Upgrade2 == another.Upgrade2 &&
				Upgrade3 == another.Upgrade3 &&
				AIRepairable == another.AIRepairable;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(this->X, this->Y))
				return;

			ppmfc::CString id = CINI::GetAvailableKey("Structures");

			FString value;
			value.Format("%s,%s,%s,%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", 
				this->House, this->TypeID, this->Health, this->Y, this->X,
				this->Facing, this->Tag, this->AISellable, this->AIRebuildable,
				this->PoweredOn, this->Upgrades, this->SpotLight, this->Upgrade1,
				this->Upgrade2, this->Upgrade3, this->AIRepairable, this->Nominal);
				
			CINI::CurrentDocument->WriteString("Structures", id, value);
			CLuaConsole::needRedraw = true;
			CLuaConsole::updateBuilding = true;
		}
		void place_node(bool deleteBuilding = false) const
		{
			if (CMapData::Instance->IsMultiOnly())
				return;
			if (this->X == -1 || this->Y == -1)
				return;
			auto cell = CMapData::Instance->TryGetCellAt(this->X, this->Y);
			if (auto pHouse = CINI::CurrentDocument->GetSection(this->House.c_str()))
			{
				for (int i = 0; i < 1000; ++i)
				{
					FString key;
					key.Format("%03d", i);
					if (!CINI::CurrentDocument->KeyExists(this->House.c_str(), key))
					{
						FString count;
						FString value;
						count.Format("%d", i + 1);
						value.Format("%s,%d,%d", this->TypeID, this->Y, this->X);
						CINI::CurrentDocument->WriteString(this->House.c_str(), "NodeCount", count);
						CINI::CurrentDocument->WriteString(this->House.c_str(), key, value);
						CLuaConsole::updateNode = true;
						CLuaConsole::needRedraw = true;
						if (deleteBuilding && cell->Structure > -1)
						{
							remove();
						}
						break;
					}
				}
			}

		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Structures"); ++i)
			{
				CBuildingData obj;
				CMapDataExt::GetBuildingDataByIniID(i, obj);
				if (*this == convert(obj))
				{
					CINI::CurrentDocument->DeleteKey("Structures", CINI::CurrentDocument->GetKeyAt("Structures", i));
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateBuilding = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
		static building convert(CBuildingData& obj)
		{
			building ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.AISellable = obj.AISellable.m_pchData;
			ret.AIRebuildable = obj.AIRebuildable.m_pchData;
			ret.PoweredOn = obj.PoweredOn.m_pchData;
			ret.Upgrades = obj.Upgrades.m_pchData;
			ret.SpotLight = obj.SpotLight.m_pchData;
			ret.Upgrade1 = obj.Upgrade1.m_pchData;
			ret.Upgrade2 = obj.Upgrade2.m_pchData;
			ret.Upgrade3 = obj.Upgrade3.m_pchData;
			ret.AIRepairable = obj.AIRepairable.m_pchData;
			ret.Nominal = obj.Nominal.m_pchData;
			return ret;
		}
		CBuildingData convert() const
		{
			CBuildingData ret;
			ret.House = House.c_str();
			ret.TypeID = TypeID.c_str();
			ret.Health = Health.c_str();
			ret.Y.Format("%d", Y);
			ret.X.Format("%d", X);
			ret.Facing = Facing.c_str();
			ret.Tag = Tag.c_str();
			ret.AISellable = AISellable.c_str();
			ret.AIRebuildable = AIRebuildable.c_str();
			ret.PoweredOn = PoweredOn.c_str();
			ret.Upgrades = Upgrades.c_str();
			ret.SpotLight = SpotLight.c_str();
			ret.Upgrade1 = Upgrade1.c_str();
			ret.Upgrade2 = Upgrade2.c_str();
			ret.Upgrade3 = Upgrade3.c_str();
			ret.AIRepairable = AIRepairable.c_str();
			ret.Nominal = Nominal.c_str();
			return ret;
		}
	};

	class cell
	{
	public:
		bool IsHidden() const
		{
			return (CMapData::Instance->GetCellAt(X, Y)->IsHidden());
		}
		bool IsMultiSelected() const
		{
			return MultiSelection::IsSelected(X, Y);
		}

		void apply() const
		{
			int pos = CMapData::Instance->GetCoordIndex(this->X, this->Y);
			auto pCell = CMapData::Instance->GetCellAt(pos);
			auto& pCellExt = CMapDataExt::CellDataExts[pos];
			//pCell->Unit = this->Unit;
			//pCell->Infantry[0] = this->Infantry[0];
			//pCell->Infantry[1] = this->Infantry[1];
			//pCell->Infantry[2] = this->Infantry[2];
			//pCell->Aircraft = this->Aircraft;
			//pCell->Structure = this->Structure;
			//pCell->TypeListIndex = this->TypeListIndex;
			//pCell->Terrain = this->Terrain;
			//pCell->TerrainType = this->TerrainType;
			//pCell->Smudge = this->Smudge;
			//pCell->SmudgeType = this->SmudgeType;
			//pCell->Waypoint = this->Waypoint;
			//pCell->BaseNode.BuildingID = this->BuildingID;
			//pCell->BaseNode.BasenodeID = this->BasenodeID;
			//pCell->BaseNode.House = this->House.c_str();
			if (pCellExt.NewOverlay != this->Overlay || pCell->OverlayData != this->OverlayData)
			{
				int olyPos = this->Y + this->X * 512;
				if (olyPos < 262144)
				{
					CMapData::Instance->Overlay[olyPos] = this->Overlay > 0xff ? 0xff : this->Overlay;
					CMapDataExt::NewOverlay[olyPos] = this->Overlay ;
					CMapData::Instance->OverlayData[olyPos] = this->OverlayData;
				}
				CLuaConsole::recalculateOre = true;
			}
			pCell->Overlay = this->Overlay > 0xff ? 0xff : this->Overlay;
			pCellExt.NewOverlay = this->Overlay;
			pCell->OverlayData = this->OverlayData;
			pCell->TileIndex = this->TileIndex;
			//pCell->TileIndexHiPart = this->TileIndexHiPart;
			pCell->TileSubIndex = this->TileSubIndex;
			pCell->Height = this->Height;
			//pCell->IceGrowth = this->IceGrowth;
			//pCell->CellTag = this->CellTag;
			//pCell->Tube = this->Tube;
			//pCell->TubeDataIndex = this->TubeDataIndex;
			//pCell->StatusFlag = this->StatusFlag;
			//pCell->Flag.NotAValidCell = this->NotAValidCell;
			pCell->Flag.IsHiddenCell = this->IsHiddenCell;
			//pCell->Flag.RedrawTerrain = this->RedrawTerrain;
			//pCell->Flag.CliffHack = this->CliffHack;
			//pCell->Flag.AltIndex = this->AltIndex;
			CMapData::Instance->UpdateMapPreviewAt(this->X, this->Y);
			CLuaConsole::needRedraw = true;
		}

		short X;
		short Y;
		short Unit;
		short Infantry[3];
		short Aircraft;
		short Structure;
		short TypeListIndex;
		short Terrain;
		int TerrainType;
		short Smudge;
		int SmudgeType;
		short Waypoint;

		int BuildingID;
		int BasenodeID;
		std::string House;

		unsigned short Overlay;
		unsigned char OverlayData;
		unsigned short TileIndex;
		unsigned short TileIndexHiPart;
		unsigned char TileSubIndex;
		unsigned char Height;
		unsigned char IceGrowth;
		short CellTag;
		short Tube;
		unsigned char TubeDataIndex;
		unsigned char StatusFlag;
		bool NotAValidCell;
		bool IsHiddenCell;
		bool RedrawTerrain;
		bool CliffHack;
		char AltIndex;


	};

	class tile 
	{
	public:
		bool Valid = false;
		short TileIndex;
		short TileSubIndex;
		short Height;
		short AltCount;
		short RelativeX;
		short RelativeY;

		short TileSet;
		short RampType;
		std::string LandType;

		static tile get_tile_block(int tileIdx, int tileSubIdx)
		{
			tile ret{};
			tileIdx = CMapDataExt::GetSafeTileIndex(tileIdx);
			ret.TileIndex = tileIdx;
			ret.TileSubIndex = tileSubIdx;
			if (tileIdx > CMapDataExt::TileDataCount)
				return ret;
			const auto& tileData = CMapDataExt::TileData[tileIdx];
			ret.AltCount = tileData.AltTypeCount;
			if (tileSubIdx > tileData.TileBlockCount)
				return ret;
			const auto& tileBlock = tileData.TileBlockDatas[tileSubIdx];
			if (tileBlock.ImageData == NULL)
				return ret;
			ret.Height = tileBlock.Height;
			ret.RelativeY = tileSubIdx % tileData.Width;
			ret.RelativeX = tileSubIdx / tileData.Width;
			ret.TileSet = tileData.TileSet;
			ret.RampType = tileBlock.RampType;

			if (tileBlock.TerrainType == 0x0)
				ret.LandType = "clear";
			else if (tileBlock.TerrainType == 0xd)
				ret.LandType = "clear";
			else if (tileBlock.TerrainType == 0xb)
				ret.LandType = "road";
			else if (tileBlock.TerrainType == 0xc)
				ret.LandType = "road";
			else if (tileBlock.TerrainType == 0x9)
				ret.LandType = "water";
			else if (tileBlock.TerrainType == 0x7)
				ret.LandType = "rock";
			else if (tileBlock.TerrainType == 0x8)
				ret.LandType = "rock";
			else if (tileBlock.TerrainType == 0xf)
				ret.LandType = "rock";
			else if (tileBlock.TerrainType == 0xa)
				ret.LandType = "beach";
			else if (tileBlock.TerrainType == 0xe)
				ret.LandType = "rough";
			else if (tileBlock.TerrainType == 0x1)
				ret.LandType = "ice";
			else if (tileBlock.TerrainType == 0x2)
				ret.LandType = "ice";
			else if (tileBlock.TerrainType == 0x3)
				ret.LandType = "ice";
			else if (tileBlock.TerrainType == 0x4)
				ret.LandType = "ice";
			else if (tileBlock.TerrainType == 0x6)
				ret.LandType = "railroad";
			else if (tileBlock.TerrainType == 0x5)
				ret.LandType = "tunnel";

			ret.Valid = true;
			return ret;
		}
		static std::vector<tile> get_whole_tile(int tileIdx)
		{
			std::vector<tile> ret;
			if (tileIdx > CMapDataExt::TileDataCount)
				return ret;
			const auto& tileData = CMapDataExt::TileData[tileIdx];
			for (int i = 0; i < tileData.Width * tileData.Height; ++i)
			{
				auto&& t = get_tile_block(tileIdx, i);
				if (t.Valid)
				{
					ret.push_back(t);
				}
			}
			return ret;
		}
	};

	class snapshot
	{
	public:
		TimePoint savedTime;
		std::string fileName;
		std::map<FString, std::map<FString, FString>> INI;
		unsigned short Overlay[0x40000];
		unsigned char OverlayData[0x40000];
		std::vector<CellData> CellDatas;
	};
	static std::map<int, snapshot> snapshots;

	static std::string GetAvailableIndex()
	{
		auto& ini = CINI::CurrentDocument;
		int n = 1000000;

		std::unordered_set<std::string> usedIDs;

		for (const auto& sec : { "ScriptTypes", "TaskForces", "TeamTypes" }) {
			if (auto pSection = ini->GetSection(sec)) {
				for (const auto& [k, v] : pSection->GetEntities()) {
					usedIDs.insert(v.m_pchData);
				}
			}
		}
		for (const auto& sec : { "Triggers", "Events", "Tags", "Actions", "AITriggerTypes" }) {
			if (auto pSection = ini->GetSection(sec)) {
				for (const auto& [k, v] : pSection->GetEntities()) {
					usedIDs.insert(k.m_pchData);
				}
			}
		}

		char idBuffer[9];
		while (true)
		{
			std::sprintf(idBuffer, "%08d", n);
			std::string id(idBuffer);

			if (usedIDs.find(id) == usedIDs.end() 
				&& UsedINIIndices.find(id) == UsedINIIndices.end() 
				&& !ini->SectionExists(id.c_str())) {
				return id;
			}

			n++;
		}

		return "";
	}

	struct tag
	{
		std::string ID;
		std::string Name;
		std::string RepeatType;
	};

	class trigger
	{
	public:
		std::string ID;
		std::string Name;
		std::string House;
		std::vector<tag> Tags;
		std::string AttachedTrigger;
		std::string Obsolete;
		bool Disabled;
		bool EasyEnabled;
		bool MediumEnabled;
		bool HardEnabled;
		std::vector<std::string> Events;
		std::vector<std::string> Actions;

		trigger()
		{
			CLuaConsole::Lua.collect_garbage();
			ID = GetAvailableIndex();
			Name = "New Trigger";
			House = "Americans";
			AttachedTrigger = "<none>";
			Obsolete = "0";
			Disabled = false;
			EasyEnabled = true;
			MediumEnabled = true;
			HardEnabled = true;
			UsedINIIndices.insert(ID);
		}
		trigger(std::string id)
			: ID(id)
		{
			Name = "New Trigger";
			House = "Americans";
			AttachedTrigger = "<none>";
			Obsolete = "0";
			Disabled = false;
			EasyEnabled = true;
			MediumEnabled = true;
			HardEnabled = true;
			UsedINIIndices.insert(id);
		}
		~trigger() {
			release_id();
		}
		void add_tag(std::string id, std::string name, int repeat)
		{
			if (id == "")
				id = GetAvailableIndex();
			if (name == "")
				name = Name + " 1";
			auto& tag = Tags.emplace_back();
			tag.ID = id;
			tag.Name = name;
			tag.RepeatType = repeat == 2 ? "2" : repeat == 1 ? "1" : "0";
			UsedINIIndices.insert(id);
		}
		void add_event(std::string value)
		{
			auto&& splits = split_string(value);
			if (splits.size() < 3 || splits.size() > 4 || (splits.size() == 4 && splits[1] != "2"))
			{
				write_lua_console("Ill-formed event " + value);
				return;
			}
			Events.push_back(value);
		}
		void add_action(std::string value)
		{
			auto&& splits = split_string(value);
			if (splits.size() != 8)
			{
				write_lua_console("Ill-formed action " + value);
				return;
			}
			Actions.push_back(value);
		}
		void delete_tag(int index, bool removeIni)
		{
			index--;
			if (0 <= index && index < Tags.size())
			{
				if (removeIni)
					CINI::CurrentDocument->DeleteKey("Tags", Tags[index].ID.c_str());
				UsedINIIndices.erase(Tags[index].ID);
				Tags.erase(Tags.begin() + index);
			}
		}		
		void delete_tags(bool removeIni)
		{
			for (int i = 0; i < Tags.size(); ++i)
			{
				if (removeIni)
					CINI::CurrentDocument->DeleteKey("Tags", Tags[i].ID.c_str());
				UsedINIIndices.erase(Tags[i].ID);
			}
			Tags.clear();
		}
		void delete_event(int index)
		{
			index--;
			if (0 <= index && index < Events.size())
				Events.erase(Events.begin() + index);
		}
		void delete_action(int index)
		{
			index--;
			if (0 <= index && index < Actions.size())
				Actions.erase(Actions.begin() + index);
		}
		void change_id(std::string id)
		{
			UsedINIIndices.erase(ID);
			UsedINIIndices.insert(id);
			ID = id;
		}
		void apply()
		{
			FString trigger;
			trigger.Format("%s,%s,%s,%s,%s,%s,%s,%s", House.c_str(), AttachedTrigger.c_str(), Name.c_str(),
				Disabled ? "1" : "0", EasyEnabled ? "1" : "0", MediumEnabled ? "1" : "0", HardEnabled ? "1" : "0", Obsolete.c_str());
			CINI::CurrentDocument->WriteString("Triggers", ID.c_str(), trigger);

			for (const auto& tag : Tags)
			{
				FString tagStr;
				tagStr.Format("%s,%s,%s", tag.RepeatType.c_str(), tag.Name.c_str(), ID.c_str());
				CINI::CurrentDocument->WriteString("Tags", tag.ID.c_str(), tagStr);
			}

			FString events;
			events.Format("%d", Events.size());
			for (const auto& e : Events)
			{
				events += ",";
				events += e.c_str();
			}
			CINI::CurrentDocument->WriteString("Events", ID.c_str(), events);

			FString actions;
			actions.Format("%d", Actions.size());
			for (const auto& a : Actions)
			{
				actions += ",";
				actions += a.c_str();
			}
			FString validate;
			validate.Format("%s=%s", ID.c_str(), actions);
			if (validate.GetLength() >= 512)
				write_lua_console(std::format("Warn: length of action {} exceeds 512.", ID));

			CINI::CurrentDocument->WriteString("Actions", ID.c_str(), actions);
			CMapDataExt::AddTrigger(ID.c_str());
			UsedINIIndices.insert(ID);
			CLuaConsole::updateTrigger = true;
		}
		void delete_trigger_self(bool keepTag)
		{
			CINI::CurrentDocument->DeleteKey("Triggers", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Events", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Actions", ID.c_str());
			if (!keepTag)
			{
				for (const auto& tag : Tags)
				{
					CINI::CurrentDocument->DeleteKey("Tags", tag.ID.c_str());
				}
			}

			CMapDataExt::DeleteTrigger(ID.c_str());
			UsedINIIndices.erase(ID);
			CLuaConsole::updateTrigger = true;
		}
		static void delete_trigger(std::string ID, bool keepTag)
		{
			CINI::CurrentDocument->DeleteKey("Triggers", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Events", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Actions", ID.c_str());
			if (!keepTag)
			{
				std::vector<FString> keys;
				if (auto pSection = CINI::CurrentDocument->GetSection("Tags"))
				{
					for (const auto& [key, value] : pSection->GetEntities())
					{
						auto&& atoms = STDHelpers::SplitString(value, 2);
						if (atoms[2] == ID.c_str())
							keys.push_back(key);
					}
				}
				for (auto& key : keys)
					CINI::CurrentDocument->DeleteKey("Tags", key);
			}
			CMapDataExt::DeleteTrigger(ID.c_str());
			UsedINIIndices.erase(ID);
			CLuaConsole::updateTrigger = true;
		}
		static void delete_tag_static(std::string ID, bool keepTrigger)
		{
			if (!keepTrigger)
			{
				auto&& atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString("Tags", ID.c_str()), 2);
				CINI::CurrentDocument->DeleteKey("Triggers", atoms[2]);
				CINI::CurrentDocument->DeleteKey("Events", atoms[2]);
				CINI::CurrentDocument->DeleteKey("Actions", atoms[2]);
				CMapDataExt::DeleteTrigger(atoms[2]);
			}
			CINI::CurrentDocument->DeleteKey("Tags", ID.c_str());
			CLuaConsole::updateTrigger = true;
		}
		static sol::object get_trigger(std::string ID)
		{
			if (auto t = CMapDataExt::GetTrigger(ID.c_str()))
			{
				trigger ret{ t->ID };
				ret.Name = t->Name;
				ret.House = t->House;
				ret.AttachedTrigger = t->AttachedTrigger;
				ret.Obsolete = "0";
				ret.Disabled = t->Disabled;
				ret.EasyEnabled = t->EasyEnabled;
				ret.MediumEnabled = t->MediumEnabled;
				ret.HardEnabled = t->HardEnabled;
				if (auto pSection = CINI::CurrentDocument->GetSection("Tags"))
				{
					for (const auto& [key, value] : pSection->GetEntities())
					{
						auto&& atoms = STDHelpers::SplitString(value, 2);
						if (atoms[2] == ID.c_str())
						{
							auto& tag = ret.Tags.emplace_back();
							tag.ID = key.m_pchData;
							tag.Name = atoms[1].m_pchData;
							tag.RepeatType = atoms[0].m_pchData;
						}
					}
				}
				for (const auto& eve : t->Events)
				{
					auto& str = ret.Events.emplace_back();
					str += eve.EventNum;
					int loopCount = eve.P3Enabled ? 3 : 2;
					for (int i = 0; i < loopCount; ++i)
					{
						str += ",";
						str += eve.Params[i];
					}
				}
				for (const auto& act : t->Actions)
				{
					auto& str = ret.Actions.emplace_back();
					str += act.ActionNum;
					for (int i = 0; i < 7; ++i)
					{
						str += ",";
						str += act.Params[i];
					}
				}
				return sol::make_object(CLuaConsole::Lua, ret);
			}
			else
			{
				write_lua_console("Cannot find trigger " + ID);
			}
			return sol::make_object(CLuaConsole::Lua, sol::nil);
		}
		void release_id() const {
			UsedINIIndices.erase(ID);
		}

	};

	class team
	{
	public:
		std::string ID;
		std::string Name = "New Teamtype";
		std::string House = "Americans";
		std::string Taskforce = "";
		std::string Script = "";
		std::string Tag = "";
		std::string VeteranLevel = "1";
		std::string Priority = "5";
		std::string Max = "5";
		std::string Techlevel = "0";
		std::string TransportWaypoint = "";
		std::string Group = "-1";
		std::string Waypoint = "0";
		std::string MindControlDecision = "0";
		bool Full = false;
		bool Whiner = false;
		bool Droppod = false;
		bool Suicide = false;
		bool Loadable = false;
		bool Prebuild = false;
		bool Annoyance = false;
		bool IonImmune = false;
		bool Recruiter = false;
		bool Reinforce = false;
		bool Aggressive = false;
		bool Autocreate = false;
		bool GuardSlower = false;
		bool OnTransOnly = false;
		bool AvoidThreats = false;
		bool LooseRecruit = false;
		bool IsBaseDefense = false;
		bool UseTransportOrigin = false;
		bool OnlyTargetHouseEnemy = false;
		bool TransportsReturnOnUnload = false;
		bool AreTeamMembersRecruitable = false;

		team()
		{
			CLuaConsole::Lua.collect_garbage();
			ID = GetAvailableIndex();
			UsedINIIndices.insert(ID);
		}
		team(std::string id)
		{
			ID = id;
			UsedINIIndices.insert(ID);
		}
		~team() {
			release_id();
		}
		void change_id(std::string id)
		{
			UsedINIIndices.erase(ID);
			UsedINIIndices.insert(id);
			ID = id;
		}
		void release_id() const {
			UsedINIIndices.erase(ID);
		}
		static sol::object get_team(std::string id)
		{
			bool found = false;
			if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == id.c_str() && CINI::CurrentDocument->SectionExists(value))
					{
						found = true;
						break;
					}
				}
			}
			if (found)
			{
				team ret;
				ret.ID = id;
				ret.Name = CINI::CurrentDocument->GetString(id.c_str(), "Name", "New Teamtype").m_pchData;
				ret.House = CINI::CurrentDocument->GetString(id.c_str(), "House").m_pchData;
				ret.Taskforce = CINI::CurrentDocument->GetString(id.c_str(), "TaskForce").m_pchData;
				ret.Script = CINI::CurrentDocument->GetString(id.c_str(), "Script").m_pchData;
				ret.Tag = CINI::CurrentDocument->GetString(id.c_str(), "Tag").m_pchData;
				ret.VeteranLevel = CINI::CurrentDocument->GetString(id.c_str(), "VeteranLevel").m_pchData;
				ret.Priority = CINI::CurrentDocument->GetString(id.c_str(), "Priority").m_pchData;
				ret.Max = CINI::CurrentDocument->GetString(id.c_str(), "Max").m_pchData;
				ret.Techlevel = CINI::CurrentDocument->GetString(id.c_str(), "TechLevel").m_pchData;
				if (CINI::CurrentDocument->KeyExists(id.c_str(), "TransportWaypoint"))
					ret.TransportWaypoint = STDHelpers::StringToWaypointStr(CINI::CurrentDocument->GetString(id.c_str(), "TransportWaypoint")).m_pchData;
				ret.Group = CINI::CurrentDocument->GetString(id.c_str(), "Group").m_pchData;
				ret.Waypoint = STDHelpers::StringToWaypointStr(CINI::CurrentDocument->GetString(id.c_str(), "Waypoint")).m_pchData;
				ret.MindControlDecision = CINI::CurrentDocument->GetString(id.c_str(), "MindControlDecision").m_pchData;
				ret.Full = CINI::CurrentDocument->GetBool(id.c_str(), "Full");
				ret.Whiner = CINI::CurrentDocument->GetBool(id.c_str(), "Whiner");
				ret.Droppod = CINI::CurrentDocument->GetBool(id.c_str(), "Droppod");
				ret.Suicide = CINI::CurrentDocument->GetBool(id.c_str(), "Suicide");
				ret.Loadable = CINI::CurrentDocument->GetBool(id.c_str(), "Loadable");
				ret.Prebuild = CINI::CurrentDocument->GetBool(id.c_str(), "Prebuild");
				ret.Annoyance = CINI::CurrentDocument->GetBool(id.c_str(), "Annoyance");
				ret.IonImmune = CINI::CurrentDocument->GetBool(id.c_str(), "IonImmune");
				ret.Recruiter = CINI::CurrentDocument->GetBool(id.c_str(), "Recruiter");
				ret.Reinforce = CINI::CurrentDocument->GetBool(id.c_str(), "Reinforce");
				ret.Aggressive = CINI::CurrentDocument->GetBool(id.c_str(), "Aggressive");
				ret.Autocreate = CINI::CurrentDocument->GetBool(id.c_str(), "Autocreate");
				ret.GuardSlower = CINI::CurrentDocument->GetBool(id.c_str(), "GuardSlower");
				ret.OnTransOnly = CINI::CurrentDocument->GetBool(id.c_str(), "OnTransOnly");
				ret.AvoidThreats = CINI::CurrentDocument->GetBool(id.c_str(), "AvoidThreats");
				ret.LooseRecruit = CINI::CurrentDocument->GetBool(id.c_str(), "LooseRecruit");
				ret.IsBaseDefense = CINI::CurrentDocument->GetBool(id.c_str(), "IsBaseDefense");
				ret.UseTransportOrigin = CINI::CurrentDocument->GetBool(id.c_str(), "UseTransportOrigin");
				ret.OnlyTargetHouseEnemy = CINI::CurrentDocument->GetBool(id.c_str(), "OnlyTargetHouseEnemy");
				ret.TransportsReturnOnUnload = CINI::CurrentDocument->GetBool(id.c_str(), "TransportsReturnOnUnload");
				ret.AreTeamMembersRecruitable = CINI::CurrentDocument->GetBool(id.c_str(), "AreTeamMembersRecruitable");

				return sol::make_object(CLuaConsole::Lua, ret);
			}
			else
			{
				write_lua_console("Cannot find team " + id);
			}
			return sol::make_object(CLuaConsole::Lua, sol::nil);
		}
		void apply()
		{
			if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == ID.c_str())
					{
						CINI::CurrentDocument->DeleteKey("TeamTypes", key);
						break;
					}
				}
			}
			CINI::CurrentDocument->WriteString("TeamTypes", CINI::GetAvailableKey("TeamTypes"), ID.c_str());

			CINI::CurrentDocument->WriteString(ID.c_str(), "Name", Name.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "House", House.c_str());
			if (Taskforce == "")
				CINI::CurrentDocument->DeleteKey(ID.c_str(), "TaskForce");
			else
				CINI::CurrentDocument->WriteString(ID.c_str(), "TaskForce", Taskforce.c_str());
			if (Script == "")
				CINI::CurrentDocument->DeleteKey(ID.c_str(), "Script");
			else
				CINI::CurrentDocument->WriteString(ID.c_str(), "Script", Script.c_str());
			if (Tag == "")
				CINI::CurrentDocument->DeleteKey(ID.c_str(), "Tag");
			else
				CINI::CurrentDocument->WriteString(ID.c_str(), "Tag", Tag.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "VeteranLevel", VeteranLevel.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "Priority", Priority.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "Max", Max.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "TechLevel", Techlevel.c_str());
			if (TransportWaypoint == "")
			{
				UseTransportOrigin = false;
				CINI::CurrentDocument->DeleteKey(ID.c_str(), "TransportWaypoint");
			}
			else
			{
				UseTransportOrigin = true;
				if (STDHelpers::IsNumber(TransportWaypoint.c_str()))
					CINI::CurrentDocument->WriteString(ID.c_str(), "TransportWaypoint", STDHelpers::WaypointToString(atoi(TransportWaypoint.c_str())));
				else
					CINI::CurrentDocument->WriteString(ID.c_str(), "TransportWaypoint", TransportWaypoint.c_str());
			}
			CINI::CurrentDocument->WriteString(ID.c_str(), "Group", Group.c_str());
			if (STDHelpers::IsNumber(Waypoint.c_str()))
				CINI::CurrentDocument->WriteString(ID.c_str(), "Waypoint", STDHelpers::WaypointToString(atoi(Waypoint.c_str())));
			else
				CINI::CurrentDocument->WriteString(ID.c_str(), "Waypoint", Waypoint.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "MindControlDecision", MindControlDecision.c_str());

			CINI::CurrentDocument->WriteBool(ID.c_str(), "Full", Full);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Whiner", Whiner);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Droppod", Droppod);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Suicide", Suicide);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Loadable", Loadable);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Prebuild", Prebuild);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Annoyance", Annoyance);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "IonImmune", IonImmune);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Recruiter", Recruiter);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Reinforce", Reinforce);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Aggressive", Aggressive);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "Autocreate", Autocreate);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "GuardSlower", GuardSlower);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "OnTransOnly", OnTransOnly);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "AvoidThreats", AvoidThreats);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "LooseRecruit", LooseRecruit);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "IsBaseDefense", IsBaseDefense);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "UseTransportOrigin", UseTransportOrigin);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "OnlyTargetHouseEnemy", OnlyTargetHouseEnemy);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "TransportsReturnOnUnload", TransportsReturnOnUnload);
			CINI::CurrentDocument->WriteBool(ID.c_str(), "AreTeamMembersRecruitable", AreTeamMembersRecruitable);

			UsedINIIndices.insert(ID);
			CLuaConsole::updateTeam = true;
		}
		void delete_team_self() const
		{
			CINI::CurrentDocument->DeleteSection(ID.c_str());
			if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == ID.c_str())
					{
						CINI::CurrentDocument->DeleteKey("TeamTypes", key);
						break;
					}
				}
			}
			UsedINIIndices.erase(ID);
			CLuaConsole::updateTeam = true;
		}
		static void delete_team(std::string id)
		{
			CINI::CurrentDocument->DeleteSection(id.c_str());
			if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == id.c_str())
					{
						CINI::CurrentDocument->DeleteKey("TeamTypes", key);
						break;
					}
				}
			}
			UsedINIIndices.erase(id);
			CLuaConsole::updateTeam = true;
		}
	};

	class task_force
	{
	public:
		std::string ID;
		std::string Name = "New task force";
		std::string Group = "-1";
		std::vector<int> Numbers;
		std::vector<std::string> Units;

		task_force()
		{
			CLuaConsole::Lua.collect_garbage();
			ID = GetAvailableIndex();
			UsedINIIndices.insert(ID);
		}
		task_force(std::string id)
		{
			ID = id;
			UsedINIIndices.insert(ID);
		}
		~task_force() {
			release_id();
		}
		void change_id(std::string id)
		{
			UsedINIIndices.erase(ID);
			UsedINIIndices.insert(id);
			ID = id;
		}
		void release_id() const {
			UsedINIIndices.erase(ID);
		}
		void add_number(int num, std::string obj)
		{
			if (Units.size() >= 6)
			{
				write_lua_console(std::format("Task force {} exceeds unit limit, abort.", ID));
				return;
			}
			for (const auto& unit : Units)
			{
				if (obj == unit)
				{
					write_lua_console(std::format("Duplicate unit {} for task force {}, abort.", obj, ID));
					return;
				}
			}
			Numbers.push_back(num);
			Units.push_back(obj);
		}
		void delete_number(int index)
		{
			index--;
			if (0 <= index && index < Numbers.size())
			{
				Numbers.erase(Numbers.begin() + index);
				Units.erase(Units.begin() + index);
			}
		}
		static sol::object get_task_force(std::string id)
		{
			bool found = false;
			if (auto pSection = CINI::CurrentDocument->GetSection("TaskForces"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == id.c_str() && CINI::CurrentDocument->SectionExists(value))
					{
						found = true;
						break;
					}
				}
			}
			if (found)
			{
				task_force ret;
				ret.ID = id;
				ret.Name = CINI::CurrentDocument->GetString(id.c_str(), "Name", "New task force").m_pchData;
				ret.Group = CINI::CurrentDocument->GetString(id.c_str(), "Group", "-1").m_pchData;
				FString key;
				for (int i = 0; i < 6; ++i)
				{
					key.Format("%d", i);
					if (CINI::CurrentDocument->KeyExists(id.c_str(), key))
					{
						auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString(id.c_str(), key, "1,E1"), 1);
						ret.Numbers.push_back(atoi(atoms[0]));
						ret.Units.push_back(atoms[1].m_pchData);
					}
				}
				return sol::make_object(CLuaConsole::Lua, ret);
			}
			else
			{
				write_lua_console("Cannot find task force " + id);
			}
			return sol::make_object(CLuaConsole::Lua, sol::nil);
		}
		void apply() const
		{
			if (auto pSection = CINI::CurrentDocument->GetSection("TaskForces"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == ID.c_str())
					{
						CINI::CurrentDocument->DeleteKey("TaskForces", key);
						break;
					}
				}
			}
			CINI::CurrentDocument->WriteString("TaskForces", CINI::GetAvailableKey("TaskForces"), ID.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "Name", Name.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "Group", Group.c_str());
			FString key;
			FString value;
			for (int i = 0; i < 6; ++i)
			{
				key.Format("%d", i);
				CINI::CurrentDocument->DeleteKey(ID.c_str(), key);
			}
			for (int i = 0; i < Numbers.size(); ++i)
			{
				key.Format("%d", i);
				value.Format("%d,%s", Numbers[i], Units[i].c_str());
				CINI::CurrentDocument->WriteString(ID.c_str(), key, value);
			}
			UsedINIIndices.insert(ID);
			CLuaConsole::updateTaskforce = true;
		}
		void delete_task_force_self() const
		{
			CINI::CurrentDocument->DeleteSection(ID.c_str());
			if (auto pSection = CINI::CurrentDocument->GetSection("TaskForces"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == ID.c_str())
					{
						CINI::CurrentDocument->DeleteKey("TaskForces", key);
						break;
					}
				}
			}
			UsedINIIndices.erase(ID);
			CLuaConsole::updateTaskforce = true;
		}
		static void delete_task_force(std::string id)
		{
			CINI::CurrentDocument->DeleteSection(id.c_str());
			if (auto pSection = CINI::CurrentDocument->GetSection("TaskForces"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == id.c_str())
					{
						CINI::CurrentDocument->DeleteKey("TaskForces", key);
						break;
					}
				}
			}
			UsedINIIndices.erase(id);
			CLuaConsole::updateTaskforce = true;
		}
	};

	class script
	{
	public:
		std::string ID;
		std::string Name = "New script";
		std::vector<int> Actions;
		std::vector<int> Params;

		script()
		{
			CLuaConsole::Lua.collect_garbage();
			ID = GetAvailableIndex();
			UsedINIIndices.insert(ID);
		}
		script(std::string id)
		{
			ID = id;
			UsedINIIndices.insert(ID);
		}
		~script() {
			release_id();
		}
		void change_id(std::string id)
		{
			UsedINIIndices.erase(ID);
			UsedINIIndices.insert(id);
			ID = id;
		}
		void release_id() const {
			UsedINIIndices.erase(ID);
		}
		void add_action(int act, int par)
		{
			if (Actions.size() >= 50)
			{
				write_lua_console(std::format("Script {} exceeds action limit, abort.", ID));
				return;
			}
			Actions.push_back(act);
			Params.push_back(par);
		}		
		void delete_action(int index)
		{
			index--;
			if (0 <= index && index < Actions.size())
			{
				Actions.erase(Actions.begin() + index);
				Params.erase(Params.begin() + index);
			}
		}
		static sol::object get_script(std::string id)
		{
			bool found = false;
			if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == id.c_str() && CINI::CurrentDocument->SectionExists(value))
					{
						found = true;
						break;
					}
				}
			}
			if (found)
			{
				script ret;
				ret.ID = id;
				ret.Name = CINI::CurrentDocument->GetString(id.c_str(), "Name", "New script").m_pchData;
				FString key;
				for (int i = 0; i < 50; ++i)
				{
					key.Format("%d", i);
					if (CINI::CurrentDocument->KeyExists(id.c_str(), key))
					{
						auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString(id.c_str(), key, "0,0"), 1);
						ret.Actions.push_back(atoi(atoms[0]));
						ret.Params.push_back(atoi(atoms[1]));
					}
				}
				return sol::make_object(CLuaConsole::Lua, ret);
			}
			else
			{
				write_lua_console("Cannot find script " + id);
			}
			return sol::make_object(CLuaConsole::Lua, sol::nil);
		}
		void apply() const
		{
			if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == ID.c_str())
					{
						CINI::CurrentDocument->DeleteKey("ScriptTypes", key);
						break;
					}
				}
			}
			CINI::CurrentDocument->WriteString("ScriptTypes", CINI::GetAvailableKey("ScriptTypes"), ID.c_str());
			CINI::CurrentDocument->WriteString(ID.c_str(), "Name", Name.c_str());
			FString key;
			FString value;
			for (int i = 0; i < 50; ++i)
			{
				key.Format("%d", i);
				CINI::CurrentDocument->DeleteKey(ID.c_str(), key);
			}
			for (int i = 0; i < Actions.size(); ++i)
			{
				key.Format("%d", i);
				value.Format("%d,%d", Actions[i], Params[i]);
				CINI::CurrentDocument->WriteString(ID.c_str(), key, value);
			}
			UsedINIIndices.insert(ID);
			CLuaConsole::updateScript = true;
		}
		void delete_script_self() const
		{
			CINI::CurrentDocument->DeleteSection(ID.c_str());
			if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == ID.c_str())
					{
						CINI::CurrentDocument->DeleteKey("ScriptTypes", key);
						break;
					}
				}
			}	
			UsedINIIndices.erase(ID);
			CLuaConsole::updateScript = true;
		}
		static void delete_script(std::string id)
		{
			CINI::CurrentDocument->DeleteSection(id.c_str());
			if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					if (value == id.c_str())
					{
						CINI::CurrentDocument->DeleteKey("ScriptTypes", key);
						break;
					}
				}
			}	
			UsedINIIndices.erase(id);
			CLuaConsole::updateScript = true;
		}
	};

	class ai_trigger
	{
	public:
		std::string ID;
		std::string Name = "New AI Trigger";
		std::string Team1 = "<none>";
		std::string House = "<all>";
		std::string TechLevel = "0";
		std::string ConditionType = "-1";
		std::string ComparisonObject = "<none>";
		int Comparators[8] = { 0 };
		double InitialWeight = 50.0;
		double MinWeight = 30.0;
		double MaxWeight = 50.0;
		bool IsForSkirmish = true;
		std::string unused = "0";
		std::string Side = "1";
		bool IsBaseDefense = false;
		std::string Team2 = "<none>";
		bool EnabledInE = true;
		bool EnabledInM = true;
		bool EnabledInH = true;
		std::string Comparator = "0";
		int Amount = 0;

		bool Enabled = true;

		ai_trigger()
		{
			CLuaConsole::Lua.collect_garbage();
			ID = GetAvailableIndex();
			UsedINIIndices.insert(ID);
		}
		ai_trigger(std::string id)
		{
			ID = id;
			UsedINIIndices.insert(ID);
		}
		~ai_trigger() {
			release_id();
		}
		static sol::object get_ai_trigger(std::string id)
		{	
			auto atoms = STDHelpers::SplitString(CINI::CurrentDocument().GetString("AITriggerTypes", id.c_str()));
			if (atoms.size() >= 18)
			{
				ai_trigger ret(id);
				ret.ID = id;
				ret.Name = atoms[0].m_pchData;
				ret.Team1 = atoms[1].m_pchData;
				ret.House = atoms[2].m_pchData;
				ret.TechLevel = atoms[3].m_pchData;
				ret.ConditionType = atoms[4].m_pchData;
				ret.ComparisonObject = atoms[5].m_pchData;
				for (int i = 0; i < 8; i++) {
					ret.Comparators[i] = ReadComparator(atoms[6], i);
				}
				ret.InitialWeight = std::atof(atoms[7]);
				ret.MinWeight = std::atof(atoms[8]);
				ret.MaxWeight = std::atof(atoms[9]);
				ret.IsForSkirmish = atoms[10] == "0" ? false : true;
				ret.unused = atoms[11].m_pchData;
				ret.Side = atoms[12].m_pchData;
				ret.IsBaseDefense = atoms[13] == "0" ? false : true;
				ret.Team2 = atoms[14].m_pchData;
				ret.EnabledInE = atoms[15] == "0" ? false : true;
				ret.EnabledInM = atoms[16] == "0" ? false : true;
				ret.EnabledInH = atoms[17] == "0" ? false : true;
				ret.Amount = ret.Comparators[0];
				ret.Comparator = std::to_string(ret.Comparators[1]);

				ret.Enabled = CINI::CurrentDocument().GetBool("AITriggerTypesEnable", id.c_str());
				return sol::make_object(CLuaConsole::Lua, ret);
			}
			else
			{
				write_lua_console("Cannot find AI trigger " + id);
			}
			return sol::make_object(CLuaConsole::Lua, sol::nil);
		}
		void apply()
		{
			Comparators[0] = Amount;
			if (Comparator == "<" || Comparator == "0")
				Comparators[1] = 0;
			else if (Comparator == "<=" || Comparator == "1")
				Comparators[1] = 1;
			else if (Comparator == "==" || Comparator == "=" || Comparator == "2")
				Comparators[1] = 2;
			else if (Comparator == ">=" || Comparator == "3")
				Comparators[1] = 3;
			else if (Comparator == ">" || Comparator == "4")
				Comparators[1] = 4;
			else if (Comparator == "!=" || Comparator == "~=" || Comparator == "5")
				Comparators[1] = 5;
			else
				Comparators[1] = 0;
			FString value;
			FString comparator = "";
			for (int i = 0; i < 8; i++) {
				comparator += SaveComparator(Comparators[i]);
			}
			std::ostringstream oss;
			oss.precision(6);
			oss << std::fixed << InitialWeight;
			std::string initial = oss.str();
			oss.str("");
			oss.precision(6);
			oss << std::fixed << MinWeight;
			std::string min = oss.str();
			oss.str("");
			oss.precision(6);
			oss << std::fixed << MaxWeight;
			std::string max = oss.str();
			oss.str("");

			value.Format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
				Name.c_str(),
				Team1.c_str(),
				House.c_str(),
				TechLevel.c_str(),
				ConditionType.c_str(),
				ComparisonObject.c_str(),
				comparator,
				initial.c_str(),
				min.c_str(),
				max.c_str(),
				IsForSkirmish ? "1" : "0",
				unused.c_str(),
				Side.c_str(),
				IsBaseDefense ? "1" : "0",
				Team2.c_str(),
				EnabledInE ? "1" : "0",
				EnabledInM ? "1" : "0",
				EnabledInH ? "1" : "0"
			);
			if (Enabled) {
				CINI::CurrentDocument->WriteBool("AITriggerTypesEnable", ID.c_str(), Enabled);
			}
			else {
				CINI::CurrentDocument->DeleteKey("AITriggerTypesEnable", ID.c_str());
			}
			CINI::CurrentDocument->WriteString("AITriggerTypes", ID.c_str(), value);
			UsedINIIndices.insert(ID);
			CLuaConsole::updateAITrigger = true;
		}
		void change_id(std::string id)
		{
			UsedINIIndices.erase(ID);
			UsedINIIndices.insert(id);
			ID = id;
		}
		void release_id() const {
			UsedINIIndices.erase(ID);
		}
		void delete_ai_trigger_self() const
		{
			CINI::CurrentDocument->DeleteKey("AITriggerTypes", ID.c_str());
			CINI::CurrentDocument->DeleteKey("AITriggerTypesEnable", ID.c_str());
			UsedINIIndices.erase(ID);
			CLuaConsole::updateAITrigger = true;
		}
		static void delete_ai_trigger(std::string ID)
		{
			CINI::CurrentDocument->DeleteKey("AITriggerTypes", ID.c_str());
			CINI::CurrentDocument->DeleteKey("AITriggerTypesEnable", ID.c_str());
			UsedINIIndices.erase(ID);
			CLuaConsole::updateAITrigger = true;
		}

	private:
		static int ReadComparator(FString text, int index)
		{
			int num = 0;
			if (text.GetLength() != 64) return num;
			if (index < 0 || index > 7) return num;

			auto thisText = text.Mid(8 * index, 8);
			unsigned char bytes[4]{ 0 };
			for (int i = 0; i < 4; ++i) {
				bytes[i] = static_cast<unsigned char>(strtol(thisText.Mid(2 * i, 2), NULL, 16));

			}
			num |= (bytes[0] << 0);
			num |= (bytes[1] << 8);
			num |= (bytes[2] << 16);
			num |= (bytes[3] << 24);

			return num;
		}
		static FString SaveComparator(int comparator)
		{
			FString ret = "";

			std::stringstream ss;
			ss << std::hex << comparator;
			FString bigEndian = ss.str();
			while (bigEndian.GetLength() < 8) {
				bigEndian = "0" + bigEndian;
			}

			for (int i = 3; i >= 0; --i) {
				ret += bigEndian.Mid(2 * i, 2);
			}
			return ret;
		}

	};

	static void place_terrain(int y, int x, std::string id)
	{
		int pos = CMapData::Instance->GetCoordIndex(x, y);
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(pos);
			if (cell->Terrain > -1) {
				CMapData::Instance->DeleteTerrainData(cell->Terrain);
			}
			CMapData::Instance->SetTerrainData(id.c_str(), pos);
			CLuaConsole::needRedraw = true;
		}
	}

	static void remove_terrain(int y, int x)
	{
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(x, y);
			if (cell->Terrain > -1) {
				CMapData::Instance->DeleteTerrainData(cell->Terrain);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static void place_smudge(int y, int x, std::string id)
	{
		int pos = CMapData::Instance->GetCoordIndex(x, y);
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(pos);
			if (cell->Smudge > -1) {
				CMapData::Instance->DeleteSmudgeData(cell->Smudge);
			}
			CSmudgeData obj;
			obj.X = y;
			obj.Y = x;
			obj.TypeID = id.c_str();
			CMapData::Instance->SetSmudgeData(&obj);
			CLuaConsole::needRedraw = true;
		}
	}

	static void remove_smudge(int y, int x)
	{
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(x, y);
			if (cell->Smudge > -1) {
				CMapData::Instance->DeleteSmudgeData(cell->Smudge);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static void place_overlay(int y, int x, int overlay, int overlayData = -1)
	{		
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int OverlayCount = Variables::RulesMap.ParseIndicies("OverlayTypes").size();
		int limit = (ExtConfigs::ExtOverlays || CMapDataExt::NewINIFormat >= 5) ? 0xFFFF : 0xFF;
		if (overlay >= limit || (overlay < 0 && overlayData < 0))
		{
			std::ostringstream oss;
			oss << "Invalid Overlay: " << overlay;
			write_lua_console(oss.str());
			return;
		}
		if (overlayData > 60)
		{
			std::ostringstream oss;
			oss << "Invalid OverlayData: " << overlayData;
			write_lua_console(oss.str());
			return;
		}
		CLuaConsole::needRedraw = true;
		const int ORE_COUNT = 12;
		auto pExt = CMapDataExt::GetExtension();
		int pos = pExt->GetCoordIndex(x, y);
		auto ovr = pExt->GetOverlayAt(pos);
		auto ovrd = pExt->GetOverlayDataAt(pos);
		if (overlay < 0)
			overlay = ovr;
		if (overlayData < 0)
			overlayData = ovr == limit ? 0 : ovrd;
		
		pExt->SetNewOverlayAt(pos, overlay);
		pExt->SetOverlayDataAt(pos, overlayData);
	}

	static void remove_overlay(int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int pos = CMapData::Instance->GetCoordIndex(x, y);
		CMapDataExt::GetExtension()->SetNewOverlayAt(pos, 0xFFFF);
		CMapData::Instance->SetOverlayDataAt(pos, 0);
		CLuaConsole::needRedraw = true;
	}

	static void smooth_ore(int Y1, int X1, int Y2, int X2)
	{
		int x1, x2, y1, y2;
		if (Y1 == 0 && Y1 == X1 && X1 == Y2 && Y2 == X2)
		{
			x1 = 0;
			y1 = 0;
			x2 = CMapData::Instance->MapWidthPlusHeight;
			y2 = CMapData::Instance->MapWidthPlusHeight;
		}
		else
		{
			if (X1 < X2) {
				x1 = X1;
				x2 = X2;
			}
			else {
				x2 = X1;
				x1 = X2;
			}
			if (Y1 < Y2) {
				y1 = Y1;
				y2 = Y2;
			}
			else {
				y2 = Y1;
				y1 = Y2;
			}
		}
		for (int x = x1; x < x2; ++x)
		{
			for (int y = y1; y < y2; ++y)
			{
				if (!CMapData::Instance->IsCoordInMap(x, y))
					continue;
				CMapData::Instance->SmoothTiberium(CMapData::Instance->GetCoordIndex(x, y));
			}
		}	
		CLuaConsole::needRedraw = true;
	}

	static void place_wall(int y, int x, int wall, int damageStage)
	{
		damageStage--; // to fit lua
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;
		auto& walls = CViewObjectsExt::WallDamageStages;
		if (walls.find(wall) != walls.end())
		{
			if (damageStage == -1)
				damageStage = STDHelpers::RandomSelectInt(0, walls[wall]);
			else if (damageStage >= walls[wall])
			{
				std::ostringstream oss;
				oss << "Wall: " << wall << ", Invalid Damage Stage:" << damageStage + 1;
				write_lua_console(oss.str());
				return;
			}	
		}
		else
		{
			std::ostringstream oss;
			oss << "Invalid Wall: " << wall;
			write_lua_console(oss.str());
			return;
		}
		CLuaConsole::needRedraw = true;
		int pos = CMapData::Instance->GetCoordIndex(x, y);
		CMapDataExt::PlaceWallAt(pos, wall, damageStage);
	}

	static int place_waypoint(int y, int x, int index = -1)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return -1;
		int oldWp = CMapData::Instance->GetCellAt(x, y)->Waypoint;
		if (oldWp > -1)
		{
			write_lua_console(std::format("Waypoint {} already exists at ({},{}), abort.", 
				CINI::CurrentDocument->GetKeyAt("Waypoints", oldWp).m_pchData, x, y));
			return atoi(CINI::CurrentDocument->GetKeyAt("Waypoints", oldWp));
		}

		CLuaConsole::needRedraw = true;
		if (index < 0)
		{
			index = atoi(CINI::CurrentDocument->GetAvailableKey("Waypoints"));
		}
		FString key;
		FString value;
		key.Format("%d", index);
		value.Format("%d", x * 1000 + y);
		CINI::CurrentDocument->WriteString("Waypoints", key, value);
		CMapData::Instance->UpdateFieldWaypointData(false);
		return index;
	}

	static void remove_waypoint(int index)
	{
		CLuaConsole::needRedraw = true;
		if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
		{
			FString key;
			key.Format("%d", index);
			if (CINI::CurrentDocument->KeyExists("Waypoints", key))
			{
				auto&& value = pSection->GetString(key);
				int x = atoi(value) / 1000;
				int y = atoi(value) % 1000;
				CINI::CurrentDocument->DeleteKey(pSection, key);
				CMapData::Instance->UpdateFieldWaypointData(false);

				if (CMapData::Instance->IsMultiOnly())
				{
					int k, l;
					for (k = -1; k < 2; k++)
						for (l = -1; l < 2; l++)
							CMapData::Instance->UpdateMapPreviewAt(x + k, y + l);
				}
			}
		}
	}

	static void remove_waypoint_at(int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;
		int index = CMapData::Instance->GetCellAt(x, y)->Waypoint;
		if (index > -1)
			remove_waypoint(atoi(CINI::CurrentDocument->GetKeyAt("Waypoints", index)));
	}

	static std::string get_string(std::string section, std::string key, std::string def = "", std::string loadFrom = "map")
	{
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			return ini->GetString(section.c_str(), key.c_str(), def.c_str()).m_pchData;
		}
		
		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		return mmh.GetString(section.c_str(), key.c_str(), def.c_str()).m_pchData;
	}

	static int get_integer(std::string section, std::string key, int def = 0, std::string loadFrom = "map")
	{
		auto result = get_string(section, key, "", loadFrom);
		int ret = 0;
		if (sscanf_s(result.c_str(), "%d", &ret) == 1)
			return ret;
		return def;
	}

	static float get_float(std::string section, std::string key, float def = 0.0f, std::string loadFrom = "map")
	{
		auto result = get_string(section, key, "", loadFrom);
		float ret = 0.0f;
		if (sscanf_s(result.c_str(), "%f", &ret) == 1)
		{
			if (strchr(result.c_str(), '%%'))
				ret *= 0.01f;
			return ret;
		}
		return def;
	}

	static bool get_bool(std::string section, std::string key, bool def = false, std::string loadFrom = "map")
	{
		auto result = get_string(section, key, "", loadFrom);
		switch (toupper(static_cast<unsigned char>(result[0])))
		{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'T':
		case 'Y':
			return true;
		case '0':
		case 'F':
		case 'N':
			return false;
		default:
			return def;
		}
	}

	static std::set<std::string> get_sections(std::string loadFrom = "map")
	{
		std::set<std::string> ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}

			auto itr = ini->Dict.begin();
			for (size_t i = 0, sz = ini->Dict.size(); i < sz; ++i, ++itr)
			{
				ret.insert(itr->first.m_pchData);
			}
			return ret;
		}

		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		for (auto pINI : mmh.GetINIData())
		{
			if (pINI)
			{
				auto itr = pINI->Dict.begin();
				for (size_t i = 0, sz = pINI->Dict.size(); i < sz; ++i, ++itr)
				{
					ret.insert(itr->first.m_pchData);
				}
			}
		}
		return ret;
	}

	static std::vector<std::string> get_keys(std::string section, std::string loadFrom = "map")
	{
		std::vector<std::string> ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			if (auto pSection = ini->GetSection(section.c_str()))
			{
				for (auto& [key, value] : pSection->GetEntities())
				{
					ret.push_back(key.m_pchData);
				}
			}
			return ret;
		}

		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		for (auto& [key, value] : mmh.GetUnorderedUnionSection(section.c_str()))
		{
			ret.push_back(key.m_pchData);
		}
		return ret;
	}
	
	static std::vector<std::string> get_values(std::string section, std::string loadFrom = "map")
	{
		std::vector<std::string> ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			if (auto pSection = ini->GetSection(section.c_str()))
			{
				for (auto& [key, value] : pSection->GetEntities())
				{
					ret.push_back(value.m_pchData);
				}
			}
			return ret;
		}

		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		for (auto& [key, value] : mmh.GetUnorderedUnionSection(section.c_str()))
		{
			ret.push_back(value.m_pchData);
		}
		return ret;
	}

	static std::vector<std::pair<std::string, std::string>> get_key_value_pairs(std::string section, std::string loadFrom = "map")
	{
		std::vector<std::pair<std::string, std::string>>  ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			if (auto pSection = ini->GetSection(section.c_str()))
			{
				for (auto& [key, value] : pSection->GetEntities())
				{
					ret.push_back(std::make_pair(key.m_pchData, value.m_pchData));
				}
			}
			return ret;
		}

		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		for (auto& [key, value] : mmh.GetUnorderedUnionSection(section.c_str()))
		{
			ret.push_back(std::make_pair(key.m_pchData, value.m_pchData));
		}
		return ret;
	}

	static std::vector<std::pair<int, std::string>> get_ordered_values(std::string section, std::string loadFrom = "map")
	{
		std::vector<std::pair<int, std::string>>  ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			if (auto pSection = ini->GetSection(section.c_str()))
			{
				std::map<int, FString> collector;

				for (auto& pair : pSection->GetIndices())
					collector[pair.second] = pair.first;

				for (auto& pair : collector)
				{
					const auto& contains = pair.second;
					const auto& value = pSection->GetEntities().find(contains)->second;
					ret.push_back(std::make_pair(pair.first, value.m_pchData));
				}
			}
		}
		else
		{
			MultimapHelper mmh;
			ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
			auto&& entries = mmh.ParseIndicies(section.c_str(), true);
			for (size_t i = 0, sz = entries.size(); i < sz; i++)
			{
				ret.push_back(std::make_pair(i, entries[i].m_pchData));
			}
		}
		return ret;
	}

	static void write_string(std::string section, std::string key, std::string value)
	{
		CINI::CurrentDocument->WriteString(section.c_str(), key.c_str(), value.c_str());
	}

	static void delete_key(std::string section, std::string key)
	{
		CINI::CurrentDocument->DeleteKey(section.c_str(), key.c_str());
	}

	static void delete_section(std::string section)
	{
		CINI::CurrentDocument->DeleteSection(section.c_str());
	}

	static std::string get_free_waypoint()
	{
		return CINI::CurrentDocument->GetAvailableKey("Waypoints").m_pchData;
	}

	static std::string get_free_key(std::string section)
	{
		return CINI::CurrentDocument->GetAvailableKey(section.c_str()).m_pchData;
	}

	static std::string get_param(std::string section, std::string key, int index, std::string delimiter = ",", std::string loadFrom = "map")
	{
		index--;// to fit lua 
		auto&& str = get_string(section, key, "", loadFrom);
		auto&& atoms = split_string(str, delimiter);
		if (atoms.size() > index) 
		{
			return atoms[index];
		}
		return "";
	}

	static void set_param(std::string section, std::string key, std::string value, int index, std::string delimiter = ",")
	{
		index--;// to fit lua 
		auto&& str = get_string(section, key, "");
		auto&& atoms = split_string(str, delimiter);
		if (atoms.size() > index)
		{
			std::string fullValue;
			for (int i = 0; i < atoms.size(); ++i)
			{
				auto& atom = atoms[i];
				if (i == index)
					atom = value;
				fullValue += atom;
				if (i != atoms.size() - 1)
					fullValue += delimiter[0];
			}
			write_string(section, key, fullValue);
		}
	}
	
	static std::string trim_index(std::string value)
	{
		char ch = ' ';
		size_t pos = value.find(ch);

		if (pos != std::string::npos) {
			std::string result = value.substr(0, pos);
			return result;
		}
		return value;
	}

	static void place_infantry(std::string house, std::string type, int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		CMapData::Instance->SetInfantryData(NULL, type.c_str(), house.c_str(), coord, -1);
		CLuaConsole::needRedraw = true;
	}

	static void remove_infantry(int indexY, int x = -1, int pos = -1)
	{
		if (x < 0)
		{
			CMapData::Instance->DeleteInfantryData(indexY);
			CLuaConsole::needRedraw = true;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			int idx = CMapDataExt::GetInfantryAt(CMapData::Instance->GetCoordIndex(x, indexY), pos);
			if (idx > -1)
			{
				CMapData::Instance->DeleteInfantryData(idx);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static infantry get_infantry(int indexY, int x = -1, int pos = -1)
	{
		infantry ret{ "","" ,-1 ,-1 };
		if (x < 0)
		{
			CInfantryData obj;
			CMapData::Instance->GetInfantryData(indexY, obj);
			return infantry::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
			{
				return ret;
			}

			int idx = CMapDataExt::GetInfantryAt(CMapData::Instance->GetCoordIndex(x, indexY), pos);
			if (idx > -1)
			{
				CInfantryData obj;
				CMapData::Instance->GetInfantryData(idx, obj);
				return infantry::convert(obj);
			}
		}
		return ret;
	}

	static void place_unit(std::string house, std::string type, int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		CMapData::Instance->SetUnitData(NULL, type.c_str(), house.c_str(), coord, "-1");
		CLuaConsole::needRedraw = true;
	}

	static void remove_unit(int indexY, int x = -1)
	{
		if (x < 0)
		{
			CMapData::Instance->DeleteUnitData(indexY);
			CLuaConsole::needRedraw = true;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Unit;
			if (idx > -1)
			{
				CMapData::Instance->DeleteUnitData(idx);
				CLuaConsole::needRedraw = true;
			}
		}
	}
	
	static unit get_unit(int indexY, int x = -1)
	{
		unit ret = { "","",-1,-1 };
		if (x < 0)
		{
			CUnitData obj;
			CMapData::Instance->GetUnitData(indexY, obj);
			return unit::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return ret;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Unit;
			if (idx > -1)
			{
				CUnitData obj;
				CMapData::Instance->GetUnitData(idx, obj);
				return unit::convert(obj);
			}
		}
		return ret;
	}

	static void place_aircraft(std::string house, std::string type, int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		CMapData::Instance->SetAircraftData(NULL, type.c_str(), house.c_str(), coord, "-1");
		CLuaConsole::needRedraw = true;
	}

	static void remove_aircraft(int indexY, int x = -1)
	{
		if (x < 0)
		{
			CMapData::Instance->DeleteAircraftData(indexY);
			CLuaConsole::needRedraw = true;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Aircraft;
			if (idx > -1)
			{
				CMapData::Instance->DeleteAircraftData(idx);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static aircraft get_aircraft(int indexY, int x = -1)
	{
		aircraft ret = { "","",-1,-1 };
		if (x < 0)
		{
			CAircraftData obj;
			CMapData::Instance->GetAircraftData(indexY, obj);
			return aircraft::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return ret;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Aircraft;
			if (idx > -1)
			{
				CAircraftData obj;
				CMapData::Instance->GetAircraftData(idx, obj);
				return aircraft::convert(obj);
			}
		}
		return ret;
	}

	static void place_building(std::string house, std::string type, int y, int x, bool ignoreOverlap = false)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		auto oldCheck = ExtConfigs::PlaceStructureOverlappingCheck;
		if (ignoreOverlap)
		{
			ExtConfigs::PlaceStructureOverlappingCheck = false;
		}
		CMapData::Instance->SetBuildingData(NULL, type.c_str(), house.c_str(), coord, "-1");
		ExtConfigs::PlaceStructureOverlappingCheck = oldCheck;

		CLuaConsole::needRedraw = true; 
		CLuaConsole::updateBuilding = true;
	}

	static void remove_building(int indexY, int x = -1)
	{
		if (x < 0)
		{
			CMapDataExt::DeleteBuildingByIniID = true;
			CMapData::Instance->DeleteBuildingData(indexY);
			CLuaConsole::needRedraw = true;
			CMapDataExt::DeleteBuildingByIniID = false;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Structure;
			if (idx > -1)
			{
				CMapData::Instance->DeleteBuildingData(idx);
				CLuaConsole::needRedraw = true;
			}
		}

	}

	static building get_building(int indexY, int x = -1)
	{
		building ret = { "","",-1,-1 };
		if (x < 0)
		{
			CBuildingData obj;
			CMapDataExt::GetBuildingDataByIniID(indexY, obj);
			return building::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return ret;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Structure;
			if (idx > -1)
			{
				CBuildingData obj;
				CMapData::Instance->GetBuildingData(idx, obj);
				return building::convert(obj);
			}
		}
		return ret;
	}

	static std::vector<building> get_buildings()
	{
		std::vector<building> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Structures"); ++i)
		{
			ret.push_back(get_building(i));
		}
		return ret;
	}

	static std::vector<aircraft> get_aircrafts()
	{
		std::vector<aircraft> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Aircraft"); ++i)
		{
			ret.push_back(get_aircraft(i));
		}
		return ret;
	}

	static std::vector<infantry> get_infantries()
	{
		std::vector<infantry> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Infantry"); ++i)
		{
			ret.push_back(get_infantry(i));
		}
		return ret;
	}

	static std::vector<unit> get_units()
	{
		std::vector<unit> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Units"); ++i)
		{
			ret.push_back(get_unit(i));
		}
		return ret;
	}

	static void place_celltag(int y, int x, std::string id)
	{
		FString key;
		key.Format("%d", x * 1000 + y);
		if (CINI::CurrentDocument->KeyExists("CellTags", key))
		{
			write_lua_console(std::format("CellTag {} already exists at ({},{}), abort.",
				CINI::CurrentDocument->GetString("CellTags", key).m_pchData, x, y));
			return;
		}
		CINI::CurrentDocument->WriteString("CellTags", key, id.c_str());
		CLuaConsole::needRedraw = true;
		CLuaConsole::updateCellTag = true;
	}

	static void remove_celltags(std::string id)
	{
		std::vector<FString> keys;
		if (auto pSection = CINI::CurrentDocument->GetSection("CellTags"))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				if (value == id.c_str())
					keys.push_back(key);
			}
		}
		for (const auto& key : keys)
		{
			CINI::CurrentDocument->DeleteKey("CellTags", key);
		}
		CLuaConsole::needRedraw = true;
		CLuaConsole::updateCellTag = true;
	}

	static void remove_celltag(int y, int x)
	{
		FString key;
		key.Format("%d", x * 1000 + y);
		CINI::CurrentDocument->DeleteKey("CellTags", key);
		CLuaConsole::needRedraw = true;
		CLuaConsole::updateCellTag = true;
	}

	static void place_node(std::string house, std::string type, int y, int x, int index = -1)
	{
		if (CMapData::Instance->IsMultiOnly())
			return;
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		if (auto pHouse = CINI::CurrentDocument->GetSection(house.c_str()))
		{
			FString value;
			value.Format("%s,%d,%d", type.c_str(), y, x);
			if (index < 0)
			{
				for (int i = 0; i < 1000; ++i)
				{
					FString key;
					key.Format("%03d", i);
					if (!CINI::CurrentDocument->KeyExists(house.c_str(), key))
					{
						FString count;
						count.Format("%d", i + 1);
						CINI::CurrentDocument->WriteString(house.c_str(), "NodeCount", count);
						CINI::CurrentDocument->WriteString(house.c_str(), key, value);
						CLuaConsole::updateNode = true;
						CLuaConsole::needRedraw = true;
						break;
					}
				}
			}
			else
			{
				std::vector<FString> nodes;
				int nodeCount = CINI::CurrentDocument->GetInteger(house.c_str(), "NodeCount", 0);
				for (int i = 0; i < nodeCount; ++i)
				{
					FString key;
					key.Format("%03d", i);
					if (CINI::CurrentDocument->KeyExists(house.c_str(), key))
					{
						nodes.push_back(CINI::CurrentDocument->GetString(house.c_str(), key));
					}
				}
				if (index > nodes.size()) index = nodes.size();
				nodes.insert(nodes.begin() + index, value);
				int i = 0;
				for (const auto& node : nodes)
				{
					FString key;
					key.Format("%03d", i);
					CINI::CurrentDocument->WriteString(house.c_str(), key, node);
					i++;
				}
				FString count;
				count.Format("%d", nodes.size());
				CINI::CurrentDocument->WriteString(house.c_str(), "NodeCount", count);
				CLuaConsole::updateNode = true;
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static void remove_node(int y, int x)
	{
		if (CMapData::Instance->IsMultiOnly())
			return;
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		const auto& node = CMapData::Instance->GetCellAt(x, y)->BaseNode;
		CMapData::Instance->DeleteBaseNodeData(node.House, node.BasenodeID);
		CLuaConsole::needRedraw = true;
	}

	static cell get_cell(int y, int x)
	{
		cell c;

		if (!CMapData::Instance->IsCoordInMap(x, y))
		{
			c.X = -1;
			c.Y = -1;
			c.X = -1;
			c.Y = -1;
			c.Unit = -1;
			c.Infantry[0] = -1;
			c.Infantry[1] = -1;
			c.Infantry[2] = -1;
			c.Aircraft = -1;
			c.Structure = -1;
			c.TypeListIndex = -1;
			c.Terrain = -1;
			c.TerrainType = -1;
			c.Smudge = -1;
			c.SmudgeType = -1;
			c.Waypoint = -1;
			c.BuildingID = -1;
			c.BasenodeID = -1;
			c.House = "";
			c.Overlay = 0xFFFF;
			c.OverlayData = 0;
			c.TileIndex = -1;
			c.TileIndexHiPart = -1;
			c.TileSubIndex = -1;
			c.Height = -1;
			c.IceGrowth = -1;
			c.CellTag = -1;
			c.Tube = -1;
			c.TubeDataIndex = -1;
			c.StatusFlag = -1;
			c.NotAValidCell = false;
			c.IsHiddenCell = false;
			c.RedrawTerrain = false;
			c.CliffHack = false;
			c.AltIndex = false;
			return c;
		}

		int pos = CMapData::Instance->GetCoordIndex(x, y);
		auto pCell = CMapData::Instance->GetCellAt(pos);
		auto& pCellExt = CMapDataExt::CellDataExts[pos];
		c.X = x;
		c.Y = y;
		c.Unit = pCell->Unit;
		c.Infantry[0] = pCell->Infantry[0];
		c.Infantry[1] = pCell->Infantry[1];
		c.Infantry[2] = pCell->Infantry[2];
		c.Aircraft = pCell->Aircraft;
		c.Structure = pCell->Structure > -1 ? CMapDataExt::StructureIndexMap[pCell->Structure] : -1;
		c.TypeListIndex = pCell->TypeListIndex;
		c.Terrain = pCell->Terrain;
		c.TerrainType = pCell->TerrainType;
		c.Smudge = pCell->Smudge;
		c.SmudgeType = pCell->SmudgeType;
		c.Waypoint = pCell->Waypoint;
		c.BuildingID = pCell->BaseNode.BuildingID;
		c.BasenodeID = pCell->BaseNode.BasenodeID;
		c.House = pCell->BaseNode.House.m_pchData;
		c.Overlay = pCellExt.NewOverlay;
		c.OverlayData = pCell->OverlayData;
		c.TileIndex = pCell->TileIndex;
		c.TileIndexHiPart = pCell->TileIndexHiPart;
		c.TileSubIndex = pCell->TileSubIndex;
		c.Height = pCell->Height;
		c.IceGrowth = pCell->IceGrowth;
		c.CellTag = pCell->CellTag;
		c.Tube = pCell->Tube;
		c.TubeDataIndex = pCell->TubeDataIndex;
		c.StatusFlag = pCell->StatusFlag;
		c.NotAValidCell = pCell->Flag.NotAValidCell > 0 ? true : false;
		c.IsHiddenCell = pCell->Flag.IsHiddenCell > 0 ? true : false;
		c.RedrawTerrain = pCell->Flag.RedrawTerrain > 0 ? true : false;
		c.CliffHack = pCell->Flag.CliffHack > 0 ? true : false;
		c.AltIndex = pCell->Flag.AltIndex;
		return c;
	}

	static std::vector<cell> get_cells()
	{
		std::vector<cell> ret;
		for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
		{
			for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
			{
				if (CMapData::Instance->IsCoordInMap(x, y))
				{
					ret.push_back(get_cell(y, x));
				}
			}
		}
		return ret;
	}

	static std::vector<cell> get_multi_selected_cells()
	{
		std::vector<cell> ret;
		for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
		{
			for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
			{
				if (CMapData::Instance->IsCoordInMap(x, y))
				{
					if (MultiSelection::IsSelected(x, y))
						ret.push_back(get_cell(y, x));
				}
			}
		}
		return ret;
	}

	static std::vector<cell> get_hidden_cells()
	{
		std::vector<cell> ret;
		for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
		{
			for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
			{
				if (CMapData::Instance->IsCoordInMap(x, y))
				{
					if (CMapData::Instance->GetCellAt(x, y)->IsHidden())
						ret.push_back(get_cell(y, x));
				}
			}
		}
		return ret;
	}

	static void place_whole_tile(int y, int x, int tileIdx)
	{
		CMapDataExt::GetExtension()->PlaceTileAt(x, y, tileIdx);
		CLuaConsole::needRedraw = true;
	}

	static void place_tile(int y, int x, tile tile, int height = -1, int altType = -1)
	{
		if (!tile.Valid) return;
		if (!CMapData::Instance->IsCoordInMap(x, y)) return;
		auto pExt = CMapDataExt::GetExtension();
		auto cell = pExt->GetCellAt(x, y);
		cell->TileIndex = tile.TileIndex;
		cell->TileSubIndex = tile.TileSubIndex;
		if (altType < 0)
			altType = STDHelpers::RandomSelectInt(0, pExt->TileData[tile.TileIndex].AltTypeCount + 1);
		else if (altType > pExt->TileData[tile.TileIndex].AltTypeCount)
			altType = pExt->TileData[tile.TileIndex].AltTypeCount;
		cell->Flag.AltIndex = altType;
		if (height == -1)
			height = cell->Height + tile.Height;
		pExt->SetHeightAt(x, y, height);
		pExt->UpdateMapPreviewAt(x, y);
		CLuaConsole::needRedraw = true;
	}

	static void set_height(int y, int x, int height)
	{
		CMapDataExt::GetExtension()->SetHeightAt(x, y, height);
		CLuaConsole::needRedraw = true;
	}

	static void hide_tile(int y, int x, int type = 0)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y)) return;
		auto cell = CMapData::Instance->GetCellAt(x, y);
		if (type == 1)
			cell->Flag.IsHiddenCell = true;
		else if (type == 2)
			cell->Flag.IsHiddenCell = false;
		else if (type == 3)
			cell->Flag.IsHiddenCell ^= 1;
		CLuaConsole::needRedraw = true;
	}

	static void hide_tile_set(int index, int type = 0)
	{
		if (index >= 0 && index < CMapDataExt::TileSet_starts.size() - 1) {
			for (int i = CMapDataExt::TileSet_starts[index]; i < CMapDataExt::TileSet_starts[index + 1]; i++) {
				if (type == 1) {
					(*CTileTypeClass::Instance)[i].IsHidden = 1;
				}
				else if (type == 2) {
					(*CTileTypeClass::Instance)[i].IsHidden = 0;
				}
				else if (type == 3) {
					(*CTileTypeClass::Instance)[i].IsHidden ^= 1;
				}
			}
			CLuaConsole::needRedraw = true;
		}
	}
	
	static void multi_select_tile(int y, int x, int type = 0)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y)) return;
		if (type == 1) {
			MultiSelection::AddCoord(x, y);
		}
		else if (type == 2) {
			MultiSelection::RemoveCoord(x, y);
		}
		else if (type == 3) {
			MultiSelection::ReverseStatus(x, y);
		}
		CLuaConsole::needRedraw = true;
	}

	static void multi_select_tile_set(int index, int type = 0)
	{
		if (index >= 0 && index < CMapDataExt::TileSet_starts.size() - 1) {
			for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; x++) {
				for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; y++) {
					if (!CMapData::Instance->IsCoordInMap(x, y)) continue;
					auto cell = CMapData::Instance->GetCellAt(x, y);
					int tileIdx = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
					if (tileIdx >= CMapDataExt::TileSet_starts[index] && tileIdx < CMapDataExt::TileSet_starts[index + 1]) {
						if (type == 1) {
							MultiSelection::AddCoord(x, y);
						}
						else if (type == 2) {
							MultiSelection::RemoveCoord(x, y);
						}
						else if (type == 3) {
							MultiSelection::ReverseStatus(x, y);
						}
					}
				}
			}
			CLuaConsole::needRedraw = true;
		}
	}

	static void create_shore(int Y1, int X1, int Y2, int X2)
	{
		int x1, x2, y1, y2;
		if (X1 < X2) {
			x1 = X1;
			x2 = X2;
		}
		else {
			x2 = X1;
			x1 = X2;
		}
		if (Y1 < Y2) {
			y1 = Y1;
			y2 = Y2;
		}
		else {
			y2 = Y1;
			y1 = Y2;
		}
		CMapData::Instance->CreateShore(x1, y1, x2, y2);
		CLuaConsole::needRedraw = true;
	}

	static void smooth_lat(int Y1, int X1, int Y2, int X2)
	{
		int x1, x2, y1, y2;
		if (X1 < X2) {
			x1 = X1;
			x2 = X2;
		}
		else {
			x2 = X1;
			x1 = X2;
		}
		if (Y1 < Y2) {
			y1 = Y1;
			y2 = Y2;
		}
		else {
			y2 = Y1;
			y1 = Y2;
		}
		for (int x = x1; x < x2; ++x)
		{
			for (int y = y1; y < y2; ++y)
			{
				CMapDataExt::SmoothTileAt(x, y);
			}
		}
		CLuaConsole::needRedraw = true;
	}

	static void create_slope(int Y1, int X1, int Y2, int X2)
	{
		int x1, x2, y1, y2;
		if (X1 < X2) {
			x1 = X1;
			x2 = X2;
		}
		else {
			x2 = X1;
			x1 = X2;
		}
		if (Y1 < Y2) {
			y1 = Y1;
			y2 = Y2;
		}
		else {
			y2 = Y1;
			y1 = Y2;
		}
		for (int x = x1; x < x2; ++x)
		{
			for (int y = y1; y < y2; ++y)
			{
				CMapDataExt::CreateSlopeAt(x, y);
			}
		}
		CLuaConsole::needRedraw = true;
	}

	static void update_minimap(int Y = -1, int X = -1)
	{
		if (X < 0 || Y < 0)
		{
			for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
			{
				for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
				{
					if (CMapData::Instance->IsCoordInMap(x, y))
					{
						CMapData::Instance->UpdateMapPreviewAt(x, y);
					}
				}
			}
		}
		else
		{
			CMapData::Instance->UpdateMapPreviewAt(X, Y);
		}
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.Minimap.m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}

	static int create_snapshot()
	{
		CMapData::Instance->UpdateINIFile(SaveMapFlag::UpdateMapFieldData);
		auto ini = &CMapData::Instance->INI;
		int version = 1;
		while (snapshots.find(version) != snapshots.end())
			version++;

		auto& snapshot = snapshots[version];
		auto itr = ini->Dict.begin();
		for (size_t i = 0, sz = ini->Dict.size(); i < sz; ++i, ++itr)
		{
			auto& section = snapshot.INI[itr->first];
			for (const auto& [key, value] : itr->second.GetEntities())
			{
				section[key] = value;
			}
		}
		memcpy(snapshot.Overlay, CMapDataExt::NewOverlay, sizeof(CMapDataExt::NewOverlay));
		memcpy(snapshot.OverlayData, CMapData::Instance->OverlayData, sizeof(CMapData::Instance->OverlayData));

		for (int i = 0; i < CMapData::Instance->CellDataCount; ++i)
		{
			snapshot.CellDatas.push_back(CMapData::Instance->CellDatas[i]);
		}
		CMapData::Instance->UpdateINIFile(SaveMapFlag::LoadFromINI);
		snapshot.savedTime = getCurrentTime();
		snapshot.fileName = CFinalSunApp::MapPath();
		if (snapshot.fileName == "")
			snapshot.fileName = "New map";
		write_lua_console(std::format("Script has just created snapshot #{}.\n   To restore, type and run \"restore_snapshot({})\".", version, version));
		return version;
	}

	static void restore_snapshot(int version)
	{
		if (snapshots.find(version) == snapshots.end())
		{
			write_lua_console(std::format("Cannot find snapshot #{}", version));
			return;
		}
		auto& snapshot = snapshots[version];
		Logger::Debug("Restoring map snapshot #%d, time point: %s\n", version, formatTime(snapshot.savedTime).c_str());
		if (CINI::CurrentDocument->GetString("Map", "Theater") != snapshot.INI["Map"]["Theater"])
		{
			write_lua_console(std::format("Cannot restore snapshot #{}, theater dismatch.", version));
			return;
		}

		auto timeSpan = std::chrono::system_clock::now() - snapshot.savedTime.time;
		std::string title = Translations::TranslateOrDefault("LuaConsole.SnapshotConfirmTitle", "Are you sure to restore the snapshot?");
		FString text;
		text.Format(Translations::TranslateOrDefault("LuaConsole.SnapshotConfirmMessage"
				, "Snapshot timepoint: %s\nTime span: %s\nMap file name: %s")
			, formatTime(snapshot.savedTime).c_str()
			, formatTimeInterval(std::chrono::duration_cast<std::chrono::seconds>(timeSpan)).c_str()
				, snapshot.fileName.c_str());
	
		if (message_box(text, title, 1) == IDCANCEL)
			return;

		auto ini = &CMapData::Instance->INI;

		RECT newSize = { 0,0,0,0 };
		auto&& ns = STDHelpers::SplitString(snapshot.INI["Map"]["Size"], 3);
		newSize.right = atoi(ns[2]);
		newSize.bottom = atoi(ns[3]);
		RECT oldSize = { 0,0,0,0 };
		auto&& os = STDHelpers::SplitString(ini->GetString("Map", "Size"), 3);
		oldSize.right = atoi(os[2]);
		oldSize.bottom = atoi(os[3]);
		if (newSize.right != oldSize.right || newSize.bottom != oldSize.bottom)
		{
			MapRect rect{ 0, 0, newSize.right, newSize.bottom };
			CMapDataExt::GetExtension()->ResizeMapExt(&rect);
		}

		std::vector<FString> sections;
		auto itr = ini->Dict.begin();
		for (size_t i = 0, sz = ini->Dict.size(); i < sz; ++i, ++itr)
		{
			sections.push_back(itr->first);
		}
		
		for (auto& sec : sections)
			ini->DeleteSection(sec);
		for (const auto& section : snapshot.INI)
		{
			auto pSection = ini->AddSection(section.first);
			for (const auto& [key, value] : section.second)
			{
				ini->WriteString(pSection, key, value);
			}
		}
		memcpy(CMapDataExt::NewOverlay, snapshot.Overlay, sizeof(snapshot.Overlay));
		memcpy(CMapData::Instance->OverlayData, snapshot.OverlayData, sizeof(snapshot.OverlayData));

		for (int i = 0; i < CMapData::Instance->CellDataCount; ++i)
		{
			CMapData::Instance->CellDatas[i] = snapshot.CellDatas[i];
		}

		CMapDataExt::UpdateFieldStructureData_Optimized();
		CMapData::Instance->UpdateFieldOverlayData(false);
		CMapData::Instance->UpdateINIFile(SaveMapFlag::LoadFromINI);
		// load objects to avoid weird palette issue
		CIsoView::GetInstance()->PrimarySurfaceLost();
		CFinalSunDlg::Instance->MyViewFrame.Minimap.Update();
		CLuaConsole::needRedraw = true;
		CLuaConsole::updateMinimap = true;
	}

	static void clear_snapshot()
	{
		snapshots.clear();
	}

	static void save_undo()
	{
		CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	}

	static void save_redo()
	{
		// No longer needed since changed undo redo method
		//CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
		//CMapData::Instance->DoUndo();
	}

	static void move_to(int y, int x = -1)
	{
		if (x == -1)
		{
			FString wp;
			wp.Format("%d", y);
			auto pWP = CINI::CurrentDocument->GetString("Waypoints", wp, "-1");
			auto second = atoi(pWP);
			if (second >= 0)
			{
				int y = second % 1000;
				int x = second / 1000;
				if (CMapData::Instance->IsCoordInMap(x, y))
				{
					CMapDataExt::CellDataExt_FindCell.X = x;
					CMapDataExt::CellDataExt_FindCell.Y = y;
					CMapDataExt::CellDataExt_FindCell.drawCell = true;
					CIsoViewExt::MoveToMapCoord(y, x);
					CMapDataExt::CellDataExt_FindCell.drawCell = false;
				}
			}
		}
		else
		{
			if (CMapData::Instance->IsCoordInMap(x, y))
			{
				CMapDataExt::CellDataExt_FindCell.X = x;
				CMapDataExt::CellDataExt_FindCell.Y = y;
				CMapDataExt::CellDataExt_FindCell.drawCell = true;
				CIsoViewExt::MoveToMapCoord(y, x);
				CMapDataExt::CellDataExt_FindCell.drawCell = false;
			}
		}
	}


	static void redraw_window()
	{
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	}
}