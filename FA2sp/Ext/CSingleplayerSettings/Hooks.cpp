#include "Body.h"
#include "../../Miscs/Hooks.INI.h"

#include "../../Helpers/Translations.h"

#include <Helpers/Macro.h>
#include <CINI.h>

DEFINE_HOOK(4DBC5C, CSingleplayerSettings_Translate, 5)
{
    GET(CSingleplayerSettings*, pThis, ESI);

    Translations::TranslateItem(pThis, 1349, "SingleplayerParTimeEasy");
    Translations::TranslateItem(pThis, 1350, "SingleplayerParTimeMedium");
    Translations::TranslateItem(pThis, 1351, "SingleplayerParTimeHard");
    Translations::TranslateItem(pThis, 1352, "SingleplayerOverParTitle");
    Translations::TranslateItem(pThis, 1353, "SingleplayerOverParMessage");
    Translations::TranslateItem(pThis, 1354, "SingleplayerUnderParTitle");
    Translations::TranslateItem(pThis, 1355, "SingleplayerUnderParMessage");
    Translations::TranslateItem(pThis, 1363, "SingleplayerRanking");
    Translations::TranslateItem(pThis, 1364, "SingleplayerGeneral");
    Translations::TranslateItem(pThis, 1365, "SingleplayerCampaignMoneyDeltaEasy");
    Translations::TranslateItem(pThis, 1367, "SingleplayerCampaignMoneyDeltaHard");
    Translations::TranslateItem(pThis, 1369, "SingleplayerSpyMoneyStealPercent");
    Translations::TranslateItem(pThis, 1371, "SingleplayerTeamDelays");
    Translations::TranslateItem(pThis, 1373, "SingleplayerPrismSupportModifier");
    Translations::TranslateItem(pThis, 1375, "SingleplayerDefaultMirageDisguises");
    Translations::TranslateItem(pThis, 1400, "SingleplayerSave");

    return 0;
}

DEFINE_HOOK(4DA1A6, CSingleplayerSettings_UpdateDialog, 5)
{
    GET(CSingleplayerSettings*, pThis, EDI);

    pThis->SetDlgItemText(1356, CINIExt::CurrentDocument->GetString("Ranking", "ParTimeEasy"));
    pThis->SetDlgItemText(1357, CINIExt::CurrentDocument->GetString("Ranking", "ParTimeMedium"));
    pThis->SetDlgItemText(1358, CINIExt::CurrentDocument->GetString("Ranking", "ParTimeHard"));
    pThis->SetDlgItemText(1359, CINIExt::CurrentDocument->GetString("Ranking", "OverParTitle"));
    pThis->SetDlgItemText(1360, CINIExt::CurrentDocument->GetString("Ranking", "OverParMessage"));
    pThis->SetDlgItemText(1361, CINIExt::CurrentDocument->GetString("Ranking", "UnderParTitle"));
    pThis->SetDlgItemText(1362, CINIExt::CurrentDocument->GetString("Ranking", "UnderParMessage"));
    pThis->SetDlgItemText(1366, CINIExt::CurrentDocument->GetString("General", "CampaignMoneyDeltaEasy"));
    pThis->SetDlgItemText(1368, CINIExt::CurrentDocument->GetString("General", "CampaignMoneyDeltaHard"));
    pThis->SetDlgItemText(1370, CINIExt::CurrentDocument->GetString("General", "SpyMoneyStealPercent"));
    pThis->SetDlgItemText(1372, CINIExt::CurrentDocument->GetString("General", "TeamDelays"));
    pThis->SetDlgItemText(1374, CINIExt::CurrentDocument->GetString("General", "PrismSupportModifier"));
    pThis->SetDlgItemText(1376, CINIExt::CurrentDocument->GetString("General", "DefaultMirageDisguises"));

    return 0;
}