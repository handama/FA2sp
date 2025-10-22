#include "lzo.h"
#include "lzo1x.h"
#include "algorithm"

std::string lzo::compress(const void* src, int slen)
{
	std::string ret;
	
	constexpr int block_size = 8 * 1024;

	char* dictionary = new char[8 * block_size];
	unsigned char* buffer = new unsigned char[2 * block_size];
	unsigned char* ptr = (unsigned char*)src;
	while (slen > 0) 
	{
		int blockSize = std::min(block_size, slen);
		size_t length = 2 * block_size - 4;
		lzo1x_1_compress((unsigned char*)ptr, blockSize, &buffer[4], &length, dictionary);
		reinterpret_cast<unsigned short*>(buffer)[0] = static_cast<unsigned short>(length);
		reinterpret_cast<unsigned short*>(buffer)[1] = static_cast<unsigned short>(blockSize);
		ret.append((const char* const)buffer, length + 4);
		ptr += block_size;
		slen -= block_size;
	}

	delete[] dictionary;
	delete[] buffer;

	return ret;
}

std::string lzo::compressIsoMapPack5(const IsoMapPack5Entry* src, int slen)
{
    std::string ret;
    constexpr int block_size = 8 * 1024;

    int count = slen / sizeof(IsoMapPack5Entry);

    std::vector<IsoMapPack5Entry> entries(src, src + count);

    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [](const IsoMapPack5Entry& e) {
                return ((e.TileIndex == 0 || e.TileIndex == 0xFFFF) && e.Level == 0);
            }),
        entries.end()
    );

    std::sort(entries.begin(), entries.end(),
        [](const IsoMapPack5Entry& a, const IsoMapPack5Entry& b) {
            if (a.X != b.X) return a.X < b.X;
            if (a.Level != b.Level) return a.Level < b.Level;
            return a.TileIndex < b.TileIndex;
        }
    );

	if (entries.empty())
		entries.push_back({0});

    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(entries.data());
    int new_slen = static_cast<int>(entries.size() * sizeof(IsoMapPack5Entry));

    char* dictionary = new char[8 * block_size];
    unsigned char* buffer = new unsigned char[2 * block_size];

    while (new_slen > 0)
    {
        int blockSize = std::min(block_size, new_slen);
        size_t length = 2 * block_size - 4;
        lzo1x_1_compress(ptr, blockSize, &buffer[4], &length, dictionary);
        reinterpret_cast<unsigned short*>(buffer)[0] = static_cast<unsigned short>(length);
        reinterpret_cast<unsigned short*>(buffer)[1] = static_cast<unsigned short>(blockSize);
        ret.append(reinterpret_cast<const char*>(buffer), length + 4);
        ptr += blockSize;
        new_slen -= blockSize;
    }

    delete[] dictionary;
    delete[] buffer;

    return ret;
}

std::string lzo::decompress(const void* src, int slen)
{
	std::string ret;

	constexpr int block_size = 8 * 1024;
	unsigned char* Buffer = new unsigned char[2 * block_size];

	unsigned char* ptr = (unsigned char*)src;
	
	while (slen > 0)
	{
		auto len = reinterpret_cast<unsigned short*>(ptr)[0];
		size_t length = 2 * block_size;
		lzo1x_decompress(ptr, len, Buffer, &length, nullptr);
		ret.append((const char* const)Buffer, length);
		ptr += 4 + len;
		slen -= 4 + len;
	}

	delete[] Buffer;
	return ret;
}
