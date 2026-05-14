#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <filesystem>

class DebugLogger {
public:
    static DebugLogger& instance() {
        static DebugLogger logger;
        return logger;
    }

    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logfile_.is_open()) {
            logfile_ << msg << std::endl;
            logfile_.flush();
        }
    }

    const std::string& getLogFilePath() const { return logPath_; }

private:
    DebugLogger() : logPath_(resolveLogFilePath()), logfile_(logPath_, std::ios::app) {}
    ~DebugLogger() { logfile_.close(); }
    DebugLogger(const DebugLogger&) = delete;
    DebugLogger& operator=(const DebugLogger&) = delete;

    static std::string normalizeEnvPath(const char* value) {
        if (value == nullptr)
            return {};
        std::string out(value);
        if (!out.empty() && (out.back() == '\\' || out.back() == '/'))
            out.pop_back();
        return out;
    }

    static std::string joinPath(const std::string& dir, const std::string& file) {
#ifdef _WIN32
        return dir + "\\" + file;
#else
        return dir + "/" + file;
#endif
    }

    static bool ensureParentDirectory(const std::string& path) {
        std::error_code ec;
        std::filesystem::path p(path);
        auto parent = p.parent_path();
        if (parent.empty())
            return true;
        std::filesystem::create_directories(parent, ec);
        return !ec;
    }

    std::string resolveLogFilePath() {
        std::vector<std::string> candidates;
        const char* explicitPath = std::getenv("JAMULUS_VST3_LOG_PATH");
        if (explicitPath && explicitPath[0] != '\0')
            candidates.emplace_back(explicitPath);
#ifdef _WIN32
        std::string tempDir  = normalizeEnvPath(std::getenv("TEMP"));
        std::string tmpDir   = normalizeEnvPath(std::getenv("TMP"));
        std::string localApp = normalizeEnvPath(std::getenv("LOCALAPPDATA"));
        if (!tempDir.empty())
            candidates.push_back(joinPath(tempDir, "JamulusPlus_debug.log"));
        if (!tmpDir.empty())
            candidates.push_back(joinPath(tmpDir, "JamulusPlus_debug.log"));
        if (!localApp.empty())
            candidates.push_back(joinPath(localApp, "JamulusPlus_debug.log"));

#else
        const char* tmp = std::getenv("TMPDIR");
        std::string tmpDir = normalizeEnvPath(tmp ? tmp : "/tmp");
        if (!tmpDir.empty())
            candidates.push_back(joinPath(tmpDir, "JamulusPlus_debug.log"));
#endif
        candidates.push_back("JamulusPlus_debug.log");

        for (const auto& candidate : candidates) {
            if (!ensureParentDirectory(candidate))
                continue;
            std::ofstream probe(candidate, std::ios::app);
            if (probe.is_open()) {
                probe.close();
                return candidate;
            }
        }

        return "JamulusPlus_debug.log";
    }

    std::string logPath_;
    std::ofstream logfile_;
    std::mutex mutex_;
};
