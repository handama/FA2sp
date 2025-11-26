#pragma once

#include <FA2PP.h>

#include <sstream>
#include <algorithm>
#include <vector>
#include <set>
#include <random>

#include <MFC/ppmfc_cstring.h>
#include "../FA2sp.h"
#include "FString.h"

#include <CINI.h>
#include <Miscs/Miscs.h>

// A class uses STL containers for assistance use

class STDHelpers
{
public:
    static std::vector<ppmfc::CString> SplitString(const ppmfc::CString& pSource, const char* pSplit = ",");
    static std::vector<ppmfc::CString> SplitStringMultiSplit(const ppmfc::CString& pSource, const char* pSplit = ",|.");
    static std::pair<ppmfc::CString, ppmfc::CString> SplitKeyValue(const ppmfc::CString& pSource);
    static std::vector<ppmfc::CString> SplitString(const ppmfc::CString& pSource, size_t nth, const char* pSplit = ",");
    static std::vector<ppmfc::CString> SplitStringAction(const ppmfc::CString& pSource, size_t nth, const char* pSplit = ",");
    static std::vector<ppmfc::CString> SplitStringTrimmed(const ppmfc::CString& pSource, const char* pSplit = ",");
    static int ParseToInt(const char* pSource, int nDefault = 0);
    static ppmfc::CString IntToString(int source, const char* format = "%d");
    static bool IsNullOrEmpty(const char* pSource);
    static bool IsNullOrWhitespace(const char* pSource);
    static bool IsNullOrWhitespaceOrReturn(const char* pSource);
    static bool IsNoneOrEmpty(const char* pSource);
    static void TrimString(ppmfc::CString& str);
    static ppmfc::CString GetTrimString(ppmfc::CString str);
    static void TrimIndex(ppmfc::CString& str);
    static void TrimSemicolon(ppmfc::CString& str);
    static void TrimIndexElse(ppmfc::CString& str);
    static bool Contains(ppmfc::CString pStr, ppmfc::CString pQuery, bool bIgnoreCase = false);
    static ppmfc::CString GetComboBoxText(const ppmfc::CComboBox& cbb);
    static bool IsNumber(const std::string& str);
    static bool IsNumber(const char * str);
	static bool IsTrue(const char* str, bool nDefault = false);
	static COLORREF HexStringToColorRefRGB(const char* hexStr);
	static ppmfc::CString ColorRefRGBToHexString(COLORREF color);

    static ppmfc::CString ReplaceSpeicalString(ppmfc::CString ori);

    static ppmfc::CString RandomSelect(std::vector<ppmfc::CString>& vec);
    static FString RandomSelect(std::vector<FString>& vec);
    static int RandomSelectInt(std::vector<int>& vec, bool record = false, int thisCT = -1);
    static int RandomSelectInt(std::set<int>& vec);
    static int RandomSelectInt(int start, int end);
	template <typename T, std::size_t N>
	static T const& RandomSelectArray(const std::array<T, N>& arr) {
		static thread_local std::mt19937 gen(std::random_device{}());
		std::uniform_int_distribution<std::size_t> dist(0, N - 1);
		return arr[dist(gen)];
	}

	static ppmfc::CString GetRandomFacing();

    static std::string ChineseTraditional_ToSimple(const std::string& _str);
    static std::string ToUpperCase(const std::string& _str);

    static std::string WStringToString(const std::wstring& wstr);
    static std::wstring StringToWString(const std::string& str);
	static void WStringReplace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr);

	static bool isUTF8(const uint8_t* data, size_t size);

	static inline int letter2number(char let) {
		int reply = let - 'A';
		return reply;

	}

	static inline char number2letter(int let) {
		int reply = let + 'A';
		return reply;

	}

	static inline int StringToWaypoint(ppmfc::CString str)
	{
		if (str == "None")
			return -1;
		int n = 0;
		int len = strlen(str);
		for (int i = len - 1, j = 1; i >= 0; i--, j *= 26)
		{
			int c = toupper(str[i]);
			if (c < 'A' || c > 'Z') return 0;
			n += ((int)c - 64) * j;
		}
		if (n <= 0)
			return -1;
		return n - 1;
	}
	static inline ppmfc::CString StringToWaypointStr(ppmfc::CString str)
	{
		ppmfc::CString ret;
		ret.Format("%d", StringToWaypoint(str));
		if (ret == "-1") ret = "None";
		return ret;
	}

	// Serialize waypoint, will be renamed later
	static inline ppmfc::CString WaypointToString(int nWaypoint)
	{
		static char buffer[8]{ '\0' };

		if (nWaypoint < 0)
			return "0";
		else if (nWaypoint == INT_MAX)
			return "FXSHRXX";
		else
		{
			++nWaypoint;
			int pos = 7;
			while (nWaypoint > 0)
			{
				--pos;
				char m = nWaypoint % 26;
				if (m == 0) m = 26;
				buffer[pos] = m + '@'; // '@' = 'A' - 1
				nWaypoint = (nWaypoint - m) / 26;
			}
			return buffer + pos;
		}
	}

	static inline ppmfc::CString WaypointToString(ppmfc::CString numStr)
	{
		int nWaypoint = atoi(numStr);
		return WaypointToString(nWaypoint);
	}

	static inline int StringToWaypoint(std::string str)
	{
		if (str == "None")
			return -1;
		int n = 0;
		int len = strlen(str.c_str());
		for (int i = len - 1, j = 1; i >= 0; i--, j *= 26)
		{
			int c = toupper(str[i]);
			if (c < 'A' || c > 'Z') return 0;
			n += ((int)c - 64) * j;
		}
		if (n <= 0)
			return -1;
		return n - 1;
	}
	static inline FString StringToWaypointStr(std::string str)
	{
		FString ret;
		ret.Format("%d", StringToWaypoint(str));
		if (ret == "-1") ret = "None";
		return ret;
	}

	static inline FString WaypointToString(std::string numStr)
	{
		int nWaypoint = atoi(numStr.c_str());
		static char buffer[8]{ '\0' };

		if (nWaypoint < 0)
			return "0";
		else if (nWaypoint == INT_MAX)
			return "FXSHRXX";
		else
		{
			++nWaypoint;
			int pos = 7;
			while (nWaypoint > 0)
			{
				--pos;
				char m = nWaypoint % 26;
				if (m == 0) m = 26;
				buffer[pos] = m + '@'; // '@' = 'A' - 1
				nWaypoint = (nWaypoint - m) / 26;
			}
			return buffer + pos;
		}
		return "0";
	}
};