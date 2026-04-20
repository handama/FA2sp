#include <afxwin.h> 
#include <wtypes.h> 
#include "FString.h"

std::vector<FString> FString::SplitString(FString_view source, const char* pSplit) {
    std::vector<FString> ret;

    if (!pSplit || source.empty())
        return ret;

    FString_view delim(pSplit);
    size_t pos = 0;

    while (true)
    {
        size_t found = source.find(delim, pos);
        if (found == FString_view::npos)
            break;

        ret.emplace_back(source.substr(pos, found - pos));
        pos = found + delim.size();
    }

    ret.emplace_back(source.substr(pos));

    return ret;
}

std::vector<FString> FString::SplitString(FString_view source, size_t nth, const char* pSplit) {
    std::vector<FString> ret = SplitString(source, pSplit);
    while (ret.size() <= nth) {
        ret.push_back("");
    }
    return ret;
}

std::vector<FString> FString::SplitStringMultiSplit(FString_view source, const char* split) {
    std::vector<FString> ret;
    if (source.empty() || !split)
        return ret;

    std::vector<std::string_view> splits;
    {
        std::string_view s(split);
        size_t pos = 0;
        while (true)
        {
            size_t found = s.find('|', pos);
            if (found == std::string_view::npos)
            {
                splits.emplace_back(s.substr(pos));
                break;
            }
            splits.emplace_back(s.substr(pos, found - pos));
            pos = found + 1;
        }
    }

    size_t idx = 0;

    while (true)
    {
        size_t bestPos = std::string_view::npos;
        size_t bestLen = 0;

        for (auto& delim : splits)
        {
            size_t p = source.find(delim, idx);
            if (p != std::string_view::npos && (bestPos == std::string_view::npos || p < bestPos))
            {
                bestPos = p;
                bestLen = delim.size();
            }
        }

        if (bestPos == std::string_view::npos)
            break;

        ret.emplace_back(source.substr(idx, bestPos - idx));
        idx = bestPos + bestLen;
    }

    ret.emplace_back(source.substr(idx));
    return ret;
}

std::pair<FString, FString> FString::SplitKeyValue(FString_view source) {
    std::pair<FString, FString> ret;

    if (source.empty())
        return ret;

    size_t pos = source.find('=');
    if (pos == std::string_view::npos)
        return ret;

    ret.first = FString(source.substr(0, pos));
    ret.second = FString(source.substr(pos + 1));

    return ret;
}

std::vector<FString> FString::SplitStringAction(FString_view source, size_t nth, const char* pSplit) {
    std::vector<FString> ret = SplitString(source, pSplit);

    while (ret.size() <= nth)
    {
        ret.emplace_back("0");
    }

    return ret;
}

std::vector<FString> FString::SplitStringTrimmed(FString_view source, const char* pSplit) {
    std::vector<FString> ret;

    if (!pSplit || source.empty())
        return ret;

    std::string_view delim(pSplit);
    size_t pos = 0;

    while (true)
    {
        size_t found = source.find(delim, pos);
        if (found == std::string_view::npos)
            break;

        FString tmp(source.substr(pos, found - pos));
        tmp.Trim();
        ret.emplace_back(std::move(tmp));

        pos = found + delim.size();
    }

    FString tmp(source.substr(pos));
    tmp.Trim();
    ret.emplace_back(std::move(tmp));

    return ret;
}

void FString::TrimIndex(FString& str) {
    str.Trim();
    int spaceIndex = str.Find(' ');
    if (spaceIndex >= 0) {
        str = str.Mid(0, spaceIndex);
    }
}

void FString::TrimSemicolon(FString& str) {
    str.Trim();
    int semicolon = str.Find(';');
    if (semicolon >= 0) {
        str = str.Mid(0, semicolon);
    }
}

FString FString::GetComment(FString_view line)
{
    size_t pos = line.find(';');
    if (pos == std::string_view::npos)
        return {};

    FString comment(line.substr(pos + 1));
    comment.Trim();
    return comment;
}

void FString::TrimSemicolonElse(FString& str) {
    str.Trim();
    int semicolon = str.Find(';');
    if (semicolon >= 0) {
        str = str.Mid(semicolon + 1);
    }
}

void FString::TrimIndexElse(FString& str) {
    str.Trim();
    int spaceIndex = str.Find(' ');
    if (spaceIndex >= 0) {
        str = str.Mid(spaceIndex + 1);
    }
}

FString FString::ReplaceSpeicalString(FString_view ori)
{
    FString ret(ori);
    ret.Replace("%1", ",");
    ret.Replace("%2", ";");
    ret.Replace("\\t", "\t");
    ret.Replace("\\n", "\r\n");

    return ret;
}

FString FString::Join(const std::vector<FString>& tokens, const char* delim)
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