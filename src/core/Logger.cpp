#include "Logger.h"
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif
#include <fstream>
#include <mutex>
#include <filesystem>

namespace Visualizer {

static std::mutex s_logMutex;

static std::filesystem::path GetLogFilePath() {
#if defined(_WIN32)
    wchar_t exePathBuf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();
    return exeDir / "DJVisualizer.log";
#elif defined(__APPLE__)
    // On macOS, write log to /tmp/DJVisualizer.log so it's always writable even inside a read-only .dmg
    return std::filesystem::path("/tmp/DJVisualizer.log");
#else
    return std::filesystem::current_path() / "DJVisualizer.log";
#endif
}

void ClearLog() {
    std::lock_guard<std::mutex> lock(s_logMutex);
    std::ofstream f(GetLogFilePath(), std::ios::trunc);
}

void Log(const std::string& msg) {
#if defined(_WIN32)
    OutputDebugStringA((msg + "\n").c_str());
#endif
    std::lock_guard<std::mutex> lock(s_logMutex);
    std::ofstream f(GetLogFilePath(), std::ios::app);
    if (f.is_open()) {
        f << msg << "\n";
        f.flush();
    }
}

} // namespace Visualizer
