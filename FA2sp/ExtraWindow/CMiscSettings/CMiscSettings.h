#pragma once

#include "../Common.h"

class CMiscSettings
{
public:
  	static CINIDialog NewSpecialFlags;
	static void InitNewSpecialFlags();

  	static CINIDialog NewBasic;
	static void InitNewBasic();

  	static CINIDialog NewSinglePlayer;
	static void InitNewSinglePlayer();
};
