#include "MapPngEncoder.h"

// zlib is vendored at the repository root and compiled directly into this
// module, so the header is located relative to this file instead of relying on
// a per-configuration include path.
#include "../../../zlib/zlib.h"

#include <cstdlib>
#include <cstring>

namespace
{
    constexpr size_t kOutputSize = size_t(1) << 20; // compressed bytes per IDAT chunk
    constexpr int kCompressionLevel = 6;
    constexpr int kBytesPerPixel = 3; // 24bpp RGB

    void WriteBigEndian(BYTE* destination, uint32_t value)
    {
        destination[0] = static_cast<BYTE>(value >> 24);
        destination[1] = static_cast<BYTE>(value >> 16);
        destination[2] = static_cast<BYTE>(value >> 8);
        destination[3] = static_cast<BYTE>(value);
    }

    inline int Paeth(int left, int above, int upperLeft)
    {
        const int estimate = left + above - upperLeft;
        const int distanceLeft = abs(estimate - left);
        const int distanceAbove = abs(estimate - above);
        const int distanceUpperLeft = abs(estimate - upperLeft);
        if (distanceLeft <= distanceAbove && distanceLeft <= distanceUpperLeft) return left;
        return distanceAbove <= distanceUpperLeft ? above : upperLeft;
    }

    // Standard minimum-sum-of-absolute-differences heuristic used to pick the
    // PNG row filter. Without it, incompressible-looking input such as dithered
    // terrain can produce files many times larger than the system encoder.
    unsigned int FilterScore(const BYTE* data, size_t length)
    {
        unsigned int score = 0;
        for (size_t i = 0; i < length; ++i)
        {
            const int value = static_cast<signed char>(data[i]);
            score += static_cast<unsigned int>(value < 0 ? -value : value);
        }
        return score;
    }
}

MapPngEncoder::~MapPngEncoder()
{
    Close();
}

void MapPngEncoder::Close()
{
    if (stream)
    {
        deflateEnd(static_cast<z_stream*>(stream));
        delete static_cast<z_stream*>(stream);
        stream = nullptr;
    }
    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }
}

bool MapPngEncoder::Begin(const wchar_t* path, uint32_t newWidth, uint32_t newHeight)
{
    Close();
    error = "";
    width = newWidth;
    height = newHeight;
    rowsWritten = 0;
    outputUsed = 0;
    if (!path || width == 0 || height == 0)
    {
        error = "invalid output path or dimensions";
        return false;
    }
    file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        error = "cannot create output file";
        return false;
    }
    static const BYTE signature[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    if (!WriteAll(signature, sizeof(signature)))
        return false;
    BYTE header[13] = {};
    WriteBigEndian(header, width);
    WriteBigEndian(header + 4, height);
    header[8] = 8;  // bit depth
    header[9] = 2;  // truecolor RGB
    header[10] = 0; // deflate
    header[11] = 0; // adaptive filtering
    header[12] = 0; // no interlace
    if (!WriteChunk("IHDR", header, sizeof(header)))
        return false;
    rowBytes = static_cast<size_t>(width) * kBytesPerPixel;
    current.assign(rowBytes, 0);
    previous.assign(rowBytes, 0);
    candidate.assign(rowBytes, 0);
    filtered.assign(rowBytes + 1, 0);
    output.resize(kOutputSize);
    auto* zs = new z_stream{};
    const int result = deflateInit2(zs, kCompressionLevel, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY);
    if (result != Z_OK)
    {
        delete zs;
        error = "cannot initialize deflate stream";
        return false;
    }
    stream = zs;
    return true;
}

bool MapPngEncoder::WriteAll(const void* data, size_t length)
{
    const BYTE* cursor = static_cast<const BYTE*>(data);
    while (length)
    {
        const DWORD block = static_cast<DWORD>(length > (size_t(1) << 20) ? (size_t(1) << 20) : length);
        DWORD written = 0;
        if (!WriteFile(file, cursor, block, &written, nullptr) || written != block)
        {
            error = "file write failed";
            return false;
        }
        cursor += written;
        length -= written;
    }
    return true;
}

bool MapPngEncoder::WriteChunk(const char* type, const BYTE* data, uint32_t length)
{
    BYTE header[8] = {};
    WriteBigEndian(header, length);
    memcpy(header + 4, type, 4);
    if (!WriteAll(header, sizeof(header)))
        return false;
    if (length && !WriteAll(data, length))
        return false;
    uLong checksum = crc32(0L, Z_NULL, 0);
    checksum = crc32(checksum, reinterpret_cast<const Bytef*>(header + 4), 4);
    if (length)
        checksum = crc32(checksum, data, length);
    BYTE trailer[4] = {};
    WriteBigEndian(trailer, static_cast<uint32_t>(checksum));
    return WriteAll(trailer, sizeof(trailer));
}

bool MapPngEncoder::Deflate(int flush)
{
    auto* zs = static_cast<z_stream*>(stream);
    for (;;)
    {
        if (outputUsed == output.size())
        {
            if (!WriteChunk("IDAT", output.data(), static_cast<uint32_t>(outputUsed)))
                return false;
            outputUsed = 0;
        }
        zs->next_out = output.data() + outputUsed;
        zs->avail_out = static_cast<uInt>(output.size() - outputUsed);
        const int result = deflate(zs, flush);
        if (result == Z_STREAM_ERROR || result == Z_MEM_ERROR)
        {
            error = "deflate encoding failed";
            return false;
        }
        outputUsed = output.size() - zs->avail_out;
        if (result == Z_STREAM_END)
            return true;
        if (flush != Z_FINISH && zs->avail_in == 0)
            return true;
    }
}

bool MapPngEncoder::ApplyBestFilter()
{
    // Filter type 0 must always be a valid fallback.
    filtered[0] = 0;
    memcpy(filtered.data() + 1, current.data(), rowBytes);
    unsigned int bestScore = FilterScore(current.data(), rowBytes);
    const bool hasPrevious = rowsWritten > 0;

    for (int type = 1; type <= 4; ++type)
    {
        for (size_t i = 0; i < rowBytes; ++i)
        {
            const int raw = current[i];
            const int left = i >= kBytesPerPixel ? current[i - kBytesPerPixel] : 0;
            const int above = hasPrevious ? previous[i] : 0;
            int predicted = 0;
            switch (type)
            {
            case 1: predicted = left; break;
            case 2: predicted = above; break;
            case 3: predicted = (left + above) >> 1; break;
            default:
                predicted = Paeth(left, above,
                    i >= kBytesPerPixel && hasPrevious ? previous[i - kBytesPerPixel] : 0);
                break;
            }
            candidate[i] = static_cast<BYTE>(raw - predicted);
        }
        const unsigned int score = FilterScore(candidate.data(), rowBytes);
        if (score < bestScore)
        {
            bestScore = score;
            filtered[0] = static_cast<BYTE>(type);
            memcpy(filtered.data() + 1, candidate.data(), rowBytes);
        }
    }
    return true;
}

bool MapPngEncoder::Write(Gdiplus::Bitmap& bitmap)
{
    if (!stream)
    {
        error = "encoder not started";
        return false;
    }
    if (bitmap.GetWidth() != width || bitmap.GetHeight() == 0)
    {
        error = "bitmap size mismatch";
        return false;
    }
    Gdiplus::BitmapData data{};
    Gdiplus::Rect rect(0, 0, static_cast<INT>(bitmap.GetWidth()), static_cast<INT>(bitmap.GetHeight()));
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &data) != Gdiplus::Ok)
    {
        error = "cannot lock bitmap pixels";
        return false;
    }
    auto* zs = static_cast<z_stream*>(stream);
    bool success = true;
    const UINT bitmapHeight = bitmap.GetHeight();
    for (UINT y = 0; y < bitmapHeight; ++y)
    {
        if (rowsWritten >= height)
        {
            error = "more rows than the declared image height";
            success = false;
            break;
        }
        const BYTE* source = static_cast<const BYTE*>(data.Scan0)
            + static_cast<ptrdiff_t>(y) * data.Stride;
        // GDI+ 24bppRGB stores channels as BGR in memory; PNG needs RGB.
        BYTE* destination = current.data();
        for (UINT x = 0; x < width; ++x)
        {
            destination[0] = source[2];
            destination[1] = source[1];
            destination[2] = source[0];
            source += 3;
            destination += 3;
        }
        ApplyBestFilter();
        zs->next_in = filtered.data();
        zs->avail_in = static_cast<uInt>(filtered.size());
        if (!Deflate(Z_NO_FLUSH))
        {
            success = false;
            break;
        }
        ++rowsWritten;
        current.swap(previous);
    }
    if (bitmap.UnlockBits(&data) != Gdiplus::Ok && success)
    {
        error = "cannot unlock bitmap pixels";
        success = false;
    }
    return success;
}

bool MapPngEncoder::Finish()
{
    if (!stream)
    {
        error = "encoder not started";
        return false;
    }
    if (rowsWritten != height)
    {
        error = "incomplete image rows";
        return false;
    }
    auto* zs = static_cast<z_stream*>(stream);
    zs->next_in = Z_NULL;
    zs->avail_in = 0;
    if (!Deflate(Z_FINISH))
        return false;
    if (outputUsed)
    {
        if (!WriteChunk("IDAT", output.data(), static_cast<uint32_t>(outputUsed)))
            return false;
        outputUsed = 0;
    }
    if (!WriteChunk("IEND", nullptr, 0))
        return false;
    Close();
    return true;
}
