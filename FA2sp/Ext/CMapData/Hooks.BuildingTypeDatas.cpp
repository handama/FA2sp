#include "Body.h"

#include <Helpers/Macro.h>

#include "../../FA2sp.h"

#include <CFinalSunDlg.h>

DEFINE_HOOK(4B5460, CMapData_InitializeBuildingTypes, 7)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(const char*, ID, 0x4);

	auto ProcessType = [pThis](const char* ID)
	{
		int idx = pThis->GetBuildingTypeID(ID);
		auto& DataExt = pThis->BuildingDataExts[idx];
		pThis->BuildingTypes[ID] = idx;
		DataExt.BottomCoords.clear();
		DataExt.DamageFireOffsets.clear();

		ppmfc::CString ImageID = Variables::RulesMap.GetString(ID, "Image", ID);
		auto foundation = CINI::Art->GetString(ImageID, "Foundation", "1x1");

		// https://modenc.renegadeprojects.com/Foundation
		// This flag is read from art(md).ini twice: 
		// from section you specified in rules as [object]¡úImage, 
		// and from simply [object] , 
		// and the second one overrules the first if present. 
		// however, ares's custom foundation doesn't have this bug.
		if (_strcmpi(foundation, "Custom"))
		{
			foundation = CINI::Art->GetString(ID, "Foundation", foundation);
		}
		
		if (_strcmpi(foundation, "Custom") && _strcmpi(foundation, "3x3REFINERY"))
		{
			auto sizes = STDHelpers::SplitStringMultiSplit(foundation, "x|X");
			if (sizes.size() >= 2)
			{
				DataExt.Width = atoi(sizes[0]);
				DataExt.Height = atoi(sizes[1]);
			}
			else
			{
				DataExt.Width = 1;
				DataExt.Height = 1;
			}
			if (DataExt.Width == 0)
				DataExt.Width = 1;
			if (DataExt.Height == 0)
				DataExt.Height = 1;
		}
		else
		{
			auto ParsePoint = [](const char* str)
			{
				int x = 0, y = 0;
				switch (sscanf_s(str, "%d,%d", &x, &y))
				{
				case 0:
					x = 0;
					y = 0;
					break;
				case 1:
					y = 0;
					break;
				case 2:
					break;
				default:
					__assume(0);
				}
				return MapCoord{ x,y };
			};

			if (_strcmpi(foundation, "3x3REFINERY"))
			{
				// Custom, code reference Ares
				DataExt.Width = CINI::Art->GetInteger(ImageID, "Foundation.X", 0);
				DataExt.Height = CINI::Art->GetInteger(ImageID, "Foundation.Y", 0);
				DataExt.Foundations = new std::vector<MapCoord>;
				for (int i = 0; i < DataExt.Width * DataExt.Height; ++i)
				{
					ppmfc::CString key;
					key.Format("Foundation.%d", i);
					if (auto pPoint = CINI::Art->TryGetString(ImageID, key)) {
						DataExt.Foundations->push_back(ParsePoint(*pPoint));
					}
					else
						break;
				}
			}
			else
			{
				DataExt.Width = 3;
				DataExt.Height = 3;
				DataExt.Foundations = new std::vector<MapCoord>;
				DataExt.Foundations->push_back({0,0});
				DataExt.Foundations->push_back({1,0});
				DataExt.Foundations->push_back({2,0});
				DataExt.Foundations->push_back({0,1});
				DataExt.Foundations->push_back({1,1});
				DataExt.Foundations->push_back({0,2});
				DataExt.Foundations->push_back({1,2});
				DataExt.Foundations->push_back({2,2});
			}
			
			// Build outline draw data
			DataExt.LinesToDraw = new std::vector<std::pair<MapCoord, MapCoord>>;
			std::vector<std::vector<BOOL>> LinesX, LinesY; 
			
			LinesX.resize(DataExt.Width);
			for (auto& l : LinesX)
				l.resize(DataExt.Height + 1);
			LinesY.resize(DataExt.Width + 1);
			for (auto& l : LinesY)
				l.resize(DataExt.Height);

			for (const auto& block : *DataExt.Foundations)
			{
				LinesX[block.X][block.Y] = !LinesX[block.X][block.Y];
				LinesX[block.X][block.Y + 1] = !LinesX[block.X][block.Y + 1];
				LinesY[block.X][block.Y] = !LinesY[block.X][block.Y];
				LinesY[block.X + 1][block.Y] = !LinesY[block.X + 1][block.Y];
			}

			for (size_t y = 0; y < DataExt.Height + 1; ++y)
			{
				size_t length = 0;
				for (size_t x = 0; x < DataExt.Width; ++x)
				{
					if (LinesX[x][y])
						++length;
					else
					{
						if (!length)
							continue;
						MapCoord start, end;
						start.X = ((x - length) - y) * 30;
						start.Y = ((x - length) + y) * 15;
						end.X = (x - y) * 30 + 2;
						end.Y = (x + y) * 15 + 1;
						DataExt.LinesToDraw->push_back(std::make_pair(start, end));
						length = 0;
					}
				}
				if (length)
				{
					MapCoord start, end;
					start.X = ((DataExt.Width - length) - y) * 30;
					start.Y = ((DataExt.Width - length) + y) * 15;
					end.X = (DataExt.Width - y) * 30 + 2;
					end.Y = (DataExt.Width + y) * 15 + 1;
					DataExt.LinesToDraw->push_back(std::make_pair(start, end));
				}
			}

			for (size_t x = 0; x < DataExt.Width + 1; ++x)
			{
				size_t length = 0;
				for (size_t y = 0; y < DataExt.Height; ++y)
				{
					if (LinesY[x][y])
						++length;
					else
					{
						if (!length)
							continue;
						MapCoord start, end;
						start.X = (x - (y - length)) * 30;
						start.Y = (x + (y - length)) * 15;
						end.X = (x - y) * 30;
						end.Y = (x + y) * 15;
						DataExt.LinesToDraw->push_back(std::make_pair(start, end));
						length = 0;
					}
				}
				if (length)
				{
					MapCoord start, end;
					start.X = (x - (DataExt.Height - length)) * 30;
					start.Y = (x + (DataExt.Height - length)) * 15;
					end.X = (x - DataExt.Height) * 30;
					end.Y = (x + DataExt.Height) * 15;
					DataExt.LinesToDraw->push_back(std::make_pair(start, end));
				}
			}
		}

		for (int i = 0; i < 8; ++i)
		{
			FString key;
			key.Format("DamageFireOffset%d", i);
			if (CINI::Art->KeyExists(ID, key))
			{
				auto atoms = STDHelpers::SplitString(CINI::Art->GetString(ID, key, "0,0"), 1);
				DataExt.DamageFireOffsets.push_back({ atoi(atoms[0]),atoi(atoms[1]) });
			}
			else
			{
				break;
			}
		}

		DataExt.BottomCoords.reserve(DataExt.Width + DataExt.Height - 1);
		for (int x = 0; x < DataExt.Width + DataExt.Height - 1; ++x)
		{
			if (x < DataExt.Width)
				DataExt.BottomCoords.emplace_back(DataExt.Height - 1, x);
			else
				DataExt.BottomCoords.emplace_back(DataExt.Height + DataExt.Width - 2 - x, DataExt.Width - 1);
		}
		
	};

	pThis->UpdateTypeDatas();
	if (ID)
		ProcessType(ID);
	else
	{
		pThis->BuildingDataExts.clear();
		BuildingDataExt tempBuildingData;
		tempBuildingData.Width = 1;
		tempBuildingData.Height = 1;
		tempBuildingData.BottomCoords = { {0,0} };
		CMapDataExt::BuildingDataExts[-1] = tempBuildingData;
		const auto Types = Variables::RulesMap.GetSection("BuildingTypes");
		for (auto& Type : Types)
			ProcessType(Type.second);
	}

	return 0;
}

DEFINE_HOOK(4A5089, CMapData_UpdateMapFieldData_Structures_CustomFoundation, 6)
{
	GET(int, BuildingIndex, ESI);
	GET_STACK(const int, X, STACK_OFFS(0x16C, 0x104));
	GET_STACK(const int, Y, STACK_OFFS(0x16C, 0x94));

	const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
	if (!DataExt.IsCustomFoundation())
	{
		for (int dy = 0; dy < DataExt.Width; ++dy)
		{
			for (int dx = 0; dx < DataExt.Height; ++dx)
			{
				const int x = X + dx;
				const int y = Y + dy;
				if (CMapData::Instance->GetCoordIndex(x, y) < CMapData::Instance->CellDataCount)
				{
					auto pCell = CMapData::Instance->GetCellAt(x, y);
					pCell->Structure = R->BX();
					pCell->TypeListIndex = BuildingIndex;
					CMapData::Instance->UpdateMapPreviewAt(x, y);
				}
			}
		}
	}
	else
	{
		for (const auto& block : *DataExt.Foundations)
		{
			const int x = X + block.Y;
			const int y = Y + block.X;
			if (CMapData::Instance->GetCoordIndex(x, y) < CMapData::Instance->CellDataCount)
			{
				auto pCell = CMapData::Instance->GetCellAt(x, y);
				pCell->Structure = R->BX();
				pCell->TypeListIndex = BuildingIndex;
				CMapData::Instance->UpdateMapPreviewAt(x, y);
			}
		}
	}

	return 0x4A57CD;
}