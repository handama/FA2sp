#include <Helpers/Macro.h>
#include "../../Helpers/Translations.h"
#include <CMapData.h>
#include <CFinalSunDlg.h>
#include <CMyViewFrame.h>
#include <CIsoView.h>

DEFINE_HOOK(412E37, CBasic_RestoreTrainCrate, 7)
{
	return 0x412E61;
}
