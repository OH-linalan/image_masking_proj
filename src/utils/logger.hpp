#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>

using namespace std;

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERR
};

class Logger {
private:
    LogLevel m_minLevel;
    ofstream m_fileStream;
    
    string getCurrentTime() {
        auto now = chrono::system_clock::now();
        auto time_t_now = chrono::system_clock::to_time_t(now);
        ostringstream oss;
        oss << put_time(localtime(&time_t_now), "%H:%M:%S");
        return oss.str();
    }

    string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERR:   return "ERROR";
            default:              return "INFO";
        }
    }
public:
    Logger(LogLevel level = LogLevel::DEBUG, const string& filename = "")
        : m_minLevel(level)
    {
        if (!filename.empty()) {
            m_fileStream.open(filename, ios::out | ios::trunc);
        }
    }

    ~Logger() {
        if (m_fileStream.is_open()) {
            m_fileStream.flush();
            m_fileStream.close();
        }
    }

    void log(LogLevel level, const string& message) {
        if (level < m_minLevel) return;

        if (m_fileStream.is_open()) {
            string logMsg = "[" + getCurrentTime() + "] [" + levelToString(level) + "] " + message + "\n";
            m_fileStream << logMsg;
        }
    }
};

extern Logger g_logger;