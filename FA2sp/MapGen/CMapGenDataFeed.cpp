#include "CMapGenDataFeed.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "../Logger.h"
#include "../Ext/CMapData/Body.h"

namespace
{
    // Routes the generator's INI reads to the INI objects FA2 parsed at start-up.
    // One object serves every file: the generator passes the logical name it
    // asked for ("rulesmd.ini" / "artmd.ini" / "temperatmd.ini").
    class FA2IniSource : public RmIniSource
    {
    public:
        int GetInt(const char* file, const char* section, const char* key,
                   int fallback) const override
        {
            CINI* ini = Pick(file);
            return (ini != nullptr) ? ini->GetInteger(section, key, fallback) : fallback;
        }

        void GetString(const char* file, const char* section, const char* key,
                       char* out, int outSize) const override
        {
            if (out == nullptr || outSize <= 0)
                return;
            out[0] = 0;

            CINI* ini = Pick(file);
            if (ini == nullptr)
                return;

            FString value = ini->GetString(section, key, "");
            strncpy_s(out, static_cast<size_t>(outSize), value.c_str(), _TRUNCATE);
        }

        void GetSectionNames(const char* file,
                             std::vector<std::string>& out) const override
        {
            out.clear();

            CINI* ini = Pick(file);
            if (ini == nullptr)
                return;

            auto it = ini->Dict.begin();
            for (size_t i = 0, n = ini->Dict.size(); i < n; ++i, ++it)
                out.push_back(it->first.GetString());
        }

        int GetKeyList(const char* file, const char* section,
                       char* out, int outSize) const override
        {
            if (out == nullptr || outSize <= 0)
                return 0;
            out[0] = 0;

            CINI* ini = Pick(file);
            if (ini == nullptr)
                return 0;

            INISection* pSection = ini->GetSection(section);
            if (pSection == nullptr)
                return 0;

            // NUL-separated names, closed by an empty string - the layout
            // GetPrivateProfileString produces for a null key.
            int written = 0;
            const unsigned int count = static_cast<unsigned int>(pSection->GetEntities().size());
            for (unsigned int i = 0; i < count; ++i)
            {
                ppmfc::CString* pKey = pSection->GetKeyAt(i);
                if (pKey == nullptr)
                    continue;

                const char* name = pKey->GetString();
                if (name == nullptr || name[0] == 0)
                    continue;

                const int len = static_cast<int>(strlen(name));
                if (written + len + 2 > outSize)
                    break;

                memcpy(out + written, name, static_cast<size_t>(len) + 1);
                written += len + 1;
            }

            if (written + 1 <= outSize)
                out[written] = 0;

            return written;
        }

    private:
        static CINI* Pick(const char* file)
        {
            if (file != nullptr)
            {
                if (_stricmp(file, "temperatmd.ini") == 0
                    || _stricmp(file, "snowmd.ini") == 0
                    || _stricmp(file, "temperat.ini") == 0)
                {
                    // Only the theater of the open map matters here, and that is
                    // exactly what FA2 kept in CurrentTheater.
                    CINI* theater = CINI::CurrentTheater;
                    if (theater != nullptr)
                        return theater;
                    CINI& temperate = CINI::Temperate;
                    return &temperate;
                }
                if (_stricmp(file, "artmd.ini") == 0 || _stricmp(file, "art.ini") == 0)
                {
                    CINI& art = CINI::Art;
                    return &art;
                }
            }
            CINI& rules = CINI::Rules;
            return &rules;
        }
    };
}

void CMapGenDataFeed::InstallIniSource()
{
    // The editor has no rulesmd.ini / temperatmd.ini on disk - FA2 reads
    // them out of the game MIX and keeps the parsed result in CINI::Rules /
    // Art / CurrentTheater. Install an RmIniSource over those so the generator
    // never touches a file.
    static FA2IniSource source;
    static bool installed = false;
    if (!installed)
    {
        RandomMapGenerator::s_iniSource = &source;
        installed = true;
        Logger::Debug("[MapGen] INI source = FA2 CINI (rules sections %u)\n",
                      static_cast<unsigned>(CINI::Rules->Dict.size()));
    }
}

int CMapGenDataFeed::CurrentTheaterIndex()
{
    if (CMapData::Instance->MapNotLoaded)
        return -1;

    FString theater = CMapData::Instance->INI.GetString("Map", "Theater", "TEMPERATE");
    theater.MakeUpper();
    if (theater == "TEMPERATE")
        return 0;
    if (theater == "SNOW")
        return 1;
    return -1;
}

bool CMapGenDataFeed::BuildTileCellAttrs(
    std::vector<std::vector<TileCellAttr> >& out)
{
    out.clear();

    // CMapDataExt::TileData is built by InitializeTileData() from the engine's
    // per-tile-index IsometricTileTypeClass array, so index i is the absolute
    // tile index the generator uses.
    if (CMapDataExt::TileData == nullptr || CMapDataExt::TileDataCount <= 0)
        return false;

    const int count = CMapDataExt::TileDataCount;
    out.resize(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i)
    {
        const CTileTypeClass& type = CMapDataExt::TileData[i];
        std::vector<TileCellAttr>& row = out[static_cast<size_t>(i)];

        const int cells = static_cast<int>(type.TileBlockCount);
        if (cells <= 0 || type.TileBlockDatas == nullptr)
            continue;

        row.resize(static_cast<size_t>(cells));
        for (int k = 0; k < cells; ++k)
        {
            const CTileBlockClass& block = type.TileBlockDatas[k];
            TileCellAttr& attr = row[static_cast<size_t>(k)];

            // One block per TMP sub-cell, so HasValidImage is the per-cell
            // "this frame exists" flag the generator tests in
            // RecalculateCellAttributes (a false there wipes the cell to bare
            // floor).
            attr.hasFrame = (block.HasValidImage != FALSE);
            attr.terrainType = static_cast<unsigned char>(block.TerrainType);
            attr.rampType = block.RampType;

            // The cell grid lives on the tile type, not on each block.
            attr.cellsInX = type.Width;
            attr.cellsInY = type.Height;

            attr.extra4 = block.XMinusExX;
            attr.extra24 = block.YMinusExY;

            // flags36 has no FA2 counterpart. The only reader is
            // TileCellImageInfo, and all it feeds is cell->ZAdjust - a render
            // field this integration never uses.
            attr.flags36 = 0;

            attr.radarRedLeft = block.RadarColorLeft.R;
            attr.radarGreenLeft = block.RadarColorLeft.G;
            attr.radarBlueLeft = block.RadarColorLeft.B;
            attr.radarRedRight = block.RadarColorRight.R;
            attr.radarGreenRight = block.RadarColorRight.G;
            attr.radarBlueRight = block.RadarColorRight.B;
        }
    }

    return true;
}

bool CMapGenDataFeed::Apply(RandomMapGenerator& rmg)
{
    InstallIniSource();

    std::vector<std::vector<TileCellAttr> > attrs;
    if (!BuildTileCellAttrs(attrs))
    {
        Logger::Error("[MapGen] TileData is empty - load a map first.\n");
        return false;
    }

    const size_t withFrame = static_cast<size_t>(std::count_if(
        attrs.begin(), attrs.end(),
        [](const std::vector<TileCellAttr>& v) { return !v.empty(); }));

    Logger::Debug("[MapGen] tileCellAttrs: %u tile indices, %u carry a frame\n",
                  static_cast<unsigned>(attrs.size()),
                  static_cast<unsigned>(withFrame));

    rmg.SetTileCellAttrs(std::move(attrs));
    RandomMapGenerator::s_bUseHostData = true;
    return true;
}
