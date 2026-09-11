#pragma once

#include "../FA2sp.h"
#include <vector>
#include <string>
#include <unordered_map>

class AudioBagSound
{
public:
    struct BagEntry
    {
        int32_t bagIndex;
        uint32_t offset;
        uint32_t size;
        int32_t samplerate;
        int32_t flags;
        int32_t chunkSize;
    };

    static bool TryBuildWav(const char* soundName, std::vector<byte>& outWav, int volumePercent = 100);
    static bool TryBuildWavFromFile(const char* pSoundName, std::vector<byte>& outWav, int volumePercent = 100);

    static void LoadIndexes();
    static void ClearIndexes();
    static const BagEntry* FindEntry(const char* soundName);

private:
    static std::unordered_map<std::string, BagEntry> entries;

    static bool ParseIdx(const byte* idx, DWORD idxSize, std::unordered_map<std::string, BagEntry>& out);
    static bool BuildWavFromBag(const BagEntry& e, int volumePercent, std::vector<byte>& outWav);
};
