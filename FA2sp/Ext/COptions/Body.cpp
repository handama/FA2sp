#include "Body.h"
#include <CINI.h>
#include "../../Logger.h"
#include "../../Helpers/Translations.h"
#include <CFinalSunDlg.h>
#include <CFinalSunApp.h>

COptions* COptionsExt::Instance = nullptr;
FString COptionsExt::Language = "English";

void COptionsExt::ProgramStartupInit()
{
    RunTime::ResetMemoryContentAt(0x5984C8, &COptionsExt::OnCommandExt);
}

BOOL COptionsExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
    WORD wmID = LOWORD(wParam);
    WORD wmMsg = HIWORD(wParam);

    if (wmID == 1231 && wmMsg == CBN_SELCHANGE)
    {
        auto hWnd = this->GetDlgItem(1231)->GetSafeHwnd();
        char buffer[512]{ 0 };
        int nSel = ::SendMessage(hWnd, CB_GETCURSEL, NULL, NULL);
        if (nSel >= 0)
        {
            auto lang = CINI::FALanguage->GetValueAt("Languages", nSel);
            FString backup = CFinalSunApp::Instance->Language;
            CFinalSunApp::Instance->Language = lang;
            COptionsExt::Language = lang;
            Translations::TranslateDialog(*this);
            CFinalSunApp::Instance->Language = backup;
        }
    }

    return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}

DEFINE_HOOK(50E55F, COptions_OnInitDialog, 8)
{
    GET(COptionsExt*, pThis, EDI);
    auto hLang = pThis->GetDlgItem(1231)->GetSafeHwnd();

    if (auto pSection = CINI::FALanguage->GetSection("Languages"))
    {
        int index = 0;
        int i = 0;
        for (const auto& [key, value] : pSection->GetEntities())
        {
            auto header = value + "Header";
            auto name = CINI::FALanguage->GetString(header, "Name");
            if (!name.IsEmpty())
            {
                ::SendMessage(hLang, CB_ADDSTRING, NULL, (LPARAM)(LPCSTR)name);
                if (CFinalSunApp::Instance->Language == value)
                    index = i;
            }
            i++;
        }
        ::SendMessage(hLang, CB_SETCURSEL, index, NULL);
    }

    return 0x50E8A8;
}

DEFINE_HOOK(50E342, COptions_OnOK, 6)
{
    GET(COptionsExt*, pThis, ESI);
    pThis->m_LanguageName = COptionsExt::Language;
    if (CFinalSunApp::Instance->Language != COptionsExt::Language)
    {
        CFinalSunApp::Instance->Language = COptionsExt::Language;

        strcpy_s(Translations::pLanguage[0], COptionsExt::Language);
        strcpy_s(Translations::pLanguage[1], COptionsExt::Language);
        strcpy_s(Translations::pLanguage[2], COptionsExt::Language);
        strcpy_s(Translations::pLanguage[3], COptionsExt::Language);
        strcat_s(Translations::pLanguage[0], "-StringsRA2");
        strcat_s(Translations::pLanguage[1], "-TranslationsRA2");
        strcat_s(Translations::pLanguage[2], "-Strings");
        strcat_s(Translations::pLanguage[3], "-Translations");

        if (CFinalSunDlg::Instance)
        {
            ::MessageBox(CFinalSunDlg::Instance->GetSafeHwnd(),
                Translations::TranslateOrDefault("ChangeLanguageMessage",
                    "Please restart FA2 to apply language changes."),
                "FA2sp",
                MB_OK | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);
        }
    }

    return 0x50E43F;
}
