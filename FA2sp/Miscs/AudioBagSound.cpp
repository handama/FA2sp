#include "AudioBagSound.h"
#include "../Ext/CLoading/Body.h"
#include "../Logger.h"
#include "../../FA2pp/FAMemory.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

std::unordered_map<std::string, AudioBagSound::BagEntry> AudioBagSound::entries;

static uint32_t ReadU32(const byte* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t ReadI32(const byte* p)
{
    return (int32_t)ReadU32(p);
}

static uint16_t ReadU16(const byte* p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static const int aud_ima_index_adjust_table[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

static const int aud_ima_step_table[89] =
{
    7,     8,     9,     10,    11,    12,     13,    14,    16,
    17,    19,    21,    23,    25,    28,     31,    34,    37,
    41,    45,    50,    55,    60,    66,     73,    80,    88,
    97,    107,   118,   130,   143,   157,    173,   190,   209,
    230,   253,   279,   307,   337,   371,    408,   449,   494,
    544,   598,   658,   724,   796,   876,    963,   1060,  1166,
    1282,  1411,  1552,  1707,  1878,  2066,   2272,  2499,  2749,
    3024,  3327,  3660,  4026,  4428,  4871,   5358,  5894,  6484,
    7132,  7845,  8630,  9493,  10442, 11487,  12635, 13899, 15289,
    16818, 18500, 20350, 22385, 24623, 27086,  29794, 32767
};

static void AudDecodeImaChunk(const byte* audio_in, short* audio_out, int& index, int& sample, int cs_chunk)
{
    for (int sample_index = 0; sample_index < cs_chunk; sample_index++)
    {
        int code = audio_in[sample_index >> 1];
        code = sample_index & 1 ? code >> 4 : code & 0xf;
        int step = aud_ima_step_table[index];
        int delta = step >> 3;
        if (code & 1)
            delta += step >> 2;
        if (code & 2)
            delta += step >> 1;
        if (code & 4)
            delta += step;
        if (code & 8)
        {
            sample -= delta;
            if (sample < -32768)
                sample = -32768;
        }
        else
        {
            sample += delta;
            if (sample > 32767)
                sample = 32767;
        }
        audio_out[sample_index] = short(sample);
        index += aud_ima_index_adjust_table[code & 7];
        if (index < 0)
            index = 0;
        else if (index > 88)
            index = 88;
    }
}

static void ScalePcmVolume(std::vector<byte>& body, int volumePercent)
{
    if (volumePercent >= 100)
        return;
    if (volumePercent < 0)
        volumePercent = 0;
    short* p = reinterpret_cast<short*>(body.data());
    size_t count = body.size() / 2;
    for (size_t i = 0; i < count; ++i)
    {
        int v = p[i] * volumePercent / 100;
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;
        p[i] = (short)v;
    }
}

static bool DecodeImaAdpcmToPcmBody(const byte* r, int cbS, int chunkSize, int channels, std::vector<byte>& body, int& dataSize)
{
    int cChunks = (cbS + chunkSize - 1) / chunkSize;
    int mcSamples = ((cbS - 4 * channels * cChunks) << 1) + cChunks * channels;
    if (mcSamples <= 0)
        return false;
    std::vector<short> samples(mcSamples);
    short* w = samples.data();
    int csRemaining = mcSamples;
    while (csRemaining)
    {
        if (channels == 1)
        {
            int chunkSample = (int16_t)ReadU16(r);
            int chunkIndex = r[2];
            if (chunkIndex > 88)
                chunkIndex = 88;
            r += 4;
            *w++ = (short)chunkSample;
            csRemaining--;
            int csChunk = std::min(csRemaining, (chunkSize - 4) << 1);
            int index = chunkIndex;
            int sample = chunkSample;
            AudDecodeImaChunk(r, w, index, sample, csChunk);
            r += csChunk >> 1;
            w += csChunk;
            csRemaining -= csChunk;
        }
        else
        {
            int leftSample = (int16_t)ReadU16(r);
            int leftIndex = r[2];
            if (leftIndex > 88)
                leftIndex = 88;
            r += 4;
            *w++ = (short)leftSample;
            csRemaining--;
            int rightSample = (int16_t)ReadU16(r);
            int rightIndex = r[2];
            if (rightIndex > 88)
                rightIndex = 88;
            r += 4;
            *w++ = (short)rightSample;
            csRemaining--;
            int csChunk = std::min(csRemaining, (chunkSize - 8) << 1);
            while (csChunk >= 16)
            {
                short leftT[8];
                short rightT[8];
                AudDecodeImaChunk(r, leftT, leftIndex, leftSample, 8);
                r += 4;
                AudDecodeImaChunk(r, rightT, rightIndex, rightSample, 8);
                r += 4;
                for (int i = 0; i < 8; i++)
                {
                    *w++ = leftT[i];
                    *w++ = rightT[i];
                }
                csChunk -= 16;
                csRemaining -= 16;
            }
            if (csRemaining < 16)
                csRemaining = 0;
        }
    }
    int cSamples = (int)samples.size() / channels;
    dataSize = channels * cSamples << 1;
    body.resize(dataSize);
    memcpy(body.data(), samples.data(), dataSize);
    return true;
}

static void BuildPcmWav(std::vector<byte>& body, int dataSize, int channels, int samplerate, std::vector<byte>& outWav)
{
    std::vector<byte> wav;
    wav.reserve(44 + dataSize);
    auto pushBytes = [&wav](const void* data, size_t size)
    {
        const byte* p = static_cast<const byte*>(data);
        wav.insert(wav.end(), p, p + size);
    };
    auto pushU16 = [&](uint16_t v)
    {
        byte b[2] = { (byte)(v & 0xff), (byte)(v >> 8) };
        pushBytes(b, 2);
    };
    auto pushU32 = [&](uint32_t v)
    {
        byte b[4] = { (byte)(v & 0xff), (byte)((v >> 8) & 0xff), (byte)((v >> 16) & 0xff), (byte)(v >> 24) };
        pushBytes(b, 4);
    };
    pushBytes("RIFF", 4);
    pushU32(36 + dataSize);
    pushBytes("WAVE", 4);
    pushBytes("fmt ", 4);
    pushU32(16);
    pushU16(1);
    pushU16((uint16_t)channels);
    pushU32((uint32_t)samplerate);
    pushU32(2 * channels * (uint32_t)samplerate);
    pushU16((uint16_t)(2 * channels));
    pushU16(16);
    pushBytes("data", 4);
    pushU32(dataSize);
    pushBytes(body.data(), body.size());
    outWav = std::move(wav);
}

void AudioBagSound::ClearIndexes()
{
    std::unordered_map<std::string, BagEntry>().swap(entries);
}

bool AudioBagSound::ParseIdx(const byte* idx, DWORD idxSize, std::unordered_map<std::string, BagEntry>& out)
{
    if (!idx || idxSize < 12 || memcmp(idx, "GABA", 4) != 0 || ReadI32(idx + 4) != 2)
        return false;
    int32_t count = ReadI32(idx + 8);
    if (count < 0 || (uint64_t)idxSize != 12ull + 36ull * (uint32_t)count)
        return false;

    for (int32_t i = 0; i < count; ++i)
    {
        const byte* p = idx + 12 + (size_t)i * 36;
        size_t nameLen = 16;
        for (size_t j = 0; j < 16; ++j)
        {
            if (p[j] == 0)
            {
                nameLen = j;
                break;
            }
        }
        if (nameLen == 0)
            continue;
        std::string key;
        key.reserve(nameLen);
        for (size_t j = 0; j < nameLen; ++j)
            key.push_back((char)tolower((unsigned char)p[j]));
        BagEntry& e = out[key];
        e.bagIndex = 0;
        e.offset = ReadU32(p + 16);
        e.size = ReadU32(p + 20);
        e.samplerate = ReadI32(p + 24);
        e.flags = ReadI32(p + 28);
        e.chunkSize = ReadI32(p + 32);
    }
    return true;
}

void AudioBagSound::LoadIndexes()
{
    ClearIndexes();

    auto loadOne = [](int index) -> void
    {
        char idxName[16];
        if (index > 0)
            sprintf_s(idxName, "audio%02d.idx", index);
        else
            strcpy_s(idxName, "audio.idx");
        DWORD idxSize = 0;
        auto idx = static_cast<byte*>(CLoadingExt::GetExtension()->ReadWholeFile(idxName, &idxSize, false, false));
        if (!idx)
            return;
        std::unordered_map<std::string, BagEntry> parsed;
        if (!ParseIdx(idx, idxSize, parsed))
            Logger::Raw("[AudioBag] %s invalid!\n", idxName);
        else
            Logger::Raw("[AudioBag] %s: %d sounds loaded.\n", idxName, (int)parsed.size());
        GameDeleteArray(idx, idxSize);
        for (auto& [name, e] : parsed)
        {
            e.bagIndex = index;
            entries[name] = e;
        }
    };

    loadOne(0);
    for (int i = 1; i <= 99; ++i)
        loadOne(i);
    Logger::Raw("[AudioBag] Total %d sound entries.\n", (int)entries.size());
}

const AudioBagSound::BagEntry* AudioBagSound::FindEntry(const char* soundName)
{
    if (!soundName || !*soundName)
        return nullptr;
    size_t len = strlen(soundName);
    std::string key;
    key.reserve(len);
    for (size_t i = 0; i < len; ++i)
        key.push_back((char)tolower((unsigned char)soundName[i]));
    auto it = entries.find(key);
    return it == entries.end() ? nullptr : &it->second;
}

bool AudioBagSound::BuildWavFromBag(const BagEntry& e, int volumePercent, std::vector<byte>& outWav)
{
    char bagName[16];
    if (e.bagIndex > 0)
        sprintf_s(bagName, "audio%02d.bag", e.bagIndex);
    else
        strcpy_s(bagName, "audio.bag");

    DWORD bagSize = 0;
    auto bag = static_cast<byte*>(CLoadingExt::GetExtension()->ReadWholeFile(bagName, &bagSize));
    if (!bag || bagSize == 0)
    {
        if (bag)
            GameDeleteArray(bag, bagSize);
        return false;
    }

    bool ok = false;
    if ((uint64_t)e.offset + e.size <= bagSize)
    {
        int channels = (e.flags & 1) ? 2 : 1;
        bool isPcm = (e.flags & 2) != 0;
        bool isAdpcm = (e.flags & 8) != 0;
        if ((isPcm || isAdpcm) && e.samplerate > 0 && e.size > 0 && !(isAdpcm && e.chunkSize <= 4 * channels))
        {
            std::vector<byte> body;
            int dataSize = 0;
            if (isPcm)
            {
                const byte* s = bag + e.offset;
                body.assign(s, s + e.size);
                dataSize = (int)e.size;
                ok = true;
            }
            else
                ok = DecodeImaAdpcmToPcmBody(bag + e.offset, (int)e.size, e.chunkSize, channels, body, dataSize);

            if (ok)
            {
                ScalePcmVolume(body, volumePercent);
                BuildPcmWav(body, dataSize, channels, e.samplerate, outWav);
            }
        }
    }
    GameDeleteArray(bag, bagSize);
    return ok;
}

bool AudioBagSound::TryBuildWav(const char* soundName, std::vector<byte>& outWav, int volumePercent)
{
    outWav.clear();
    const BagEntry* e = FindEntry(soundName);
    if (!e)
        return false;
    return BuildWavFromBag(*e, volumePercent, outWav);
}

bool AudioBagSound::TryBuildWavFromFile(const char* pSoundName, std::vector<byte>& outWav, int volumePercent)
{
    outWav.clear();
    if (!pSoundName || !*pSoundName)
        return false;
    std::string fileName = pSoundName;
    fileName += ".wav";
    DWORD dwSize = 0;
    auto pFile = static_cast<byte*>(CLoadingExt::GetExtension()->ReadWholeFile(fileName.c_str(), &dwSize));
    if (!pFile)
        return false;
    bool result = false;
    if (dwSize >= 44)
    {
        int formatTag = 0, channels = 0, samplerate = 0, bitsPerSample = 0, blockAlign = 0;
        const byte* data = nullptr;
        int dataSize = 0;
        const byte* p = pFile + 12;
        const byte* end = pFile + dwSize;
        while (p + 8 <= end)
        {
            unsigned int cs = p[4] | p[5] << 8 | p[6] << 16 | p[7] << 24;
            const byte* cd = p + 8;
            if (cd + cs > end)
                break;
            if (!memcmp(p, "fmt ", 4) && cs >= 16)
            {
                formatTag = cd[0] | cd[1] << 8;
                channels = cd[2] | cd[3] << 8;
                samplerate = cd[4] | cd[5] << 8 | cd[6] << 16 | cd[7] << 24;
                blockAlign = cd[12] | cd[13] << 8;
                bitsPerSample = cd[14] | cd[15] << 8;
            }
            else if (!memcmp(p, "data", 4))
            {
                data = cd;
                dataSize = (int)cs;
            }
            p = cd + cs + (cs & 1);
        }
        std::vector<byte> body;
        int outDataSize = 0;
        if (channels >= 1 && channels <= 2 && samplerate > 0)
        {
            if (formatTag == 1 && bitsPerSample == 16 && data && dataSize >= 2)
            {
                outDataSize = dataSize & ~1;
                body.assign(data, data + outDataSize);
                result = true;
            }
            else if (formatTag == 0x11 && data && dataSize > 0 && blockAlign > 4 * channels)
            {
                result = DecodeImaAdpcmToPcmBody(data, dataSize, blockAlign, channels, body, outDataSize);
            }
        }
        if (result)
        {
            ScalePcmVolume(body, volumePercent);
            BuildPcmWav(body, outDataSize, channels, samplerate, outWav);
        }
    }
    GameDeleteArray(pFile, dwSize);
    return result;
}
