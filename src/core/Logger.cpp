#include "Logger.h"
#include <windows.h>
#include <fstream>
#include <mutex>
#include <filesystem>

namespace Visualizer {

static std::mutex s_logMutex;

static std::wstring GetLogFilePath() {
    wchar_t exePathBuf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();
    return (exeDir / L"DJVisualizer.log").wstring();
}

void ClearLog() {
    std::lock_guard<std::mutex> lock(s_logMutex);
    HANDLE hFile = CreateFileW(GetLogFilePath().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}

void Log(const std::string& msg) {
    OutputDebugStringA((msg + "\n").c_str());
    std::lock_guard<std::mutex> lock(s_logMutex);
    HANDLE hFile = CreateFileW(GetLogFilePath().c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        std::string line = msg + "\r\n";
        DWORD written = 0;
        WriteFile(hFile, line.c_str(), static_cast<DWORD>(line.length()), &written, nullptr);
        CloseHandle(hFile);
    }
}

} // namespace Visualizer
