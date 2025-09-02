#pragma once

#include <MFC/ppmfc_cstring.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <wtypes.h>
#include <type_traits>
#include <utility>
#include <variant>

class FString : public std::string {
public:

    using std::string::string;
    FString() = default;
    FString(const FString&) = default;
    FString(FString&&) = default;
    FString& operator=(const FString&) = default;
    FString& operator=(FString&&) = default;
    FString(const char* str) : std::string(str ? str : "") {}
    FString(const std::string& str) : std::string(str) {}
    FString(size_t n, char c) : std::string(n, c) {}
    FString(const ppmfc::CString& cstr) : std::string(cstr.m_pchData) {}
    operator ppmfc::CString() const { return ppmfc::CString(c_str()); }

    operator LPCSTR() const { return c_str(); }
    operator LPARAM() const { return reinterpret_cast<LPARAM>(c_str()); }
    LPCTSTR operator()() const { return c_str(); }
    LPSTR GetBuffer(size_t nMinBufLength = 0) {
        if (nMinBufLength > length()) resize(nMinBufLength);
        return data();
    }

    void ReleaseBuffer(int nNewLength = -1) {
        if (nNewLength == -1) {
            nNewLength = static_cast<int>(length());
        }
        if (nNewLength < 0) nNewLength = 0;
        resize(static_cast<size_t>(nNewLength));
    }

    FString& operator+=(const FString& other) { append(other); return *this; }
    FString& operator+=(const ppmfc::CString& other) { append(other.m_pchData); return *this; }
    FString& operator+=(LPCSTR other) { if(other) append(other); return *this; }
    FString& operator+=(TCHAR ch) { push_back(ch); return *this; }

    friend FString operator+(const FString& lhs, const FString& rhs) { FString r = lhs; r += rhs; return r; }
    friend FString operator+(const FString& lhs, const ppmfc::CString& rhs) { FString r = lhs; r += rhs.m_pchData; return r; }
    friend FString operator+(const FString& lhs, LPCSTR rhs) { FString r = lhs; r += rhs; return r; }
    friend FString operator+(const FString& lhs, TCHAR rhs) { FString r = lhs; r += rhs; return r; }
    friend FString operator+(LPCSTR lhs, const FString& rhs) { FString r; if(lhs) r = lhs; r += rhs; return r; }

    int GetLength() const { return static_cast<int>(length()); }
    bool IsEmpty() const { return empty(); }
    FString& MakeReverse() {
        std::reverse(begin(), end());
        return *this;
    }

    TCHAR GetAt(int nIndex) const {
        if (nIndex < 0 || static_cast<size_t>(nIndex) >= length()) {
            throw std::out_of_range("Index out of range");
        }
        return at(nIndex);
    }

    void SetAt(int nIndex, TCHAR ch) {
        if (nIndex < 0 || static_cast<size_t>(nIndex) >= length()) {
            throw std::out_of_range("Index out of range");
        }
        at(nIndex) = ch;
    }

    TCHAR operator[](int nIndex) const { return GetAt(nIndex); }
    TCHAR& operator[](int nIndex) { return at(nIndex); }

    int Compare(LPCTSTR lpsz) const { return compare(lpsz); }
    int CompareNoCase(LPCTSTR lpsz) const {
        std::string s1 = *this;
        std::string s2 = lpsz;
        std::transform(s1.begin(), s1.end(), s1.begin(),
            [](unsigned char c) { return std::tolower(c); });
        std::transform(s2.begin(), s2.end(), s2.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s1.compare(s2);
    }

    bool operator==(const FString& other) const { return Compare(other) == 0; }
    bool operator==(const ppmfc::CString& other) const { return Compare(other.m_pchData) == 0; }
    bool operator==(LPCSTR other) const { return Compare(other) == 0; }
    bool operator==(const std::string& other) const { return Compare(other.c_str()) == 0; }
    bool operator!=(const FString& other) const { return !(*this == other); }
    bool operator!=(const ppmfc::CString& other) const { return !(*this == other); }
    bool operator!=(LPCSTR other) const { return !(*this == other); }
    bool operator!=(const std::string& other) const { return !(*this == other); }

    FString& Delete(size_t nIndex, size_t nCount = 1) {
        if (nIndex >= length() || nCount == 0) return *this;
        if (nCount > length() - nIndex) nCount = length() - nIndex;
        erase(nIndex, nCount);
        return *this;
    }

    FString& Insert(size_t nIndex, TCHAR ch) {
        if (nIndex > length()) throw std::out_of_range("Index out of range");
        insert(nIndex, 1, ch);
        return *this;
    }

    FString& Replace(TCHAR chOld, TCHAR chNew) {
        std::replace(begin(), end(), chOld, chNew);
        return *this;
    }

    FString& Replace(const FString& oldStr, const FString& newStr) {
        if (oldStr.empty()) return *this;
        size_t pos = 0;
        while ((pos = find(oldStr, pos)) != npos) {
            std::string::replace(pos, oldStr.length(), newStr);
            pos += newStr.length();
        }
        return *this;
    }

    FString Mid(size_t nFirst, size_t nCount = npos) const {
        if (nFirst >= length()) return FString();
        if (nCount == npos || nFirst + nCount > length()) nCount = length() - nFirst;
        return FString(substr(nFirst, nCount));
    }

    FString Left(size_t nCount) const {
        if (nCount == 0) return FString();
        if (nCount > length()) nCount = length();
        return FString(substr(0, nCount));
    }

    FString Right(size_t nCount) const {
        if (nCount == 0) return FString();
        if (nCount > length()) nCount = length();
        return FString(substr(length() - nCount, nCount));
    }

    int Find(const FString& str, size_t nStart = 0) const {
        if (str.empty() || nStart >= length()) return -1;
        size_t pos = std::string::find(str, nStart);
        return pos == npos ? -1 : static_cast<int>(pos);
    }

    int Find(const char* s, size_t nStart = 0) const {
        if (!s || nStart >= length()) return -1;
        size_t pos = std::string::find(s, nStart);
        return pos == npos ? -1 : static_cast<int>(pos);
    }

    int Find(char ch, size_t nStart = 0) const {
        if (nStart >= length()) return -1;
        size_t pos = std::string::find(ch, nStart);
        return pos == npos ? -1 : static_cast<int>(pos);
    }

    FString& MakeUpper() { std::transform(begin(), end(), begin(), [](unsigned char c) { return std::toupper(c); }); return *this; }
    FString& MakeLower() { std::transform(begin(), end(), begin(), [](unsigned char c) { return std::tolower(c); }); return *this; }

    FString& TrimLeft() {
        erase(begin(), std::find_if(begin(), end(), [](unsigned char c) { return !std::isspace(c); }));
        return *this;
    }
    FString& TrimRight() {
        erase(std::find_if(rbegin(), rend(), [](unsigned char c) { return !std::isspace(c); }).base(), end());
        return *this;
    }
    FString& Trim() { return TrimLeft().TrimRight(); }

    template<typename... Args>
    void Format(const char* lpszFormat, Args&&... args) {
        if (!lpszFormat || !strlen(lpszFormat)) {
            throw std::invalid_argument("Invalid format string");
        }

        std::vector<std::variant<int, double, const char*, const wchar_t*>> convertedArgs;
        (convertArg(convertedArgs, std::forward<Args>(args)), ...);

        FormatVImpl(lpszFormat, convertedArgs);
    }

private:
    template<typename T>
    void convertArg(std::vector<std::variant<int, double, const char*, const wchar_t*>>& vec, T&& arg) {
        using U = std::decay_t<T>;
        if constexpr (std::is_same_v<U, FString>) {
            vec.push_back(arg.c_str());
        }
        else if constexpr (std::is_same_v<U, ppmfc::CString>) {
            vec.push_back(arg.m_pchData);
        }
        else if constexpr (std::is_same_v<U, std::string>) {
            vec.push_back(arg.c_str());
        }
        else if constexpr (std::is_same_v<U, std::wstring>) {
            vec.push_back(arg.c_str());
        }
        else if constexpr (std::is_same_v<U, const char*>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, char*>) {
            vec.push_back(static_cast<const char*>(arg));
        }
        else if constexpr (std::is_same_v<U, const wchar_t*>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, int>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, size_t>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, short>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, word>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, dword>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, char>) {
            vec.push_back(static_cast<int>(arg)); 
        }
        else if constexpr (std::is_same_v<U, byte>) {
            vec.push_back(static_cast<int>(arg)); 
        }
        else if constexpr (std::is_same_v<U, bool>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, double>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, float>) {
            vec.push_back(static_cast<double>(arg));
        }
        else if constexpr (std::is_same_v<U, void*>) {
            vec.push_back(reinterpret_cast<intptr_t>(arg));
        }
        else if constexpr (std::is_same_v<U, int*>) {
            vec.push_back(reinterpret_cast<intptr_t>(arg));
        }
        else {
            static_assert(false, "Unsupported argument type for Format");
        }
    }

    void FormatVImpl(const char* format, const std::vector<std::variant<int, double, const char*, const wchar_t*>>& args) {
        constexpr int FORCE_ANSI = 0x10000;
        constexpr int FORCE_UNICODE = 0x20000;

        size_t argIndex = 0;
        std::string result;
        result.reserve(strlen(format) + 32);

        for (const char* p = format; *p; ++p) {
            if (*p != '%') {
                result += *p;
                continue;
            }

            ++p; 
            if (*p == '%') {
                result += '%';
                continue;
            }

            std::string formatSpec = "%";
            int nWidth = 0;
            int nPrecision = 0;
            int nModifier = 0;

            while (*p == '#' || *p == '*' || *p == '-' || *p == '+' || *p == '0' || *p == ' ') {
                formatSpec += *p;
                if (*p == '*') {
                    if (argIndex >= args.size()) throw std::runtime_error("Too few arguments for width");
                    nWidth = std::get<int>(args[argIndex++]);
                }
                ++p;
            }

            if (isdigit(*p)) {
                formatSpec += std::string(p, strspn(p, "0123456789"));
                nWidth = atoi(p);
                while (isdigit(*p)) ++p;
            }

            if (*p == '.') {
                formatSpec += '.';
                ++p;
                if (*p == '*') {
                    formatSpec += '*';
                    if (argIndex >= args.size()) throw std::runtime_error("Too few arguments for precision");
                    nPrecision = std::get<int>(args[argIndex++]);
                    ++p;
                }
                else if (isdigit(*p)) {
                    formatSpec += std::string(p, strspn(p, "0123456789"));
                    nPrecision = atoi(p);
                    while (isdigit(*p)) ++p;
                }
            }

            switch (*p) {
            case 'h':
                nModifier = FORCE_ANSI;
                formatSpec += 'h';
                ++p;
                break;
            case 'l':
                nModifier = FORCE_UNICODE;
                formatSpec += 'l';
                ++p;
                break;
            case 'F':
            case 'N':
            case 'L':
                formatSpec += *p;
                ++p;
                break;
            }

            if (argIndex >= args.size()) throw std::runtime_error("Too few arguments for format specifier");
            formatSpec += *p;

            switch (*p | nModifier) {
            case 'c':
            case 'C':
            case 'c' | FORCE_ANSI:
            case 'C' | FORCE_ANSI:
            {
                int value = std::get<int>(args[argIndex++]);
                char buf[16];
                snprintf(buf, sizeof(buf), formatSpec.c_str(), value);
                result += buf;
                break;
            }
            case 'c' | FORCE_UNICODE:
            case 'C' | FORCE_UNICODE:
            {
                wchar_t value = static_cast<wchar_t>(std::get<int>(args[argIndex++]));
                wint_t wvalue = static_cast<wint_t>(value);
                char buf[16];
                snprintf(buf, sizeof(buf), "%lc", wvalue);
                result += buf;
                break;
            }
            case 's':
            case 's' | FORCE_ANSI:
            case 'S':
            case 'S' | FORCE_ANSI:
            {
                const char* str = std::get<const char*>(args[argIndex++]);
                char buf[1024];
                snprintf(buf, sizeof(buf), formatSpec.c_str(), str ? str : "(null)");
                result += buf;
                break;
            }
            case 's' | FORCE_UNICODE:
            case 'S' | FORCE_UNICODE:
            {
                const wchar_t* str = std::get<const wchar_t*>(args[argIndex++]);
                if (str) {
                    int ansiSize = WideCharToMultiByte(CP_ACP, 0, str, -1, nullptr, 0, nullptr, nullptr);
                    if (ansiSize == 0) {
                        result += "(null)";
                        break;
                    }
                    std::vector<char> ansiStr(ansiSize);
                    WideCharToMultiByte(CP_ACP, 0, str, -1, ansiStr.data(), ansiSize, nullptr, nullptr);
                    char buf[1024];
                    snprintf(buf, sizeof(buf), "%s", ansiStr.data());
                    result += buf;
                }
                else {
                    char buf[1024];
                    snprintf(buf, sizeof(buf), "%s", "(null)");
                    result += buf;
                }
                break;
            }
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            case 'o':
            case 'p':
            case 'n':
            {
                int value = std::get<int>(args[argIndex++]);
                char buf[64];
                snprintf(buf, sizeof(buf), formatSpec.c_str(), value);
                result += buf;
                break;
            }
            case 'e':
            case 'f':
            case 'g':
            case 'G':
            {
                double value = std::get<double>(args[argIndex++]);
                char buf[128];
                snprintf(buf, sizeof(buf), formatSpec.c_str(), value);
                result += buf;
                break;
            }
            default:
                throw std::invalid_argument("Unsupported format specifier: " + formatSpec);
            }
            }

        if (argIndex < args.size()) {
            throw std::runtime_error("Too many arguments provided");
        }

        *this = result.c_str();
    }
public:

    void ReplaceNumString(int value, const char* to)
    {
        char buffer[64], num[64];
        _itoa_s(value, num, 10);
        num[63] = '\0';
        buffer[0] = '%';
        buffer[1] = '\0';
        strcat_s(buffer, num);
        buffer[63] = '\0';
        if (Find(buffer) >= 0)
            Replace(buffer, to);
    }

    void toANSI() {
        int wideSize = MultiByteToWideChar(CP_UTF8, 0, c_str(), -1, nullptr, 0);
        if (wideSize == 0) {
            clear();
            return;
        }

        std::vector<wchar_t> wideStr(wideSize);
        MultiByteToWideChar(CP_UTF8, 0, c_str(), -1, wideStr.data(), wideSize);

        int ansiSize = WideCharToMultiByte(CP_ACP, 0, wideStr.data(), -1, nullptr, 0, nullptr, nullptr);
        if (ansiSize == 0) {
            clear();
            return;
        }

        std::vector<char> ansiStr(ansiSize);
        WideCharToMultiByte(CP_ACP, 0, wideStr.data(), -1, ansiStr.data(), ansiSize, nullptr, nullptr);

        assign(ansiStr.data());
    }

    void toUTF8() {
        int wideSize = MultiByteToWideChar(CP_ACP, 0, c_str(), -1, nullptr, 0);
        if (wideSize == 0) {
            return;
        }

        std::wstring wide(wideSize, 0);
        MultiByteToWideChar(CP_ACP, 0, c_str(), -1, &wide[0], wideSize);

        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Size == 0) {
            return; 
        }

        std::vector<char> utf8(utf8Size);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), utf8Size, nullptr, nullptr);

        assign(utf8.data());
    }

    FString GetParam(int index)
    {
        std::vector<FString> tokens = SplitString(*this, ",");
        if (index < 0 || index >= (int)tokens.size())
            return {};
        return tokens[index];
    }

    void SetParam(int index, const FString& value)
    {
        std::vector<FString> tokens = SplitString(*this, ",");
        if (index < 0)
            return;

        if (index >= (int)tokens.size())
        {
            tokens.resize(index + 1, _T(""));
        }

        tokens[index] = value;
        *this = Join(tokens);
    }

    static std::vector<FString> SplitString(const FString& pSource, const char* pSplit = ",") {
        std::vector<FString> ret;
        if (pSplit == nullptr || pSource.GetLength() == 0) {
            return ret;
        }

        int nIdx = 0;
        while (true) {
            int nPos = pSource.Find(FString(pSplit), nIdx);
            if (nPos == -1) break;

            if (nPos >= nIdx) {
                ret.push_back(pSource.Mid(nIdx, nPos - nIdx));
            }
            nIdx = nPos + strlen(pSplit);
        }
        ret.push_back(pSource.Mid(nIdx));
        return ret;
    }

    static std::vector<FString> SplitString(const FString& pSource, size_t nth, const char* pSplit = ",") {
        std::vector<FString> ret = SplitString(pSource, pSplit);
        while (ret.size() <= nth) {
            ret.push_back("");
        }
        return ret;
    }

    static std::vector<FString> SplitStringMultiSplit(const FString& pSource, const char* pSplit) {
        auto splits = SplitString(pSplit, "|");
        std::vector<FString> ret;
        if (pSource.GetLength() == 0) {
            return ret;
        }

        int nIdx = 0;
        while (true) {
            int nPos = INT_MAX;
            bool found = false;
            for (auto& p : splits) {
                int thisPos = pSource.Find(p, nIdx);
                if (thisPos == -1) continue;
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

    static std::pair<FString, FString> SplitKeyValue(const FString& pSource) {
        const char* pSplit = "=";
        std::pair<FString, FString> ret;
        if (pSource.GetLength() == 0) {
            return ret;
        }

        int nIdx = 0;
        int nPos = pSource.Find(pSplit, nIdx);
        if (nPos == -1) {
            return ret;
        }

        ret.first = pSource.Mid(nIdx, nPos - nIdx);
        nIdx = nPos + 1;
        ret.second = pSource.Mid(nIdx);
        return ret;
    }

    static std::vector<FString> SplitStringAction(const FString& pSource, size_t nth, const char* pSplit = ",") {
        std::vector<FString> ret = SplitString(pSource, pSplit);
        while (ret.size() <= nth) {
            ret.push_back("0");
        }
        return ret;
    }

    static std::vector<FString> SplitStringTrimmed(const FString& pSource, const char* pSplit = ",") {
        std::vector<FString> ret;
        if (pSplit == nullptr || pSource.GetLength() == 0) {
            return ret;
        }

        int nIdx = 0;
        while (true) {
            int nPos = pSource.Find(FString(pSplit), nIdx);
            if (nPos == -1) break;

            if (nPos >= nIdx) {
                ret.push_back(pSource.Mid(nIdx, nPos - nIdx).Trim());
            }
            nIdx = nPos + strlen(pSplit);
        }
        ret.push_back(pSource.Mid(nIdx).Trim());
        return ret;
    }

    static void TrimIndex(FString& str) {
        str.Trim();
        int spaceIndex = str.Find(' ');
        if (spaceIndex > 0) {
            str = str.Mid(0, spaceIndex);
        }
    }

    static void TrimSemicolon(FString& str) {
        str.Trim();
        int semicolon = str.Find(';');
        if (semicolon > 0) {
            str = str.Mid(0, semicolon);
        }
    }

    static void TrimIndexElse(FString& str) {
        str.Trim();
        int spaceIndex = str.Find(' ');
        if (spaceIndex > 0) {
            str = str.Mid(spaceIndex + 1);
        }
    }

    static FString ReplaceSpeicalString(const FString& ori)
    {
        FString ret = ori;
        ret.Replace("%1", ",");
        ret.Replace("\\t", "\t");
        ret.Replace("\\n", "\r\n");
        return ret;
    }

    static FString Join(const std::vector<FString>& tokens, const char* delim = ",")
    {
        FString result;
        for (size_t i = 0; i < tokens.size(); ++i)
        {
            if (i > 0)
                result += delim;
            result += tokens[i];
        }
        return result;
    }
};

namespace std {
    template<>
    struct hash<FString> {
        size_t operator()(const FString& s) const {
            return std::hash<std::string>{}(s); 
        }
    };
}