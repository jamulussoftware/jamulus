#pragma once
#include <string>
#include <fstream>
#include <mutex>

class DebugLogger {
public:
    static DebugLogger& instance() {
        static DebugLogger logger;
        return logger;
    }

    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        logfile_ << msg << std::endl;
        logfile_.flush();
    }

private:
    DebugLogger() : logfile_(getLogFilePath(), std::ios::app) {}
    ~DebugLogger() { logfile_.close(); }
    DebugLogger(const DebugLogger&) = delete;
    DebugLogger& operator=(const DebugLogger&) = delete;

    std::string getLogFilePath() {
#ifdef _WIN32
        char* tmp = nullptr;
        size_t len = 0;
        _dupenv_s(&tmp, &len, "TEMP");
        std::string path = tmp ? std::string(tmp) : ".";
        if (tmp) free(tmp);
        return path + "\\JamulusVST3_debug.log";
#else
        return "/tmp/JamulusVST3_debug.log";
#endif
    }

    std::ofstream logfile_;
    std::mutex mutex_;
};
