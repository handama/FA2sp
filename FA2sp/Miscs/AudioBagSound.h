#pragma once

#include "../FA2sp.h"
#include <vector>
#include <string>
#include <unordered_map>

class AudioBagSound
{
public:
    static bool TryBuildWav(const char* soundName, std::vector<byte>& outWav, int volumePercent = 100);
    static bool TryBuildWavFromFile(const char* pSoundName, std::vector<byte>& outWav, int volumePercent = 100);
    static void ClearCache();

private:
    struct Entry
    {
        uint32_t offset;
        uint32_t size;
        int32_t samplerate;
        int32_t flags;
        int32_t chunkSize;
    };

    static bool EnsureLoaded();
    static void Fail();

    static bool loaded;
    static bool loadFailed;
    static std::unordered_map<std::string, Entry> entries;
    static std::vector<byte> bagData;
};
