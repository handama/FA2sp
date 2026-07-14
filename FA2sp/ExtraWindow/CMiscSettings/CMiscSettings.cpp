#include "CMiscSettings.h"
#include "../Common.h"
#include "../../Ext/CRandomTree/Body.h"
#include "../../Ext/CFinalSunDlg/Body.h"

CINIDialog CMiscSettings::NewSpecialFlags = {342};
CINIDialog CMiscSettings::NewBasic = {343};
CINIDialog CMiscSettings::NewSinglePlayer= {344};
using T = CINIDialog::ControlType;

void CMiscSettings::InitNewSpecialFlags()
{
	NewSpecialFlags.SetTransparencyKey("SpecialFlagsOpacity");
	NewSpecialFlags.Translate(1314, "SpecialFlags.TiberiumGrows");
	NewSpecialFlags.Translate(1315, "SpecialFlags.TiberiumSpreads");
	NewSpecialFlags.Translate(1316, "SpecialFlags.TiberiumExplosive");
	NewSpecialFlags.Translate(1317, "SpecialFlags.DestroyableBridges");
	NewSpecialFlags.Translate(1318, "SpecialFlags.MCVDeploy");
	NewSpecialFlags.Translate(1319, "SpecialFlags.InitialVeteran");
	NewSpecialFlags.Translate(1320, "SpecialFlags.FixedAlliance");
	NewSpecialFlags.Translate(1321, "SpecialFlags.HarvesterImmune");
	NewSpecialFlags.Translate(1322, "SpecialFlags.FogOfWar");
	NewSpecialFlags.Translate(1323, "SpecialFlags.Inert");
	NewSpecialFlags.Translate(1324, "SpecialFlags.IonStorms");
	NewSpecialFlags.Translate(1325, "SpecialFlags.Meteorites");
	NewSpecialFlags.Translate(1326, "SpecialFlags.Visceroids");
	NewSpecialFlags.Translate(1285, "SpecialFlags.Note");
	NewSpecialFlags.TranslateTitle( "SpecialFlags.Title");

	NewSpecialFlags.SetControlInfo(1314, {T::CheckBox, "SpecialFlags", "TiberiumGrows", [] {}});
	NewSpecialFlags.SetControlInfo(1315, {T::CheckBox, "SpecialFlags", "TiberiumSpreads", [] {}});
	NewSpecialFlags.SetControlInfo(1316, {T::CheckBox, "SpecialFlags", "TiberiumExplosive", [] {}});
	NewSpecialFlags.SetControlInfo(1317, {T::CheckBox, "SpecialFlags", "DestroyableBridges", [] {}});
	NewSpecialFlags.SetControlInfo(1318, {T::CheckBox, "SpecialFlags", "MCVDeploy", [] {}});
	NewSpecialFlags.SetControlInfo(1319, {T::CheckBox, "SpecialFlags", "InitialVeteran", [] {}});
	NewSpecialFlags.SetControlInfo(1320, {T::CheckBox, "SpecialFlags", "FixedAlliance", [] {}});
	NewSpecialFlags.SetControlInfo(1321, {T::CheckBox, "SpecialFlags", "HarvesterImmune", [] {}});
	NewSpecialFlags.SetControlInfo(1322, {T::CheckBox, "SpecialFlags", "FogOfWar", [] {}});
	NewSpecialFlags.SetControlInfo(1323, {T::CheckBox, "SpecialFlags", "Inert", [] {}});
	NewSpecialFlags.SetControlInfo(1324, {T::CheckBox, "SpecialFlags", "IonStorms", [] {}});
	NewSpecialFlags.SetControlInfo(1325, {T::CheckBox, "SpecialFlags", "Meteorites", [] {}});
	NewSpecialFlags.SetControlInfo(1326, {T::CheckBox, "SpecialFlags", "Visceroids", [] {}});

	NewSpecialFlags.ShowDialog();
}

void CMiscSettings::InitNewBasic()
{
	NewBasic.SetTransparencyKey("BasicSettingsOpacity");
	NewBasic.Translate(1233, "BasicDesc");
	NewBasic.Translate(1234, "BasicName");
	NewBasic.Translate(1235, "BasicNextScenario");
	NewBasic.Translate(1236, "BasicAltNextScenario");
	NewBasic.Translate(1237, "BasicNewIniFormat");
	NewBasic.Translate(1238, "BasicCarryOverCap");
	NewBasic.Translate(1248, "BasicMultiplayerOnly");
	NewBasic.Translate(1239, "BasicEndOfGame");
	NewBasic.Translate(1240, "BasicSkipScore");
	NewBasic.Translate(1241, "BasicOneTimeOnly");
	NewBasic.Translate(1242, "BasicSkipMapSelect");
	NewBasic.Translate(1243, "BasicOfficial");
	NewBasic.Translate(1244, "BasicIgnoreGlobalAITriggers");
	NewBasic.Translate(1245, "BasicTruckCrate");
	NewBasic.Translate(1246, "BasicTrainCrate");
	NewBasic.Translate(1249, "BasicTiberiumGrowthEnabled");
	NewBasic.Translate(1253, "BasicFreeRadar");
	NewBasic.Translate(1257, "BasicTheme");
	NewBasic.Translate(1259, "BasicLoadingTheme");

	NewBasic.Translate(1250, "BasicHomeCell");
	NewBasic.Translate(1251, "BasicAltHomeCell");
	NewBasic.Translate(1254, "BasicRepairBaseNodes");
	NewBasic.Translate(1255, "BasicMCVRedeploys");
	NewBasic.Translate(1256, "BasicShowBriefing");
	NewBasic.Translate(2000, "BasicBriefingTheme");
	NewBasic.TranslateTitle("BasicTitle");

	NewBasic.SetControlInfo(1010, {T::Edit, "Basic", "Name", [] {}});
	NewBasic.SetControlInfo(1026, {T::Edit, "Basic", "HomeCell", [] {}});
	NewBasic.SetControlInfo(1027, {T::Edit, "Basic", "AltHomeCell", [] {}});
	NewBasic.SetControlInfo(1014, {T::Edit, "Basic", "NextScenario", [] {}});
	NewBasic.SetControlInfo(1015, {T::Edit, "Basic", "AltNextScenario", [] {}});
	NewBasic.SetControlInfo(1011, {T::Edit, "Basic", "NewINIFormat", [] {}});
	NewBasic.SetControlInfo(1012, {T::Edit, "Basic", "CarryOverCap", [] {}});
	NewBasic.SetControlInfo(1024, {T::Edit, "Basic", "MultiplayerOnly", [] {}});

	NewBasic.SetControlInfo(1239, {T::CheckBox, "Basic", "EndOfGame", [] {}});
	NewBasic.SetControlInfo(1240, {T::CheckBox, "Basic", "SkipScore", [] {}});
	NewBasic.SetControlInfo(1241, {T::CheckBox, "Basic", "OneTimeOnly", [] {}});
	NewBasic.SetControlInfo(1242, {T::CheckBox, "Basic", "SkipMapSelect", [] {}});
	NewBasic.SetControlInfo(1243, {T::CheckBox, "Basic", "Official", [] {}});
	NewBasic.SetControlInfo(1244, {T::CheckBox, "Basic", "IgnoreGlobalAITriggers", [] {}});
	NewBasic.SetControlInfo(1245, {T::CheckBox, "Basic", "TruckCrate", [] {}});
	NewBasic.SetControlInfo(1246, {T::CheckBox, "Basic", "TrainCrate", [] {}});
	NewBasic.SetControlInfo(1249, {T::CheckBox, "Basic", "TiberiumGrowthEnabled", [] {}});
	NewBasic.SetControlInfo(1253, {T::CheckBox, "Basic", "FreeRadar", [] {}});
	NewBasic.SetControlInfo(1254, {T::CheckBox, "Basic", "RepairBaseNodes", [] {}});
	NewBasic.SetControlInfo(1255, {T::CheckBox, "Basic", "MCVRedeploys", [] {}});
	NewBasic.SetControlInfo(1256, {T::CheckBox, "Basic", "ShowBriefing", [] {}});

	std::vector<FString> themeLabels;
	std::vector<FString> themeLabelsPlus;
	if (auto pSection = CINI::Theme->GetSection("Themes"))
	{
		for (const auto& [_, value] : pSection->GetEntities())
		{
			if (!value.IsEmpty())
			{
				themeLabels.push_back(value);
			}
		}
	}
	themeLabelsPlus = themeLabels;
	themeLabelsPlus.push_back("none");

	NewBasic.SetControlInfo(2001, {T::Combobox, "Basic", "BriefingTheme", [] {}, themeLabels});
	NewBasic.SetControlInfo(1258, {T::Combobox, "Basic", "Theme", [] {}, themeLabels});
	NewBasic.SetControlInfo(1260, {T::Combobox, "Basic", "LoadingTheme", [] {}, themeLabelsPlus});

	NewBasic.ShowDialog();
}

void CMiscSettings::InitNewSinglePlayer()
{
	NewSinglePlayer.SetTransparencyKey("SinglePlayerSettingsOpacity");
	NewSinglePlayer.Translate(1285, "SingleplayerDesc");

	NewSinglePlayer.Translate(1341, "SingleplayerMovies");
	NewSinglePlayer.Translate(1342, "SingleplayerIntro");
	NewSinglePlayer.Translate(1343, "SingleplayerBrief");
	NewSinglePlayer.Translate(1344, "SingleplayerWin");
	NewSinglePlayer.Translate(1345, "SingleplayerLose");
	NewSinglePlayer.Translate(1346, "SingleplayerAction");
	NewSinglePlayer.Translate(1347, "SingleplayerPostScore");
	NewSinglePlayer.Translate(1348, "SingleplayerPreMapSelect");	
	NewSinglePlayer.Translate(1338, "SingleplayerCarryOverMoney");
	NewSinglePlayer.Translate(1339, "SingleplayerTimerInherit");
	NewSinglePlayer.Translate(1340, "SingleplayerFillSilos");	
	NewSinglePlayer.Translate(1363, "SingleplayerRanking");
	NewSinglePlayer.Translate(1349, "SingleplayerParTimeEasy");
	NewSinglePlayer.Translate(1350, "SingleplayerParTimeMedium");
	NewSinglePlayer.Translate(1351, "SingleplayerParTimeHard");
	NewSinglePlayer.Translate(1352, "SingleplayerOverParTitle");
	NewSinglePlayer.Translate(1353, "SingleplayerOverParMessage");
	NewSinglePlayer.Translate(1354, "SingleplayerUnderParTitle");
	NewSinglePlayer.Translate(1355, "SingleplayerUnderParMessage");	
	NewSinglePlayer.Translate(1364, "SingleplayerGeneral");
	NewSinglePlayer.Translate(1365, "SingleplayerCampaignMoneyDeltaEasy");
	NewSinglePlayer.Translate(1367, "SingleplayerCampaignMoneyDeltaHard");
	NewSinglePlayer.Translate(1369, "SingleplayerSpyMoneyStealPercent");
	NewSinglePlayer.Translate(1371, "SingleplayerTeamDelays");
	NewSinglePlayer.Translate(1373, "SingleplayerPrismSupportModifier");
	NewSinglePlayer.Translate(1375, "SingleplayerDefaultMirageDisguises");
	NewSinglePlayer.TranslateTitle( "SingleplayerTitle");


	NewSinglePlayer.SetControlInfo(1334, {T::Edit, "Basic", "CarryOverMoney", [] {}});
	
	NewSinglePlayer.SetControlInfo(1339, {T::CheckBox, "Basic", "CarryOverTimer", [] {}});
	NewSinglePlayer.SetControlInfo(1340, {T::CheckBox, "Basic", "FillSilos", [] {}});
	
	NewSinglePlayer.SetControlInfo(1356, {T::Edit, "Ranking", "ParTimeEasy", [] {}});
	NewSinglePlayer.SetControlInfo(1357, {T::Edit, "Ranking", "ParTimeMedium", [] {}});
	NewSinglePlayer.SetControlInfo(1358, {T::Edit, "Ranking", "ParTimeHard", [] {}});
	NewSinglePlayer.SetControlInfo(1359, {T::Edit, "Ranking", "OverParTitle", [] {}});
	NewSinglePlayer.SetControlInfo(1360, {T::Edit, "Ranking", "OverParMessage", [] {}});
	NewSinglePlayer.SetControlInfo(1361, {T::Edit, "Ranking", "UnderParTitle", [] {}});
	NewSinglePlayer.SetControlInfo(1362, {T::Edit, "Ranking", "UnderParMessage", [] {}});
	
	NewSinglePlayer.SetControlInfo(1366, {T::Edit, "General", "CampaignMoneyDeltaEasy", [] {}});
	NewSinglePlayer.SetControlInfo(1368, {T::Edit, "General", "CampaignMoneyDeltaHard", [] {}});
	NewSinglePlayer.SetControlInfo(1370, {T::Edit, "General", "SpyMoneyStealPercent", [] {}});
	NewSinglePlayer.SetControlInfo(1372, {T::Edit, "General", "TeamDelays", [] {}});
	NewSinglePlayer.SetControlInfo(1374, {T::Edit, "General", "PrismSupportModifier", [] {}});
	NewSinglePlayer.SetControlInfo(1376, {T::Edit, "General", "DefaultMirageDisguises", [] {}});
	NewSinglePlayer.SetControlInfo(1377, {T::Button, "", "", [] {	
		if (!CMapData::Instance->MapWidthPlusHeight)
			return;
		CRandomTreeExt randomTree;
		auto mirageIni = CINI::CurrentDocument->GetString("General", "DefaultMirageDisguises");
		mirageIni.Trim();
		randomTree.MirageDisguiseTrees = mirageIni;
		randomTree.LaunchingFromSingleplayerSettings = true;
        if (randomTree.DoModal() == IDOK)
        {
			auto trees = CRandomTree::RandomTrees();
			FString result;
			for (size_t i = 0; i < trees.size(); ++i)
			{
				if (i > 0)
					result += ",";
				result += trees[i];
			}
			auto hEdit = NewSinglePlayer.GetControlInfo(1376).hWnd;
			SetWindowText(hEdit, result);
		}
		randomTree.MirageDisguiseTrees = "";
		randomTree.LaunchingFromSingleplayerSettings = false;
	}});
	
	std::vector<FString> labels;
	if (auto pSection = CINI::Art->GetSection("Movies"))
	{
		for (const auto& [_, value] : pSection->GetEntities())
		{
			if (!value.IsEmpty())
			{
				labels.push_back(value);
			}
		}
	}

	NewSinglePlayer.SetControlInfo(1327, {T::Combobox, "Basic", "Intro", [] {}, labels});
	NewSinglePlayer.SetControlInfo(1328, {T::Combobox, "Basic", "Brief", [] {}, labels});
	NewSinglePlayer.SetControlInfo(1329, {T::Combobox, "Basic", "Win", [] {}, labels});
	NewSinglePlayer.SetControlInfo(1330, {T::Combobox, "Basic", "Lose", [] {}, labels});
	NewSinglePlayer.SetControlInfo(1170, {T::Combobox, "Basic", "Action", [] {}, labels});
	NewSinglePlayer.SetControlInfo(1332, {T::Combobox, "Basic", "PostScore", [] {}, labels});
	NewSinglePlayer.SetControlInfo(1333, {T::Combobox, "Basic", "PreMapSelect", [] {}, labels});

	NewSinglePlayer.ShowDialog();
}
