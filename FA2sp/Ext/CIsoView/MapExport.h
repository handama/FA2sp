#pragma once

#include "MapPngEncoder.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <gdiplus.h>
#include <cstdint>
#include <string>
#include <cstddef>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

class MapExportFile
{
    std::wstring temporary;
    std::wstring destination;
    bool uninitialize = false;
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    Microsoft::WRL::ComPtr<IWICStream> stream;
    Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
    Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frame;
    UINT rowsWritten = 0;
    UINT imageHeight = 0;
    MapPngEncoder pngWriter;
    bool usePngWriter = false;

public:
    MapExportFile() = default;
    MapExportFile(const MapExportFile&) = delete;
    MapExportFile& operator=(const MapExportFile&) = delete;

    ~MapExportFile()
    {
        Close();
        if (!temporary.empty())
            DeleteFileW(temporary.c_str());
        if (uninitialize)
            CoUninitialize();
    }

    void Close()
    {
        pngWriter.Close();
        usePngWriter = false;
        frame.Reset();
        encoder.Reset();
        stream.Reset();
        factory.Reset();
    }

    const char* PngError() const { return pngWriter.LastError(); }

    HRESULT Create(const std::wstring& path)
    {
        const DWORD length = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
        if (!length) return HRESULT_FROM_WIN32(GetLastError());
        destination.resize(length);
        const DWORD written = GetFullPathNameW(path.c_str(), length, destination.data(), nullptr);
        if (!written || written >= length) return E_INVALIDARG;
        destination.resize(written);
        const auto slash = destination.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return E_INVALIDARG;
        const auto directory = destination.substr(0, slash + 1);
        WCHAR name[MAX_PATH]{};
        if (!GetTempFileNameW(directory.c_str(), L"FA2", 0, name))
            return HRESULT_FROM_WIN32(GetLastError());
        temporary = name;
        return S_OK;
    }

    const wchar_t* Path() const { return temporary.c_str(); }

    // WIC is only used for JPEG now, so the encoder is always the JPEG one.
    HRESULT InitializeJpeg(UINT width, UINT height)
    {
        if (width > 65535 || height > 65535)
            return WINCODEC_ERR_IMAGESIZEOUTOFRANGE;
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        uninitialize = SUCCEEDED(hr);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
            return hr;
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(factory.GetAddressOf()));
        if (FAILED(hr)) return hr;
        hr = factory->CreateStream(stream.GetAddressOf());
        if (FAILED(hr)) return hr;
        hr = stream->InitializeFromFilename(Path(), GENERIC_WRITE);
        if (FAILED(hr)) return hr;
        hr = factory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, encoder.GetAddressOf());
        if (FAILED(hr)) return hr;
        hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
        if (FAILED(hr)) return hr;
        Microsoft::WRL::ComPtr<IPropertyBag2> options;
        hr = encoder->CreateNewFrame(frame.GetAddressOf(), options.GetAddressOf());
        if (FAILED(hr)) return hr;
        PROPBAG2 property{};
        property.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
        VARIANT value{};
        value.vt = VT_R4;
        value.fltVal = 0.8f;
        hr = options->Write(1, &property, &value);
        if (FAILED(hr)) return hr;
        hr = frame->Initialize(options.Get());
        if (FAILED(hr)) return hr;
        hr = frame->SetSize(width, height);
        if (FAILED(hr)) return hr;
        WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
        hr = frame->SetPixelFormat(&format);
        if (FAILED(hr)) return hr;
        if (!IsEqualGUID(format, GUID_WICPixelFormat24bppBGR))
            return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        imageHeight = height;
        return S_OK;
    }

    // PNG streaming writer used for every PNG export that is not the in-memory
    // whole-bitmap GDI+ path.
    HRESULT InitializePng(UINT width, UINT height)
    {
        Close();
        if (!pngWriter.Begin(Path(), width, height))
            return E_FAIL;
        usePngWriter = true;
        return S_OK;
    }

    HRESULT Write(Gdiplus::Bitmap& bitmap)
    {
        if (usePngWriter)
            return pngWriter.Write(bitmap) ? S_OK : E_FAIL;
        Gdiplus::BitmapData data{};
        Gdiplus::Rect rect(0, 0, bitmap.GetWidth(), bitmap.GetHeight());
        if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &data) != Gdiplus::Ok)
            return E_FAIL;
        HRESULT hr = S_OK;
        const UINT stride = (bitmap.GetWidth() * 3 + 3) & ~3u;
        for (UINT y = 0; y < bitmap.GetHeight() && SUCCEEDED(hr); ++y)
        {
            auto row = static_cast<BYTE*>(data.Scan0) + static_cast<ptrdiff_t>(y) * data.Stride;
            hr = frame->WritePixels(1, stride, stride, row);
            if (SUCCEEDED(hr)) ++rowsWritten;
        }
        if (bitmap.UnlockBits(&data) != Gdiplus::Ok && SUCCEEDED(hr))
            hr = E_FAIL;
        return hr;
    }

    HRESULT Finish()
    {
        if (usePngWriter)
        {
            if (!pngWriter.Finish()) return E_FAIL;
            usePngWriter = false;
            return S_OK;
        }
        if (rowsWritten != imageHeight) return E_FAIL;
        HRESULT hr = frame->Commit();
        if (SUCCEEDED(hr)) hr = encoder->Commit();
        Close();
        return hr;
    }

    HRESULT Publish()
    {
        Close();
        if (!MoveFileExW(Path(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return HRESULT_FROM_WIN32(GetLastError());
        temporary.clear();
        return S_OK;
    }
};
