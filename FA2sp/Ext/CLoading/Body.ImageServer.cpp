#include <string>
#include <iostream>
#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>
#include <TlHelp32.h>
#include "../../FA2sp.h"
#include "Body.h"
#include <../FA2sp/Ext/CMapData/Body.h>
#include <thread>
#include "../../Helpers/Translations.h"
#include "../CFinalSunApp/Body.h"

HANDLE CLoadingExt::hPipeData = NULL;
std::atomic<bool> CLoadingExt::PingServerRunning = true;
FString CLoadingExt::PipeNameData;
FString CLoadingExt::PipeNamePing;
FString CLoadingExt::PipeName;

//static bool Is64BitProcess()
//{
//#if defined(_WIN64)
//    return true;
//#else
//    return false;
//#endif
//}
//
//static bool Is64BitOS()
//{
//    BOOL isWow64 = FALSE;
//
//    typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
//    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)
//        GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
//
//    if (fnIsWow64Process) {
//        if (!fnIsWow64Process(GetCurrentProcess(), &isWow64)) {
//            return false; 
//        }
//    }
//
//    return Is64BitProcess() || isWow64;
//}

bool CLoadingExt::StartImageServerProcess()
{
    if (!ExtConfigs::LoadImageDataFromServer)
        return false;

    PipeNameData = PipeName + "_data";
    PipeNamePing = PipeName + "_ping";

    FString exePath = CFinalSunAppExt::ExePathExt;
    exePath += "\\ImageServer.exe"; //Is64BitOS() ? "\\ImageServer64.exe" :

    FString command = exePath + " \"" + PipeName + "\"";

    Logger::Raw("[ImageServer] About to create process %s\n", exePath.c_str());

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    BOOL result = CreateProcessA(
        exePath.c_str(),
        const_cast<char*>(command.c_str()),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!result) {

        ::MessageBoxA(NULL,
            Translations::TranslateOrDefault("LaunchImageServerFailed", "Failed to launch image server!\n"
                "Please check whether ImageServer.exe exists,\n"
                "or turn off LoadImageDataFromServer."),
            Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);
        exit(0);

        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    ConnectToImageServer();

    return true;
}

bool CLoadingExt::ConnectToImageServer() 
{
    if (!ExtConfigs::LoadImageDataFromServer)
        return false;

    const DWORD totalTimeoutMs = 10000; 
    DWORD retryInterval = 30; 
    DWORD startTime = GetTickCount();

    while (true) {
        hPipeData = CreateFileA(
            PipeNameData.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        DWORD error = GetLastError();
        DWORD elapsed = GetTickCount() - startTime;

        if (hPipeData != INVALID_HANDLE_VALUE) {
            DWORD mode = PIPE_READMODE_MESSAGE;

            Logger::Raw("[ImageServer] Successfully connected to ImagePipe, handle: %x\n", hPipeData);
            StartPingThread();

            return true;
        }

        if (elapsed >= totalTimeoutMs) break;
        Sleep(retryInterval);
    }

    StartPingThread();
    return false;
}

void CLoadingExt::StartPingThread() 
{
    std::thread([]() {
        if (!ExtConfigs::LoadImageDataFromServer)
            return;

        while (CLoadingExt::PingServerRunning.load()) {
            HANDLE hPipePing = CreateFileA(
                PipeNamePing.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );
            Sleep(3000);

            DisconnectNamedPipe(hPipePing);
            CloseHandle(hPipePing);
        }
        }).detach();
}

bool CLoadingExt::WriteImageData(HANDLE hPipe, const FString& imageID, const ImageDataClassSafe* data)
{
    char charArray[256] = {};
    std::strncpy(charArray, imageID.c_str(), sizeof(charArray) - 1);

    size_t imageSize = data->FullWidth * data->FullHeight;
    size_t rangeSize = data->FullHeight * sizeof(ImageDataClassSafe::ValidRangeData);
    
    size_t totalSize = sizeof(char[256]) +
        sizeof(data->ValidX) + sizeof(data->ValidY) + sizeof(data->ValidWidth) + sizeof(data->ValidHeight) +
        sizeof(data->FullWidth) + sizeof(data->FullHeight) + sizeof(data->Flag) +
        sizeof(data->BuildingFlag) + sizeof(data->IsOverlay) + sizeof(data->pPalette) +
        imageSize + rangeSize;

    if (imageSize + rangeSize == 0)
        return false;

    std::vector<char> buffer(totalSize);
    char* ptr = buffer.data();
    
    memcpy(ptr, charArray, 256);
    ptr += 256;
    
    memcpy(ptr, &data->ValidX, sizeof(data->ValidX)); ptr += sizeof(data->ValidX);
    memcpy(ptr, &data->ValidY, sizeof(data->ValidY)); ptr += sizeof(data->ValidY);
    memcpy(ptr, &data->ValidWidth, sizeof(data->ValidWidth)); ptr += sizeof(data->ValidWidth);
    memcpy(ptr, &data->ValidHeight, sizeof(data->ValidHeight)); ptr += sizeof(data->ValidHeight);
    memcpy(ptr, &data->FullWidth, sizeof(data->FullWidth)); ptr += sizeof(data->FullWidth);
    memcpy(ptr, &data->FullHeight, sizeof(data->FullHeight)); ptr += sizeof(data->FullHeight);
    memcpy(ptr, &data->Flag, sizeof(data->Flag)); ptr += sizeof(data->Flag);
    memcpy(ptr, &data->BuildingFlag, sizeof(data->BuildingFlag)); ptr += sizeof(data->BuildingFlag);
    memcpy(ptr, &data->IsOverlay, sizeof(data->IsOverlay)); ptr += sizeof(data->IsOverlay);
    memcpy(ptr, &data->pPalette, sizeof(data->pPalette)); ptr += sizeof(data->pPalette);

    memcpy(ptr, data->pImageBuffer.get(), imageSize); ptr += imageSize;
    memcpy(ptr, data->pPixelValidRanges.get(), rangeSize); ptr += rangeSize;

    DWORD written = 0;
    WriteFile(hPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &written, NULL);

    return true;
}

bool CLoadingExt::SendImageToServer(const FString& imageID, const ImageDataClassSafe* imageData)
{
    if (!imageData->pImageBuffer)
        return false;

    WriteImageData(hPipeData, imageID, imageData);
    return true;
}

bool CLoadingExt::ReadImageData(HANDLE hPipe, ImageDataClassSafe& data)
{
    char buffer[256] = {};
    DWORD readBytes = 0;
    bool success = true;

    const size_t charSize = sizeof(char[256]);
    const size_t headerSize =
        sizeof(data.ValidX) + sizeof(data.ValidY) + sizeof(data.ValidWidth) + sizeof(data.ValidHeight) +
        sizeof(data.FullWidth) + sizeof(data.FullHeight) + sizeof(data.Flag) +
        sizeof(data.BuildingFlag) + sizeof(data.IsOverlay) + sizeof(data.pPalette);

    if (!ReadFile(hPipe, buffer, charSize, &readBytes, NULL))
        success = false;
    buffer[255] = '\0';

    if (!success || strncmp(buffer, "NOT_FOUND", 9) == 0)
    {
        return false;
    }
    std::vector<char> headerBuf(headerSize);
    if (!ReadFile(hPipe, headerBuf.data(), headerSize, &readBytes, NULL))
        success = false;

    const char* ptr = headerBuf.data();
    memcpy(&data.ValidX, ptr, sizeof(data.ValidX)); ptr += sizeof(data.ValidX);
    memcpy(&data.ValidY, ptr, sizeof(data.ValidY)); ptr += sizeof(data.ValidY);
    memcpy(&data.ValidWidth, ptr, sizeof(data.ValidWidth)); ptr += sizeof(data.ValidWidth);
    memcpy(&data.ValidHeight, ptr, sizeof(data.ValidHeight)); ptr += sizeof(data.ValidHeight);
    memcpy(&data.FullWidth, ptr, sizeof(data.FullWidth)); ptr += sizeof(data.FullWidth);
    memcpy(&data.FullHeight, ptr, sizeof(data.FullHeight)); ptr += sizeof(data.FullHeight);
    memcpy(&data.Flag, ptr, sizeof(data.Flag)); ptr += sizeof(data.Flag);
    memcpy(&data.BuildingFlag, ptr, sizeof(data.BuildingFlag)); ptr += sizeof(data.BuildingFlag);
    memcpy(&data.IsOverlay, ptr, sizeof(data.IsOverlay)); ptr += sizeof(data.IsOverlay);
    memcpy(&data.pPalette, ptr, sizeof(data.pPalette)); ptr += sizeof(data.pPalette);

    size_t imageSize = data.FullWidth * data.FullHeight;
    size_t rangeSize = data.FullHeight * sizeof(ImageDataClassSafe::ValidRangeData);

    data.pImageBuffer = std::make_unique<unsigned char[]>(imageSize);
    data.pPixelValidRanges = std::make_unique<ImageDataClassSafe::ValidRangeData[]>(data.FullHeight);

    if (!ReadFile(hPipe, data.pImageBuffer.get(), static_cast<DWORD>(imageSize), &readBytes, NULL))
        success = false;
    if (!ReadFile(hPipe, data.pPixelValidRanges.get(), static_cast<DWORD>(rangeSize), &readBytes, NULL))
        success = false;

    return success;
}

bool CLoadingExt::RequestImageFromServer(const FString& imageID, ImageDataClassSafe& outImageData)
{
    char requestID[256] = {};
    requestID[0] = '#';
    strncpy(requestID + 1, imageID.c_str(), 0xFF);
    requestID[sizeof(requestID) - 1] = '\0';

    DWORD bytesWritten = 0;
    WriteFile(hPipeData, requestID, sizeof(requestID), &bytesWritten, NULL);
    ReadImageData(hPipeData, outImageData);
    return true;
}

void CLoadingExt::SendRequestText(const char* text)
{
    DWORD bytesWritten = 0;
    char buffer[256] = {};
    WriteFile(hPipeData, text, strlen(text) + 1, &bytesWritten, NULL);
}
