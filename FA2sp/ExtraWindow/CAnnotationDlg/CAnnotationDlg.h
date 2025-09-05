#pragma once

#include <FA2PP.h>
#include "../../Helpers/Translations.h"

class CAnnotationDlg : public ppmfc::CDialog
{
public:
	CAnnotationDlg(CWnd* pParent = NULL);
	ppmfc::CString m_Text;
	ppmfc::CString m_FontSize;
	ppmfc::CString m_TextColor;
	ppmfc::CString m_BgColor;
    BOOL m_Bold;

    virtual int DoModal() override
    {
        HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
        if (!hModule)
            ::MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll£¡"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

        return ppmfc::CDialog::DoModal();
    }

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
    COLORREF ToCOLORREF(const ppmfc::CString& hexStr)
    {
        unsigned int rgb = strtoul(hexStr.Mid(2), nullptr, 16);
        BYTE r = (rgb >> 16) & 0xFF;
        BYTE g = (rgb >> 8) & 0xFF;
        BYTE b = rgb & 0xFF;
        return RGB(r, g, b);
    }
    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->hwnd == GetDlgItem(1000)->GetSafeHwnd())
        {
            GetDlgItem(1000)->SendMessage(EM_REPLACESEL, FALSE, (LPARAM)"\n");
            return TRUE;
        }
        else if (pMsg->message == WM_LBUTTONUP &&
            (pMsg->hwnd == GetDlgItem(1004)->GetSafeHwnd() || pMsg->hwnd == GetDlgItem(1006)->GetSafeHwnd()))
        {
            UINT id = (pMsg->hwnd == GetDlgItem(1004)->GetSafeHwnd()) ? 1004 : 1006;

            CHOOSECOLOR cc;
            static COLORREF acrCustClr[16];
            ZeroMemory(&cc, sizeof(cc));
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = m_hWnd;
            cc.lpCustColors = acrCustClr;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            cc.rgbResult = (id == 1004) ? ToCOLORREF(m_TextColor) : ToCOLORREF(m_BgColor);

            if (ChooseColor(&cc))
            {
                BYTE r = GetRValue(cc.rgbResult);
                BYTE g = GetGValue(cc.rgbResult);
                BYTE b = GetBValue(cc.rgbResult);
                char buf[16];
                sprintf_s(buf, "0x%02X%02X%02X", r, g, b);

                if (id == 1004) m_TextColor = buf;
                else m_BgColor = buf;

                InvalidateRect(GetDlgItem(id)->GetSafeHwnd(), NULL, TRUE);
            }
        }
        else if (pMsg->message == WM_PAINT &&
            (pMsg->hwnd == GetDlgItem(1004)->GetSafeHwnd() || pMsg->hwnd == GetDlgItem(1006)->GetSafeHwnd()))
        {
            UINT id = (pMsg->hwnd == GetDlgItem(1004)->GetSafeHwnd()) ? 1004 : 1006;
            HWND hWnd = pMsg->hwnd;
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);

            RECT rc;
            ::GetClientRect(hWnd, &rc);

            COLORREF color = (id == 1004) ? ToCOLORREF(m_TextColor)
                : ToCOLORREF(m_BgColor);

            HBRUSH brush = CreateSolidBrush(color);
            FillRect(hDC, &rc, brush);
            FrameRect(hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

            DeleteObject(brush);
            EndPaint(hWnd, &ps);

            return TRUE;
        }
        else if (pMsg->message == WM_TIMER && pMsg->wParam == m_nInitTimer)
        {
            KillTimer(GetSafeHwnd(), m_nInitTimer);
            m_nInitTimer = NULL;

            InvalidateRect(GetDlgItem(1004)->GetSafeHwnd(), NULL, TRUE);
            InvalidateRect(GetDlgItem(1006)->GetSafeHwnd(), NULL, TRUE);
            return TRUE;
        }

        return ppmfc::CDialog::PreTranslateMessage(pMsg);
    }
    UINT_PTR m_nInitTimer;
};
