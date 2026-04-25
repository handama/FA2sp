#include "STDHelpers.h"
#include <iostream>
#include <vector>
#include "../Logger.h"
#include "../Ext/CFinalSunDlg/Body.h"
#include "../FA2sp.h"

ppmfc::CString STDHelpers::GetRandomFacing()
{
    std::vector<ppmfc::CString> facings = { "0", "32" ,"64" ,"96" ,"128" ,"160" ,"192" ,"224" };
    return RandomSelect(facings);
}

ppmfc::CString STDHelpers::RandomSelect(std::vector<ppmfc::CString>& vec) {
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<int> dis(0, vec.size() - 1); 

    int index = dis(gen); 
    return vec[index];
}

FString STDHelpers::RandomSelect(std::vector<FString>& vec) {
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<int> dis(0, vec.size() - 1); 

    int index = dis(gen); 
    return vec[index];
}

int STDHelpers::RandomSelectInt(std::vector<int>& vec, bool record, int thisCT) {
    if (vec.empty())
        return 0;
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<int> dis(0, vec.size() - 1); 

    int index = dis(gen); 

    if (record)
    {
        if (thisCT == CViewObjectsExt::LastPlacedCT.Index)
        {
            if (vec.size() > 1)
            {
                while (vec[index] == CViewObjectsExt::LastCTTile)
                {
                    index = dis(gen);
                }
            }
        } 
    }

    return vec[index];
}

int STDHelpers::RandomSelectInt(std::set<int>& s) {
    if (s.empty()) {
        return 0;
    }

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, s.size() - 1);

    size_t index = dist(gen);
    auto it = s.begin();
    std::advance(it, index);
    return *it;
}

int STDHelpers::RandomSelectInt(int start, int end)
{
    std::vector<int> vec;
    for (int i = start; i < end; i++)
        vec.push_back(i);
    return RandomSelectInt(vec);
}

bool STDHelpers::IsNumber(const std::string& str) 
{
    if (str.empty()) return false;
    size_t start = 0;
    if (str[0] == '-') {
        if (str.size() == 1) return false;
        start = 1;
    }
    for (size_t i = start; i < str.size(); ++i) 
    {
        if (!std::isdigit(str[i])) return false;
    }
    return true;
}

bool STDHelpers::IsNumber(const char * str) 
{
    std::string tmp = str;
    return STDHelpers::IsNumber(tmp);
}

bool STDHelpers::IsTrue(const char* str, bool nDefault)
{
    switch (toupper(static_cast<unsigned char>(str[0])))
    {
    case '1':
    case 'T':
    case 'Y':
        return true;
    case '0':
    case 'F':
    case 'N':
        return false;
    default:
        return nDefault;
    }
}

COLORREF STDHelpers::HexStringToColorRefRGB(const char* hexStr) 
{
    std::string hex = hexStr;
    if (hex.find("0x") == 0 || hex.find("0X") == 0) {
        hex = hex.substr(2);
    }
    else if (hex.find("#") == 0) {
        hex = hex.substr(1);
    }
    if (hex.length() != 6) return 0;
    VEHGuard guard(false);
    try {
        unsigned int color = std::stoul(hex, nullptr, 16);
        return RGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    }
    catch (...) {
        return 0; 
    }
}

ppmfc::CString STDHelpers::ColorRefRGBToHexString(COLORREF color)
{
    char hexStr[10];
    std::sprintf(hexStr, "0x%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
    return hexStr;
}

std::vector<ppmfc::CString> STDHelpers::SplitString(const ppmfc::CString& pSource, const char* pSplit)
{
    std::vector<ppmfc::CString> ret;
    if (pSplit == nullptr || pSource.GetLength() == 0)
        return ret;

    int nIdx = 0;
    while (true)
    {
        int nPos = pSource.Find(pSplit, nIdx);
        if (nPos == -1)
            break;

        if (nPos >= nIdx)
            ret.push_back(pSource.Mid(nIdx, nPos - nIdx));

        nIdx = nPos + strlen(pSplit);
    }
    ret.push_back(pSource.Mid(nIdx));

    return ret;
}

std::vector<ppmfc::CString> STDHelpers::SplitStringMultiSplit(const ppmfc::CString& pSource, const char* pSplit)
{
    auto splits = SplitString(pSplit, "|");
    std::vector<ppmfc::CString> ret;
    if (pSource.GetLength() == 0)
        return ret;

    int nIdx = 0;
    while (true)
    {
        int nPos = INT_MAX;
        bool found = false;
        for (auto& p : splits)
        {
            int thisPos = pSource.Find(p, nIdx);
            if (thisPos == -1)
                continue;
            nPos = std::min(thisPos, nPos);
            found = true;
        }
        if (!found) break;

        ret.push_back(pSource.Mid(nIdx, nPos - nIdx));
        nIdx = nPos + 1;
    }
    ret.push_back(pSource.Mid(nIdx));
    return ret;
}

std::pair<ppmfc::CString, ppmfc::CString> STDHelpers::SplitKeyValue(const ppmfc::CString& pSource)
{
    const char* pSplit = "=";
    std::pair<ppmfc::CString, ppmfc::CString> ret;
    if (pSource.GetLength() == 0)
        return ret;

    int nIdx = 0;

    int nPos = pSource.Find(pSplit, nIdx);
    if (nPos == -1)
        return ret;

    ret.first = pSource.Mid(nIdx, nPos - nIdx);
    nIdx = nPos + 1;

    ret.second = pSource.Mid(nIdx);
    return ret;
}

std::vector<ppmfc::CString> STDHelpers::SplitString(const ppmfc::CString& pSource, size_t nth, const char* pSplit)
{
    std::vector<ppmfc::CString> ret = SplitString(pSource, pSplit);

    while (ret.size() <= nth)
    {
        ret.push_back("");
        //Logger::Raw("[STDHelpers::SplitString] Warn: (%s) cannot meet nth, adding empty strings.\n", pSource);
    }

    return ret;
}

std::vector<ppmfc::CString> STDHelpers::SplitStringAction(const ppmfc::CString& pSource, size_t nth, const char* pSplit)
{
    std::vector<ppmfc::CString> ret = SplitString(pSource, pSplit);

    while (ret.size() <= nth)
    {
        ret.push_back("0");
    }

    return ret;
}

std::vector<ppmfc::CString> STDHelpers::SplitStringTrimmed(const ppmfc::CString& pSource, const char* pSplit)
{
	std::vector<ppmfc::CString> ret;
	if (pSource.GetLength() == 0)
		return ret;

	int nIdx = 0;
	ppmfc::CString temp;
	while (true)
	{
		int nPos = pSource.Find(pSplit, nIdx);
		if (nPos == -1)
			break;

		temp = pSource.Mid(nIdx, nPos - nIdx);
		TrimString(temp);
		ret.push_back(temp);
		nIdx = nPos + 1;
	}
	temp = pSource.Mid(nIdx);
	TrimString(temp);
	ret.push_back(temp);
	return ret;
}

int STDHelpers::ParseToInt(const char* pSource, int nDefault)
{
    int ret;
    if (sscanf_s(pSource, "%d", &ret) != 1)
        return nDefault;
    return ret;
}

ppmfc::CString STDHelpers::IntToString(int source, const char* format)
{
    ppmfc::CString ret;
    ret.Format(format, source);
    return ret;
}

bool STDHelpers::IsNullOrEmpty(const char* pSource)
{
    return pSource == nullptr || strlen(pSource) == 0;
}

bool STDHelpers::IsNullOrWhitespace(const char* pSource)
{
    if (pSource == nullptr)
        return true;

    auto p = pSource;
    auto q = pSource + strlen(pSource);
    while (p != q)
    {
        if (*p != '\0' && *p != ' ')
            return false;
        ++p;
    }

    return true;
}

bool STDHelpers::IsNullOrWhitespaceOrReturn(const char* pSource)
{
    if (pSource == nullptr || *pSource == '\0') 
        return true;

    while (*pSource)
    {
        if (!isspace(static_cast<unsigned char>(*pSource)))
            return false;
        ++pSource;
    }

    return true;
}

bool STDHelpers::IsNoneOrEmpty(const char* pSource)
{
    int len = strlen(pSource);
    if (len == 0)  return true;
    for (int i = 0; i < len; ++i)
        if (pSource[i] != ' ' && pSource[i] != '\0')  return false;
    return
        strcmp(pSource, "none") != 0 &&
        strcmp(pSource, "<none>") != 0;
}

void STDHelpers::TrimString(ppmfc::CString& str)
{
    str.TrimLeft();
    str.TrimRight();
}

ppmfc::CString STDHelpers::GetTrimString(ppmfc::CString str)
{
    str.TrimLeft();
    str.TrimRight();
    return str;
}

void STDHelpers::TrimIndex(ppmfc::CString& str)
{
    TrimString(str);
    int spaceIndex = str.Find(' ');
    if (spaceIndex > 0)
        str = str.Mid(0, spaceIndex);
}

void STDHelpers::TrimSemicolon(ppmfc::CString& str)
{
    TrimString(str);
    int semicolon = str.Find(';');
    if (semicolon > 0)
        str = str.Mid(0, semicolon);
}

void STDHelpers::TrimIndexElse(ppmfc::CString& str)
{
    TrimString(str);
    int spaceIndex = str.Find(' ');
    if (spaceIndex > 0)
        str = str.Mid(spaceIndex + 1);
}

bool STDHelpers::Contains(ppmfc::CString pStr, ppmfc::CString pQuery, bool bIgnoreCase)
{
    if (bIgnoreCase)
    {
        ppmfc::CString s = pStr;
        ppmfc::CString q = pQuery;
        s.MakeLower();
        q.MakeLower();
        return s.Find(q) != -1;
    }
    else
        return pStr.Find(pQuery) != -1;
}

ppmfc::CString STDHelpers::GetComboBoxText(const ppmfc::CComboBox& cbb)
{
    int nCurSel = cbb.GetCurSel();
    ppmfc::CString ret;

    if (nCurSel == CB_ERR)
        cbb.GetWindowText(ret);
    else
        cbb.GetLBText(nCurSel, ret);

    return ret;
}

ppmfc::CString STDHelpers::ReplaceSpeicalString(FString_view ori)
{
    ppmfc::CString ret(ori.c_str());
    ret.Replace("%1", ",");
    ret.Replace("%2", ";");
    ret.Replace("\\t", "\t");
    ret.Replace("\\n", "\r\n");
    return ret;
}

FString STDHelpers::ChineseTraditional_ToSimple(FString_view str)
{
    if (str.empty())
        return {};

    int srcLen = (int)str.size();

    int destLen = LCMapStringA(
        0x0804,
        LCMAP_SIMPLIFIED_CHINESE,
        str.data(),
        srcLen,
        nullptr,
        0
    );

    if (destLen <= 0)
        return FString(str);

    FString buffer(destLen, 0);

    LCMapStringA(
        0x0804,
        LCMAP_SIMPLIFIED_CHINESE,
        str.data(),
        srcLen,
        buffer.data(),
        destLen
    );

    return buffer;
}

std::string STDHelpers::ToUpperCase(FString_view str)
{
    std::string ret;
    ret.resize(str.size());

    for (size_t i = 0; i < str.size(); ++i)
    {
        ret[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(str[i]))
            );
    }

    return ret;
}
std::string STDHelpers::WStringToString(const std::wstring& wstr) {

    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return "";
    }

    std::string result;
    result.resize(len);

    int bytesConverted = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), &result[0], len, nullptr, nullptr);
    if (bytesConverted != len) {
        return "";
    }

    return result;
}

std::wstring STDHelpers::StringToWString(FString_view str)
{
    if (str.empty())
        return L"";

    int wideSize = MultiByteToWideChar(
        CP_ACP,
        0,
        str.data(),
        (int)str.size(),
        nullptr,
        0
    );

    if (wideSize <= 0)
        return L"";

    std::wstring wstr(wideSize, 0);

    MultiByteToWideChar(
        CP_ACP,
        0,
        str.data(),
        (int)str.size(),
        &wstr[0],
        wideSize
    );

    return wstr;
}
void STDHelpers::WStringReplace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr)
{
    if (oldStr.empty()) return;
    size_t pos = 0;
    while ((pos = str.find(oldStr, pos)) != std::wstring::npos) {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

std::string STDHelpers::ReplaceEnding(FString_view str,
    FString_view oldSuffix,
    FString_view newSuffix)
{
    if (str.size() < oldSuffix.size() ||
        !str.ends_with(oldSuffix))
    {
        return std::string(str);
    }

    std::string ret;
    ret.reserve(str.size() - oldSuffix.size() + newSuffix.size());

    ret.append(str.data(), str.size() - oldSuffix.size());
    ret.append(newSuffix.data(), newSuffix.size());

    return ret;
}

FileEncoding STDHelpers::GetFileEncoding(const uint8_t* data, size_t size) {
    if (!data || size == 0)
        return FileEncoding::ANSI;

    // --- UTF-8 BOM ---
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        return FileEncoding::UTF8_BOM;

    bool allAscii = true;
    size_t i = 0;

    while (i < size) {
        uint8_t c = data[i];
        if (c <= 0x7F) {
            // ASCII
            i++;
            continue;
        }

        allAscii = false; 

        if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= size) return FileEncoding::ANSI;
            if ((data[i + 1] & 0xC0) != 0x80) return FileEncoding::ANSI;
            if (c == 0xC0 || c == 0xC1) return FileEncoding::ANSI; // overlong
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= size) return FileEncoding::ANSI;
            if ((data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) return FileEncoding::ANSI;
            if (c == 0xE0 && (data[i + 1] & 0xE0) == 0x80) return FileEncoding::ANSI; // overlong
            if (c == 0xED && (data[i + 1] & 0xE0) == 0xA0) return FileEncoding::ANSI; // surrogate
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= size) return FileEncoding::ANSI;
            if ((data[i + 1] & 0xC0) != 0x80 ||
                (data[i + 2] & 0xC0) != 0x80 ||
                (data[i + 3] & 0xC0) != 0x80)
                return FileEncoding::ANSI;
            if (c == 0xF0 && (data[i + 1] & 0xF0) == 0x80) return FileEncoding::ANSI;
            if (c > 0xF4 || (c == 0xF4 && data[i + 1] > 0x8F)) return FileEncoding::ANSI;
            i += 4;
        }
        else {
            return FileEncoding::ANSI;
        }
    }

    // label ASCII as ANSI
    if (allAscii)
        return FileEncoding::UTF8_ASCII;

    return FileEncoding::UTF8;
}

int STDHelpers::letter2number(char let) {
    return let - 'A';
}

char STDHelpers::number2letter(int let) {
    return let + 'A';
}

int STDHelpers::StringToWaypoint(ppmfc::CString str)
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

ppmfc::CString STDHelpers::StringToWaypointStr(ppmfc::CString str)
{
    ppmfc::CString ret;
    ret.Format("%d", StringToWaypoint(str));
    if (ret == "-1") ret = "None";
    return ret;
}

ppmfc::CString STDHelpers::WaypointToString(int nWaypoint)
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
            buffer[pos] = m + '@';
            nWaypoint = (nWaypoint - m) / 26;
        }
        return buffer + pos;
    }
}

ppmfc::CString STDHelpers::WaypointToString(ppmfc::CString numStr)
{
    int nWaypoint = atoi(numStr);
    return WaypointToString(nWaypoint);
}

int STDHelpers::StringToWaypoint(std::string str)
{
    if (str == "None")
        return -1;
    int n = 0;
    int len = str.size();
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

FString STDHelpers::StringToWaypointStr(std::string str)
{
    FString ret;
    ret.Format("%d", StringToWaypoint(str));
    if (ret == "-1") ret = "None";
    return ret;
}

FString STDHelpers::WaypointToString(std::string numStr)
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
            buffer[pos] = m + '@';
            nWaypoint = (nWaypoint - m) / 26;
        }
        return buffer + pos;
    }
    return "0";
}