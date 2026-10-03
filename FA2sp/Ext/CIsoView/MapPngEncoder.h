#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <cstdint>
#include <vector>

// Streaming PNG writer built on the statically linked zlib deflate stream.
// It exists because the Windows WIC PNG encoder rejects single image
// dimensions above 65535 pixels (WINCODEC_ERR_IMAGESIZEOUTOFRANGE), while the
// PNG container itself allows 32-bit dimensions. Rows are written top to
// bottom through a bounded buffer, so the memory cost is O(row width) instead
// of O(whole image).
class MapPngEncoder
{
public:
    MapPngEncoder() = default;
    ~MapPngEncoder();
    MapPngEncoder(const MapPngEncoder&) = delete;
    MapPngEncoder& operator=(const MapPngEncoder&) = delete;

    bool Begin(const wchar_t* path, uint32_t width, uint32_t height);
    bool Write(Gdiplus::Bitmap& bitmap);
    bool Finish();
    void Close();

    uint64_t RowsWritten() const { return rowsWritten; }
    const char* LastError() const { return error; }

private:
    bool WriteAll(const void* data, size_t length);
    bool WriteChunk(const char* type, const BYTE* data, uint32_t length);
    bool Deflate(int flush);
    bool ApplyBestFilter();

    HANDLE file = INVALID_HANDLE_VALUE;
    void* stream = nullptr; // z_stream, kept opaque so callers avoid zlib.h
    // Raw current/previous scanlines plus the filtered scanline written to
    // deflate. Kept as three row-sized buffers so the memory cost stays O(width).
    std::vector<BYTE> current;
    std::vector<BYTE> previous;
    std::vector<BYTE> candidate;
    std::vector<BYTE> filtered;
    size_t rowBytes = 0;
    std::vector<BYTE> output;
    size_t outputUsed = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t rowsWritten = 0;
    const char* error = "";
};
