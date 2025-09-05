#include <Helpers/Macro.h>
#include <./../FA2sp/Logger.h>
#include <./../FA2sp/FA2sp.h>

// DEFINE_HOOK(54FC1E, CFileDialog_EnableExplorerStyle, 7)
// {
//     GET(CFileDialog*, pDialog, ESI);
// 
//     OPENFILENAME ofn = pDialog->m_ofn;
// 
//     ofn.Flags &= ~OFN_ENABLEHOOK;
// 
//     if (ofn.pvReserved)
//         R->EAX(GetOpenFileNameA(&ofn));
//     else
//         R->EAX(GetSaveFileNameA(&ofn));
// 
//     return 0x54FC37;
// }
DEFINE_HOOK(4248B3, CFinalSunDlg_OpenMap_ChangeDialogStyle, 7)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x60C, (0x398 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(4248DE, CFinalSunDlg_OpenMap_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() != 2)
        return 0x4248F0;
    return 0x4248E3;
}

DEFINE_HOOK(42686A, CFinalSunDlg_SaveMap_ChangeDialogStyle, 5)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x3CC, (0x280 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(426897, CFinalSunDlg_SaveMap_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() == 2)
        return 0x42698C;
    return 0x4268A0;
}

DEFINE_HOOK(4D312E, CFinalSunDlg_ImportMap_ChangeDialogStyle, 5)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x310, (0x280 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(4D3158, CFinalSunDlg_ImportMap_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() != 2)
        return 0x4D316A;
    return 0x4D315D;
}

DEFINE_HOOK(40B7B3, CINIEditor_OnClickImportINI_ChangeDialogStyle, 5)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x3B0, (0x280 - 0x8)), OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(40B7CD, CINIEditor_OnClickImportINI_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() == 2)
        return 0x40B865;
    return 0x40B7D6;
}
