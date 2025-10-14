#include "Body.h"
#include "../../ExtraWindow/CTriggerAnnotation/CTriggerAnnotation.h"
#include "../../Helpers/STDHelpers.h"

CTags* CTagsExt::Instance = nullptr;

void CTagsExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x596A60, &CTagsExt::PreTranslateMessageExt);
}

BOOL CTagsExt::PreTranslateMessageExt(MSG* pMsg)
{
	switch (pMsg->message) {
	default:
		break;
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}

DEFINE_HOOK(4DEF80, OnCBCurrentTagSelectedChanged_Annotation, 6)
{
    GET(CTagsExt*, pThis, ECX);
    int index = pThis->CCBTagList.GetCurSel();
    if (index != CB_ERR)
    {
        ppmfc::CString text;
        pThis->CCBTagList.GetLBText(index, text);
        STDHelpers::TrimIndex(text);
        if (!text.IsEmpty())
        {
            CTriggerAnnotation::Type = AnnoTag;
            CTriggerAnnotation::ID = text;
            ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
        }
    }
    return 0;
}