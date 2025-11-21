#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/Translations.h"

class CMapRendererBatchDlg : public ppmfc::CDialog
{
public:
    CMapRendererBatchDlg(CWnd* pParent = NULL);
    ppmfc::CString m_Text;
    virtual int DoModal() override
    {
        HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
        if (!hModule)
            ::MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll!"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

        return ppmfc::CDialog::DoModal();
    }

protected:
    virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->hwnd == GetDlgItem(1000)->GetSafeHwnd())
        {
            GetDlgItem(1000)->SendMessage(EM_REPLACESEL, FALSE, (LPARAM)"\n");
            return TRUE;
        }
        return ppmfc::CDialog::PreTranslateMessage(pMsg);
    }
};

class CMapRendererDlg : public ppmfc::CDialog
{
public:
	CMapRendererDlg(CWnd* pParent = NULL);
	BOOL b_LocalSize;
	BOOL b_GameLayers;
	BOOL b_SaveFormat;
	int n_Lighting;
	BOOL b_DisplayInvisibleOverlay;
	BOOL b_MarkStartPositions;
	BOOL b_MarkOres;
	BOOL b_IgnoreObjects;
	BOOL b_Tube;
    std::vector<FString> BatchPaths;
    ppmfc::CString m_Text;

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void OnOK();
	virtual void OnCancel();
    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if (pMsg->message == WM_LBUTTONUP && pMsg->hwnd == GetDlgItem(4000)->GetSafeHwnd())
        {
            CMapRendererBatchDlg dlg;
            dlg.m_Text = m_Text;
            dlg.DoModal();
            BatchPaths = FString::SplitStringMultiSplit(dlg.m_Text, "\n|\r");
            for (auto& p : BatchPaths) {
                p.Replace("\"", "");
            }
            m_Text = dlg.m_Text;
            return TRUE;
        }
        return ppmfc::CDialog::PreTranslateMessage(pMsg);
    }
};

