#include "Body.h"
#include "../../ExtraWindow/CTriggerAnnotation/CTriggerAnnotation.h"
#include "../../Helpers/STDHelpers.h"
#include "../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../Helpers/Translations.h"

CTags* CTagsExt::Instance = nullptr;

void CTagsExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x596A60, &CTagsExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x596A8C, &CTagsExt::OnInitDialogExt);
}

BOOL CTagsExt::PreTranslateMessageExt(MSG* pMsg)
{
	switch (pMsg->message) {
	default:
		break;
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}

BOOL CTagsExt::OnInitDialogExt()
{
	auto result = this->ppmfc::CDialog::OnInitDialog();

    this->CCBRepeat.DeleteAllStrings();
    this->CCBRepeat.InsertString(0, (FString("0 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeOr", "One Time OR")));
    this->CCBRepeat.InsertString(1, (FString("1 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeAnd", "One Time AND")));
    this->CCBRepeat.InsertString(2, (FString("2 - ") + Translations::TranslateOrDefault("TriggerRepeatType.RepeatingOr", "Repeating OR")));

    return result;
}

DEFINE_HOOK(4DEF80, CTags_OnCBCurrentTagSelectedChanged_Annotation, 6)
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

DEFINE_HOOK(4E117C, CTags_OnDelete_Notify, 5)
{
    CNewTeamTypes::TagListChanged = true;
    return 0;
}

DEFINE_HOOK(4E1461, CTags_OnNew_Notify, 6)
{
    CNewTeamTypes::TagListChanged = true;
    return 0;
}

DEFINE_HOOK(4DF567, CTags_OnEditName_Notify, 6)
{
    CNewTeamTypes::TagListChanged = true;
    return 0;
}