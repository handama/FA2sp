#include <Helpers/Macro.h>
#include <Miscs/Miscs.LoadParams.h>
#include <CMapData.h>

#include "Body.h"
#include "../../FA2sp.h"

#include "../CFinalSunDlg/Body.h"

DEFINE_HOOK(50F000, CPropertyUnit_OnInitDialog, 7)
{
    GET(CPropertyUnit*, pThis, ECX);

    pThis->ppmfc::CDialog::OnInitDialog();

    CMapData::Instance->UpdateCurrentDocument();

    if (!CMapData::Instance->IsMultiOnly())
    {
        Miscs::LoadParams::Houses(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1079)), false, false, false);
    }
    else
    {
        if (ExtConfigs::TestNotLoaded)
        {

        }
        else if (ExtConfigs::PlayerAtXForTechnos)
            Miscs::LoadParams::Houses(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1079)), false, false, true);
        else
            Miscs::LoadParams::Houses(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1079)), false, false, false);
    }
    Miscs::LoadParams::Tags(reinterpret_cast<ppmfc::CComboBox*>(pThis->GetDlgItem(1083)), true);

    pThis->CSCStrength.SetRange(0, 256);
    pThis->CSCStrength.SetPos(atoi(pThis->CString_HealthPoint));
    pThis->UpdateData(FALSE);
    pThis->GetDlgItem(1088)->SetWindowTextA(pThis->CString_Direction);
    pThis->GetDlgItem(1285)->ShowWindow(FALSE);

    if (!CViewObjectsExt::InitPropertyDlgFromProperty)
    {
        for (int i = 1300; i <= 1310; ++i)
            pThis->GetDlgItem(i)->ShowWindow(SW_HIDE);
    }

    pThis->Translate();

    return 0x50F084;
}