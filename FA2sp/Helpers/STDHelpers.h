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

enum FileEncoding : char
{
	Unknown = -1,
	ANSI = 0,
	UTF8 = 1,
	UTF8_BOM = 2,
	UTF8_ASCII = 3
};

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
    static bool IsNumber(const char* str);
    static bool IsTrue(const char* str, bool nDefault = false);
    static COLORREF HexStringToColorRefRGB(const char* hexStr);
    static ppmfc::CString ColorRefRGBToHexString(COLORREF color);

    static ppmfc::CString ReplaceSpeicalString(FString_view ori);

    static ppmfc::CString RandomSelect(std::vector<ppmfc::CString>& vec);
    static FString RandomSelect(std::vector<FString>& vec);
    static int RandomSelectInt(std::vector<int>& vec, bool record = false, int thisCT = -1);
    static int RandomSelectInt(std::set<int>& vec);
    static int RandomSelectInt(int start, int end);
    static ppmfc::CString GetRandomFacing();

    static FString ChineseTraditional_ToSimple(FString_view str);
    static std::string ToUpperCase(FString_view str);

    static std::string WStringToString(const std::wstring& wstr);
    static std::wstring StringToWString(FString_view str);
    static void WStringReplace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr);
    static std::string ReplaceEnding(FString_view str, FString_view oldSuffix, FString_view newSuffix);

    static FileEncoding GetFileEncoding(const uint8_t* data, size_t size);

    static int letter2number(char let);
    static char number2letter(int let);
    static int StringToWaypoint(ppmfc::CString str);
    static ppmfc::CString StringToWaypointStr(ppmfc::CString str);
    static ppmfc::CString WaypointToString(int nWaypoint);
    static ppmfc::CString WaypointToString(ppmfc::CString numStr);
    static int StringToWaypoint(std::string str);
    static FString StringToWaypointStr(std::string str);
    static FString WaypointToString(std::string numStr);

	template <typename T, std::size_t N>
	static T const& RandomSelectArray(const std::array<T, N>& arr) {
		static thread_local std::mt19937 gen(std::random_device{}());
		std::uniform_int_distribution<std::size_t> dist(0, N - 1);
		return arr[dist(gen)];
	}
};