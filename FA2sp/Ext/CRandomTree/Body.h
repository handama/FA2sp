#pragma once

#include <CRandomTree.h>
#include "../FA2Expand.h"
#include "../../Helpers/FString.h"

class NOVTABLE CRandomTreeExt : public CRandomTree
{
public:
	static void ProgramStartupInit();

	CRandomTreeExt() {};
	~CRandomTreeExt() {};
	
	static FString MirageDisguiseTrees;
	static bool LaunchingFromSingleplayerSettings;
private:
};