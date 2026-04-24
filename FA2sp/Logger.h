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
        : m_baseName(name),
        m_lastMsg("Start"),
        m_start(std::chrono::high_resolution_clock::now())
    {}

    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - m_start).count();

        Logger::Raw("[Timer] %s %s -> End: %.3f ms\n", m_baseName, m_lastMsg, duration_ms);
    }

    void printElapsed(const char* msg = nullptr) const
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(now - m_start).count();

        if (msg && std::strlen(msg) > 0) {
            Logger::Raw("[Timer] %s Elapsed (%s): %.3f ms\n", m_baseName, msg, duration_ms);
        }
        else {
            Logger::Raw("[Timer] %s Elapsed: %.3f ms\n", m_baseName, duration_ms);
        }
    }

    void printAndReset(const char* msg = nullptr)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(now - m_start).count();

        FString currentMsg = "Segment";
        if (msg && std::strlen(msg) > 0) {
            currentMsg = msg;
        }

        Logger::Raw("[Timer] %s %s -> %s: %.3f ms\n",
            m_baseName, m_lastMsg, currentMsg, duration_ms);

        m_lastMsg = currentMsg;
        m_start = now;
    }

private:
    FString m_baseName;
    FString m_lastMsg;
    std::chrono::high_resolution_clock::time_point m_start;
};

class FpsCounter {
public:
    FpsCounter(const std::string& name = "FPS")
        : m_name(name), m_frameCount(0) {
        m_lastTime = std::chrono::steady_clock::now();
    }

    void update() {
        m_frameCount++;

        auto currentTime = std::chrono::steady_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - m_lastTime).count();

        if (deltaTime >= 1.0) {
            Logger::Raw("[Timer] %s: %d fps\n", m_name, m_frameCount);

            m_frameCount = 0;
            m_lastTime = currentTime;
        }
    }

private:
    FString m_name;
    int m_frameCount;
    std::chrono::steady_clock::time_point m_lastTime;
};