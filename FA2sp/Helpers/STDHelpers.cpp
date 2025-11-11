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

ppmfc::CString STDHelpers::ReplaceSpeicalString(ppmfc::CString ori)
{
    ppmfc::CString ret = ori;
    ret.Replace("%1", ",");
    ret.Replace("\\t", "\t");
    ret.Replace("\\n", "\r\n");
    return ret;
}

std::string STDHelpers::ChineseTraditional_ToSimple(const std::string& _str)
{
    LPCSTR lpSrcStr = _str.c_str();
    int cchSrc = static_cast<int>(_str.size());
    int cchDest = static_cast<int>(1 + _str.size());
    LPSTR lpDestStr = new CHAR[cchDest]{ 0 };
    LCMapStringA(0x0804, LCMAP_SIMPLIFIED_CHINESE, lpSrcStr, cchSrc, lpDestStr, cchDest);
    std::string str(lpDestStr);
    delete[] lpDestStr;
    lpDestStr = nullptr;
    return str;
}

std::string STDHelpers::ToUpperCase(const std::string& _str) {
    std::string upperStr;
    for (char c : _str) {
        upperStr += std::toupper(static_cast<unsigned char>(c));
    }
    return upperStr;
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

std::wstring STDHelpers::StringToWString(const std::string& str) 
{
    int wideSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (wideSize == 0) return L"";

    std::wstring wstr(wideSize, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], wideSize);

    return wstr;
}

bool STDHelpers::isUTF8(const uint8_t* data, size_t size) {
    if (!data || size == 0)
        return false;

    // --- UTF-8 BOM ---
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        return true;

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
            if (i + 1 >= size) return false;
            if ((data[i + 1] & 0xC0) != 0x80) return false;
            if (c == 0xC0 || c == 0xC1) return false; // overlong
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= size) return false;
            if ((data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) return false;
            if (c == 0xE0 && (data[i + 1] & 0xE0) == 0x80) return false; // overlong
            if (c == 0xED && (data[i + 1] & 0xE0) == 0xA0) return false; // surrogate
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= size) return false;
            if ((data[i + 1] & 0xC0) != 0x80 ||
                (data[i + 2] & 0xC0) != 0x80 ||
                (data[i + 3] & 0xC0) != 0x80)
                return false;
            if (c == 0xF0 && (data[i + 1] & 0xF0) == 0x80) return false;
            if (c > 0xF4 || (c == 0xF4 && data[i + 1] > 0x8F)) return false;
            i += 4;
        }
        else {
            return false;
        }
    }

    // label ASCII as ANSI
    if (allAscii)
        return false; 

    return true;
}
