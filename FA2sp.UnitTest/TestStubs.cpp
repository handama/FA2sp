#include <afxwin.h>
#include <FA2PP.h>
#include <Helpers/FString.h>
#include <Helpers/STDHelpers.h>
#include <Ext/CMapData/Body.h>
#include <Ext/CLoading/Body.h>
#include <Logger.h>
#include <FA2sp.h>

// Logger stubs
FILE* Logger::pFile = nullptr;
bool Logger::bInitialized = false;

// ExtConfigs stubs
bool ExtConfigs::AllowIncludes = false;
bool ExtConfigs::AllowInherits = false;
bool ExtConfigs::AllowPlusEqual = false;
bool ExtConfigs::SaveMap_KeepComments = false;
bool ExtConfigs::IncludeType = false;
bool ExtConfigs::InheritType = false;
bool ExtConfigs::SaveMap_PreserveINISorting = true;
bool ExtConfigs::SaveMap_AdaptiveSorting = true;
bool ExtConfigs::UTF8Support_InferEncoding = true;

// CMapDataExt stubs
bool CMapDataExt::IsLoadingMapFile = false;
bool CMapDataExt::IsUTF8File = false;
std::unordered_map<int, FString> CMapDataExt::TileSetOriginSetNames[6];
FMap<FMap<FString>> CMapDataExt::MapInlineComments;
FMap<FMap<FString>> CMapDataExt::MapFrontlineComments;
FMap<FString> CMapDataExt::MapInsectionComments;
FMap<FString> CMapDataExt::MapFrontsectionComments;

ppmfc::CString CMapDataExt::GetAvailableIndex(EIndexType type)
{
    return "";
}

// CLoadingExt stubs
void* CLoadingExt::ReadWholeFile(char const* filename, unsigned long* pDwSize, bool fa2path, bool useCache)
{
    return nullptr;
}

// STDHelpers stubs needed by Hooks.INI.cpp
FileEncoding STDHelpers::GetFileEncoding(const uint8_t* data, size_t size)
{
    if (!data || size == 0)
        return FileEncoding::ANSI;
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        return FileEncoding::UTF8_BOM;
    return FileEncoding::ANSI;
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

std::vector<ppmfc::CString> STDHelpers::SplitString(const ppmfc::CString& pSource, size_t nth, const char* pSplit)
{
    std::vector<ppmfc::CString> ret = SplitString(pSource, pSplit);
    while (ret.size() <= nth)
    {
        ret.push_back("");
    }
    return ret;
}

bool STDHelpers::IsNoneOrEmpty(const char* pSource)
{
    return !pSource || *pSource == '\0' || _stricmp(pSource, "<none>") == 0;
}

