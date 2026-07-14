#include <Helpers/Macro.h>
#include "../../FA2sp/Helpers/Translations.h"
#include "../../FA2sp/Helpers/Helper.h"
#include "../../FA2sp/Ext/CMapData/Body.h"
#include <Miscs/Miscs.LoadParams.h>
#include <CMapData.h>

#include "Body.h"
#include "../../FA2sp.h"

#include "../CFinalSunDlg/Body.h"

DEFINE_HOOK(4014D0, CPropertyAircraft_OnInitDialog, 7)
{
    GET(CPropertyAircraft*, pThis, ECX);

    pThis->ppmfc::CDialog::OnInitDialog();

    CMapData::Instance->UpdateCurrentDocument();

    TempValueHolder temp(CMapDataExt::IsInitingPropertyDialog, true);
	if (!CMapData::Instance->IsMultiOnly())
	{
        Miscs::LoadParams::Houses(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1079)), false, false, false);
    }
    else
    {
        if (!ExtConfigs::TestNotLoaded)
        {
            if (ExtConfigs::PlayerAtXForTechnos)
                Miscs::LoadParams::Houses(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1079)), false, true, true);
            else
                Miscs::LoadParams::Houses(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1079)), false, true, false);
        }
    }
    Miscs::LoadParams::Tags(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1083)), true);

    pThis->CSCStrength.SetRange(0, 256);
    pThis->CSCStrength.SetPos(atoi(pThis->CString_HealthPoint));
    pThis->UpdateData(FALSE);
    pThis->GetDlgItem(1088)->SetWindowTextA(pThis->CString_Direction);
    pThis->GetDlgItem(1233)->ShowWindow(FALSE);

    if (!CViewObjectsExt::InitPropertyDlgFromProperty)
    {
        for (int i = 1300; i <= 1308; ++i)
            pThis->GetDlgItem(i)->ShowWindow(SW_HIDE);
    }

    pThis->Translate();

    return 0x401569;
}
DEFINE_HOOK(4448F0, Miscs_LoadParams_Houses, 7)
{
	GET_STACK(ppmfc::CComboBox*, cb, 0x4);
    GET_STACK(bool, bNumbers, 0x8);
    GET_STACK(bool, bCountries, 0xC);
    GET_STACK(bool, bPlayers, 0x10);

	int sel= cb->GetCurSel();
	auto pCountry = CINI::Rules->GetSection("Countries");
	int crulesh = pCountry ? pCountry->GetEntities().size() : 0;

    if (!CMapData::Instance->IsMultiOnly())
	    bPlayers = false;

	while(cb->DeleteString(0) != CB_ERR);

    auto pHouse = CINI::CurrentDocument->GetSection("Houses");
	INISection* pSecton = nullptr;
	
	if (CMapDataExt::IsInitingPropertyDialog) 
        pSecton = CMapData::Instance->IsMultiOnly() ? pCountry : pHouse;
    else
        pSecton = bCountries ? pCountry : pHouse;

    int insertIndex = 0;
    if (pSecton)
    {
		if (bPlayers)
		{
			if (bNumbers)
			{
				cb->InsertString(insertIndex++, "4475 <Player @ A>");
				cb->InsertString(insertIndex++, "4476 <Player @ B>");
				cb->InsertString(insertIndex++, "4477 <Player @ C>");
				cb->InsertString(insertIndex++, "4478 <Player @ D>");
				cb->InsertString(insertIndex++, "4479 <Player @ E>");
				cb->InsertString(insertIndex++, "4480 <Player @ F>");
				cb->InsertString(insertIndex++, "4481 <Player @ G>");
				cb->InsertString(insertIndex++, "4482 <Player @ H>");								
			}
			else
			{
				cb->InsertString(insertIndex++, "<Player @ A>");
				cb->InsertString(insertIndex++, "<Player @ B>");
				cb->InsertString(insertIndex++, "<Player @ C>");
				cb->InsertString(insertIndex++, "<Player @ D>");
				cb->InsertString(insertIndex++, "<Player @ E>");
				cb->InsertString(insertIndex++, "<Player @ F>");
				cb->InsertString(insertIndex++, "<Player @ G>");
				cb->InsertString(insertIndex++, "<Player @ H>");
			}		
		}

		int index = 0;
		for (const auto& [key, value] : pSecton->GetEntities())
		{
			if (value == "Nod" || value == "GDI")
				continue;

			FString text;
			if (bNumbers)
			{
				text.Format("%d %s", index++, Translations::ParseHouseName(value, true));
			}
			else
            {
                if (ExtConfigs::ObjectBrowser_SafeHouses 
                    && CMapDataExt::IsInitingPropertyDialog
                    && pSecton == pCountry)
                {
                    if (value != "Neutral" && value != "Special")
                        continue;
                }
				text = Translations::ParseHouseName(value, true);
			}

			cb->InsertString(insertIndex++, text);
		}
	}
	else if (bPlayers)
	{
		if(bNumbers)
		{
            cb->InsertString(insertIndex++, "4475 <Player @ A>");
            cb->InsertString(insertIndex++, "4476 <Player @ B>");
            cb->InsertString(insertIndex++, "4477 <Player @ C>");
            cb->InsertString(insertIndex++, "4478 <Player @ D>");
            cb->InsertString(insertIndex++, "4479 <Player @ E>");
            cb->InsertString(insertIndex++, "4480 <Player @ F>");
            cb->InsertString(insertIndex++, "4481 <Player @ G>");
            cb->InsertString(insertIndex++, "4482 <Player @ H>");
		}
		else
		{
            cb->InsertString(insertIndex++, "<Player @ A>");
            cb->InsertString(insertIndex++, "<Player @ B>");
            cb->InsertString(insertIndex++, "<Player @ C>");
            cb->InsertString(insertIndex++, "<Player @ D>");
            cb->InsertString(insertIndex++, "<Player @ E>");
            cb->InsertString(insertIndex++, "<Player @ F>");
            cb->InsertString(insertIndex++, "<Player @ G>");
            cb->InsertString(insertIndex++, "<Player @ H>");						
		}
	}

	if(sel>=0) cb->SetCurSel(sel);

	return 0x445B22;
}
