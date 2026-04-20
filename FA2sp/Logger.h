#pragma once

#include "FA2PP.h"
#include "../FA2sp/Helpers/FString.h"
#include <chrono>

class Logger {
public:
    enum class kLoggerType { Raw = -1, Debug, Info, Warn, Error };
    static void Initialize();
    static void Close();
public:
    template<typename... Args>
    static void Write(kLoggerType type, const char* format, Args&&... args) {
        if (!bInitialized) {
            return;
        }

        FString message;
        message.Format(format, std::forward<Args>(args)...);
        message.toUTF8();
 
        std::string type_str;
        switch (type) {
        case kLoggerType::Raw:
            type_str = "";
            break;
        case kLoggerType::Debug:
            type_str = "Debug";
            break;
        case kLoggerType::Info:
            type_str = "Info";
            break;
        case kLoggerType::Warn:
            type_str = "Warn";
            break;
        case kLoggerType::Error:
            type_str = "Error";
            break;
        default:
            type_str = "";
            break;
        }

        if (type == kLoggerType::Raw) {
            fprintf_s(pFile, "%s", message.c_str());
        }
        else {
            fprintf_s(pFile, "[%s] %s", type_str.c_str(), message.c_str());
        }
        fflush(pFile);
    }

    template<typename... Args>
    static void Debug(const char* format, Args&&... args) {
        Write(kLoggerType::Debug, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Warn(const char* format, Args&&... args) {
        Write(kLoggerType::Warn, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Error(const char* format, Args&&... args) {
        Write(kLoggerType::Error, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Info(const char* format, Args&&... args) {
        Write(kLoggerType::Info, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Raw(const char* format, Args&&... args) {
        Write(kLoggerType::Raw, format, std::forward<Args>(args)...);
    }

    static void Put(const char*);
    static void Time(char*);
    static void Wrap(unsigned int cnt = 1);

private:
    static char pTime[24];
    static char pBuffer[0x800];
    static FILE* pFile;
    static bool bInitialized;
};

class ScopedTimer
{
public:
    explicit ScopedTimer(const char* name)
        : m_name(name), m_start(std::chrono::high_resolution_clock::now())
    {}

    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - m_start).count();

        Logger::Raw("%s took %.3f ms\n", m_name, duration_ms);
    }

private:
    const char* m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};