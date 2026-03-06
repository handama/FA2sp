#include "CNewTipsOfTheDay.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"
#include "../../Ext/CLoading/Body.h"
#include <CFinalSunApp.h>

CNewTipsOfTheDay* CNewTipsOfTheDay::m_pNewTipsOfTheDayDlg = nullptr;

static bool ReplaceBitmapColor(
    HBITMAP hBmp,
    COLORREF fromColor,
    COLORREF toColor)
{
    if (!hBmp)
        return false;

    DIBSECTION ds = {};
    if (GetObject(hBmp, sizeof(DIBSECTION), &ds) != sizeof(DIBSECTION))
        return false;

    if (!ds.dsBm.bmBits)
        return false;

    int width = ds.dsBm.bmWidth;
    int height = ds.dsBm.bmHeight;
    int bpp = ds.dsBm.bmBitsPixel;
    int stride = ds.dsBm.bmWidthBytes;
    BYTE* bits = (BYTE*)ds.dsBm.bmBits;

    BYTE fromR = GetRValue(fromColor);
    BYTE fromG = GetGValue(fromColor);
    BYTE fromB = GetBValue(fromColor);

    BYTE toR = GetRValue(toColor);
    BYTE toG = GetGValue(toColor);
    BYTE toB = GetBValue(toColor);

    if (bpp == 24)
    {
        for (int y = 0; y < height; ++y)
        {
            BYTE* row = bits + y * stride;

            for (int x = 0; x < width; ++x)
            {
                BYTE* px = row + x * 3;

                BYTE b = px[0];
                BYTE g = px[1];
                BYTE r = px[2];

                if (r == fromR && g == fromG && b == fromB)
                {
                    px[0] = toB;
                    px[1] = toG;
                    px[2] = toR;
                }
            }
        }
    }
    else if (bpp == 32)
    {
        for (int y = 0; y < height; ++y)
        {
            DWORD* row = (DWORD*)(bits + y * stride);

            for (int x = 0; x < width; ++x)
            {
                BYTE* px = (BYTE*)&row[x];

                BYTE b = px[0];
                BYTE g = px[1];
                BYTE r = px[2];

                if (r == fromR && g == fromG && b == fromB)
                {
                    px[0] = toB;
                    px[1] = toG;
                    px[2] = toR;
                }
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

void CNewTipsOfTheDay::ShowNewTipsOfTheDay()
{
	if (m_pNewTipsOfTheDayDlg == nullptr || !::IsWindow(m_pNewTipsOfTheDayDlg->m_hWnd))
	{
		m_pNewTipsOfTheDayDlg = new CNewTipsOfTheDay();   

		if (!m_pNewTipsOfTheDayDlg->Create(335, CFinalSunDlg::Instance)) 
		{
			delete m_pNewTipsOfTheDayDlg;
			m_pNewTipsOfTheDayDlg = nullptr;
			Logger::Error("Failed to create CNewTipsOfTheDay.\n");
			return;
		}
	}
	m_pNewTipsOfTheDayDlg->ShowWindow(SW_SHOW);    
	::SetWindowPos(m_pNewTipsOfTheDayDlg->GetSafeHwnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

CNewTipsOfTheDay::CNewTipsOfTheDay(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(335, pParent)
{
    b_ShowAtStartup = !ExtConfigs::SkipTipsOfTheDay;
    m_Tips.clear();
}

void CNewTipsOfTheDay::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, 1000, m_staticPic);
	DDX_Control(pDX, 1002, m_staticText);
    DDX_Check(pDX, 1001, b_ShowAtStartup);
}

BOOL CNewTipsOfTheDay::OnInitDialog()
{
	CDialog::OnInitDialog();
	HBITMAP hBmp = (HBITMAP)LoadImage(
		static_cast<HINSTANCE>(FA2sp::hInstance), 
		MAKEINTRESOURCE(1037), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    ReplaceBitmapColor(hBmp, RGB(255, 255, 255), ::GetSysColor(COLOR_3DFACE));

	m_staticPic.SendMessage(STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
	ExtraWindow::SetEditControlFontSize(m_staticText.GetSafeHwnd(), 1.4f);
	ExtraWindow::SetEditControlFontSize(GetDlgItem(1004)->GetSafeHwnd(), 1.2f);

	FString buffer;

	if (Translations::GetTranslationItem("TipDialogNext", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("TipDialogPrevious", buffer))
		GetDlgItem(1005)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("TipDialogClose", buffer))
		GetDlgItem(2)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("TipDialogShowAtStartup", buffer))
		GetDlgItem(1001)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("CG_IDS_DIDYOUKNOW", buffer))
		GetDlgItem(1002)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("TipDialogCaption", buffer))
		SetWindowTextA(buffer);

    FString langHeader = FinalAlertConfig::Language + "Header";
    FString Ext = CINI::FALanguage->GetString(langHeader, "ExtensionName", "ENG");

    FString tipFile = "TIPS.";
    tipFile += Ext;

    DWORD fileSize;
    if (auto pBuffer = static_cast<byte*>(CLoadingExt::GetExtension()->ReadWholeFile(tipFile, &fileSize, true))) {
        auto encoding = STDHelpers::GetFileEncoding(pBuffer, fileSize);
        bool loadAsUTF8 = ExtConfigs::UTF8Support_InferEncoding && encoding == UTF8 || encoding == UTF8_BOM;
        FString content(reinterpret_cast<char*>(pBuffer), fileSize);
        if (loadAsUTF8)
            content.toANSI();
        std::istringstream iss(content);
        FString line;
        while (std::getline(iss, line))
        {
            if (STDHelpers::IsNullOrWhitespaceOrReturn(line)) continue;
            m_Tips.push_back(FString::ReplaceSpeicalString(line));
        }
    }

    CINI ini;
    std::string path = CFinalSunApp::Instance->ExePath();
    path += "\\FinalAlert.ini";
    ini.ClearAndLoad(path.c_str());
    m_CurrentPos = ini.GetInteger("Tip", "FilePos");

    if (m_CurrentPos < 0 || m_CurrentPos >= m_Tips.size())
        m_CurrentPos = 0;

    if (!m_Tips.empty())
    {
        ::SendMessage(GetDlgItem(1004)->GetSafeHwnd(), WM_SETTEXT, 0, m_Tips[m_CurrentPos]);
    }

	return TRUE;
}

BOOL CNewTipsOfTheDay::OnCommand(WPARAM wParam, LPARAM lParam)
{
    WORD nID = LOWORD(wParam);
    WORD nNotify = HIWORD(wParam);

    if (nNotify == BN_CLICKED)
    {
        switch (nID)
        {
        case 1005:  OnPrevious(); return TRUE;
        }
    }

    return CDialog::OnCommand(wParam, lParam);
}

void CNewTipsOfTheDay::OnOK()
{
    if (!m_Tips.empty())
    {
        m_CurrentPos++;
        if (m_CurrentPos >= m_Tips.size()) m_CurrentPos = 0;

        ::SendMessage(GetDlgItem(1004)->GetSafeHwnd(), WM_SETTEXT, 0, m_Tips[m_CurrentPos]);
    }
}

void CNewTipsOfTheDay::OnPrevious()
{
    if (!m_Tips.empty())
    {
        m_CurrentPos--;
        if (m_CurrentPos < 0) m_CurrentPos = m_Tips.size() - 1;

        ::SendMessage(GetDlgItem(1004)->GetSafeHwnd(), WM_SETTEXT, 0, m_Tips[m_CurrentPos]);
    }
}

void CNewTipsOfTheDay::OnCancel()
{
	UpdateData(TRUE);

    CINI ini;
    std::string path = CFinalSunApp::Instance->ExePath();
    path += "\\FinalAlert.ini";
    ini.ClearAndLoad(path.c_str());
    ini.WriteString("Tip", "StartUp", b_ShowAtStartup == TRUE ? "0" : "1");
    ExtConfigs::SkipTipsOfTheDay = b_ShowAtStartup != TRUE;
    ini.WriteString("Options", "SkipTipsOfTheDay", ExtConfigs::SkipTipsOfTheDay ? "yes" : "no");
    // starts from n+1
    ini.WriteString("Tip", "FilePos", STDHelpers::IntToString(m_CurrentPos + 1));
    ini.WriteToFile(path.c_str());

	DestroyWindow();
}

void CNewTipsOfTheDay::OnClose()
{
    OnCancel();
}

void CNewTipsOfTheDay::PostNcDestroy()
{
	delete this;
	m_pNewTipsOfTheDayDlg = nullptr;
}