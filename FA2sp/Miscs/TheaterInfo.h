#pragma once

#include <vector>
#include <set>

struct InfoStruct
{
	unsigned short Ramp;
	unsigned short Morphable;
	unsigned short RampIndex;
	unsigned short MorphableIndex;
};

class TheaterInfo
{
public:
	static const char* GetInfoSection();
	static void UpdateTheaterInfo();

	static std::vector<InfoStruct> CurrentInfo;
	static std::vector<InfoStruct> CurrentInfoNonMorphable;
	static std::set<int> CurrentBigWaters;
	static std::set<int> CurrentSmallWaters;
	static bool CurrentInfoHasCliff2;
};