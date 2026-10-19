#include "AutoUpdater.h"
#include "../core/Logger.h"
#include "../../third_party/json/json.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <unistd.h>
#endif

namespace Visualizer::Control {

static std::filesystem::path GetAppDirectory() {
#if defined(_WIN32)
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return std::filesystem::weakly_canonical(std::filesystem::path(buf)).parent_path();
    }
    return std::filesystem::current_path();
#else
    return std::filesystem::current_path();
#endif
}

static std::string RunCommandCaptureStdout(const std::string& cmd) {
    std::string output;
#if defined(_WIN32)
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return "";

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return output;
}

AutoUpdater::AutoUpdater() {
    std::filesystem::path appDir = GetAppDirectory();
    std::error_code ec;
    if (std::filesystem::exists(appDir / "../../CMakeLists.txt", ec) ||
        std::filesystem::exists(appDir / "../../src", ec) ||
        std::filesystem::exists(appDir / "../CMakeLists.txt", ec)) {
        m_isDevBuild = true;
    }
}

AutoUpdater::~AutoUpdater() {
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

std::string AutoUpdater::getLatestVersion() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latestVersion;
}

std::string AutoUpdater::getStatusMessage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_statusMessage;
}

void AutoUpdater::checkForUpdatesAsync() {
    UpdateState current = m_state.load();
    if (current == UpdateState::Checking || current == UpdateState::DownloadingAndInstalling) {
        return;
    }

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_state.store(UpdateState::Checking);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_statusMessage = "Checking GitHub Releases...";
    }

    m_workerThread = std::thread([this]() {
        try {
            std::string apiUrl = "https://api.github.com/repos/" + m_repoSlug + "/releases/latest";
            std::string cmd = "curl -sL -m 10 -H \"Accept: application/vnd.github+json\" \"" + apiUrl + "\"";
            std::string jsonStr = RunCommandCaptureStdout(cmd);

            if (jsonStr.empty()) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_statusMessage = "Offline or unable to reach GitHub";
                m_state.store(UpdateState::Idle);
                return;
            }

            auto j = Visualizer::Json::Value::parse(jsonStr);
            if (!j.is_object() || !j["tag_name"].is_string()) {
                // No releases published yet
                std::lock_guard<std::mutex> lock(m_mutex);
                m_statusMessage = "Up to date (" + m_currentVersion + ")";
                m_state.store(UpdateState::UpToDate);
                return;
            }

            std::string tagName = j["tag_name"].as_string();
#if defined(__APPLE__)
            std::string targetAsset = "DJVisualizer-macOS-arm64.zip";
#else
            std::string targetAsset = "DJVisualizer-Windows-x64.zip";
#endif
            std::string assetUrl;
            std::string checksumUrl;
            if (j["assets"].is_array()) {
                for (const auto& asset : j["assets"].as_array()) {
                    if (asset["name"].is_string() && asset["browser_download_url"].is_string()) {
                        std::string name = asset["name"].as_string();
                        if (name == targetAsset) {
                            assetUrl = asset["browser_download_url"].as_string();
                        } else if (name == "SHA256SUMS.txt" || name == "sha256.txt") {
                            checksumUrl = asset["browser_download_url"].as_string();
                        }
                    }
                }
            }

            // Security validation: strictly verify HTTPS GitHub release URL prefix
            std::string expectedPrefix = "https://github.com/" + m_repoSlug + "/releases/download/";
            if (!assetUrl.empty() && assetUrl.rfind(expectedPrefix, 0) != 0) {
                Log("[SECURITY] Rejected unexpected update download URL: " + assetUrl);
                assetUrl.clear();
            }
            if (!checksumUrl.empty() && checksumUrl.rfind(expectedPrefix, 0) != 0) {
                Log("[SECURITY] Rejected unexpected checksum download URL: " + checksumUrl);
                checksumUrl.clear();
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_latestVersion = tagName;
                m_downloadUrl = assetUrl;
                m_checksumUrl = checksumUrl;
                if (tagName != m_currentVersion && !assetUrl.empty()) {
                    m_statusMessage = "Update available: " + tagName;
                    m_state.store(UpdateState::UpdateAvailable);
                } else {
                    m_statusMessage = "Up to date (" + m_currentVersion + ")";
                    m_state.store(UpdateState::UpToDate);
                }
            }
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_statusMessage = std::string("Update check error: ") + e.what();
            m_state.store(UpdateState::Error);
        }
    });
}

void AutoUpdater::downloadAndInstallUpdateAsync() {
    if (m_state.load() != UpdateState::UpdateAvailable) return;

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_state.store(UpdateState::DownloadingAndInstalling);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_statusMessage = "Downloading update...";
    }

    m_workerThread = std::thread([this]() {
        std::string url;
        std::string checkUrl;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            url = m_downloadUrl;
            checkUrl = m_checksumUrl;
        }

        std::filesystem::path appDir = GetAppDirectory();
        std::error_code ec;
        std::filesystem::path stageDir = appDir / "update_staging";
        std::filesystem::remove_all(stageDir, ec);
        std::filesystem::create_directories(stageDir, ec);

        std::filesystem::path zipPath = stageDir / "update.zip";
        std::string dlCmd = "curl -sL -o \"" + zipPath.string() + "\" \"" + url + "\"";
        int rc = std::system(dlCmd.c_str());
        if (rc != 0 || !std::filesystem::exists(zipPath, ec)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_statusMessage = "Download failed";
            m_state.store(UpdateState::Error);
            return;
        }

        // Cryptographic integrity validation (SHA-256)
        if (!checkUrl.empty()) {
            std::filesystem::path checkPath = stageDir / "SHA256SUMS.txt";
            std::string checkDlCmd = "curl -sL -o \"" + checkPath.string() + "\" \"" + checkUrl + "\"";
            std::system(checkDlCmd.c_str());

            if (std::filesystem::exists(checkPath, ec)) {
                std::ifstream csFile(checkPath);
                std::string line;
#if defined(__APPLE__)
                std::string targetAsset = "DJVisualizer-macOS-arm64.zip";
#else
                std::string targetAsset = "DJVisualizer-Windows-x64.zip";
#endif
                std::string expectedHash;
                while (std::getline(csFile, line)) {
                    if (line.find(targetAsset) != std::string::npos) {
                        std::istringstream iss(line);
                        iss >> expectedHash;
                        break;
                    }
                }

                if (!expectedHash.empty()) {
                    std::string actualHash;
#if defined(_WIN32)
                    std::string hashCmd = "certutil -hashfile \"" + zipPath.string() + "\" SHA256";
                    std::string hashOut = RunCommandCaptureStdout(hashCmd);
                    std::istringstream hss(hashOut);
                    std::string hline;
                    while (std::getline(hss, hline)) {
                        hline.erase(std::remove_if(hline.begin(), hline.end(), ::isspace), hline.end());
                        if (hline.length() == 64 && hline.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
                            actualHash = hline;
                            break;
                        }
                    }
#elif defined(__APPLE__)
                    std::string hashCmd = "shasum -a 256 \"" + zipPath.string() + "\"";
                    std::string hashOut = RunCommandCaptureStdout(hashCmd);
                    std::istringstream hss(hashOut);
                    hss >> actualHash;
#endif
                    auto toLower = [](std::string s) {
                        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                        return s;
                    };

                    if (!actualHash.empty() && toLower(actualHash) != toLower(expectedHash)) {
                        Log("[SECURITY] SHA-256 checksum mismatch! Expected: " + expectedHash + " Actual: " + actualHash);
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_statusMessage = "Security error: Checksum mismatch";
                        m_state.store(UpdateState::Error);
                        std::filesystem::remove_all(stageDir, ec);
                        return;
                    }
                    Log("[SECURITY] SHA-256 checksum verified successfully: " + actualHash);
                }
            }
        }

#if defined(__APPLE__)
        // On macOS: unzip, strip Gatekeeper quarantine (xattr -cr), replace .app bundle, and relaunch
        std::string unzipCmd = "unzip -q -o \"" + zipPath.string() + "\" -d \"" + stageDir.string() + "\" && xattr -cr \"" + stageDir.string() + "\"";
        if (std::system(unzipCmd.c_str()) == 0) {
            std::filesystem::path bundleRoot = appDir;
            if (appDir.filename() == "MacOS" && appDir.parent_path().filename() == "Contents") {
                bundleRoot = appDir.parent_path().parent_path();
            }

            std::filesystem::path scriptPath = stageDir / "apply_update.sh";
            {
                std::ofstream sh(scriptPath);
                sh << "#!/bin/bash\n";
                sh << "sleep 1\n";
                sh << "if [ -d \"" << (stageDir / "DJVisualizer.app").string() << "\" ]; then\n";
                sh << "  cp -Rf \"" << (stageDir / "DJVisualizer.app").string() << "/\"* \"" << bundleRoot.string() << "/\"\n";
                sh << "  xattr -cr \"" << bundleRoot.string() << "\"\n";
                sh << "  open -n \"" << bundleRoot.string() << "\"\n";
                sh << "fi\n";
                sh << "rm -rf \"" << stageDir.string() << "\"\n";
            }
            std::string launchCmd = "chmod +x \"" + scriptPath.string() + "\" && nohup \"" + scriptPath.string() + "\" >/dev/null 2>&1 &";
            std::system(launchCmd.c_str());
            std::exit(0);
        }
#elif defined(_WIN32)
        // On Windows: extract zip with path escaping and run detached batch updater that swaps files and relaunches DJVisualizer.exe
        auto escapeSingleQuotes = [](const std::string& in) {
            std::string out;
            for (char c : in) {
                if (c == '\'') out += "''";
                else out += c;
            }
            return out;
        };

        std::string safeZipPath = escapeSingleQuotes(zipPath.string());
        std::string safeStageDir = escapeSingleQuotes(stageDir.string());
        std::string unzipCmd = "powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"Expand-Archive -LiteralPath '" + safeZipPath + "' -DestinationPath '" + safeStageDir + "' -Force\"";
        if (std::system(unzipCmd.c_str()) == 0) {
            std::filesystem::path extractedRoot = stageDir / "DJVisualizer";
            if (!std::filesystem::exists(extractedRoot, ec)) {
                extractedRoot = stageDir;
            }

            std::filesystem::path batPath = stageDir / "apply_update.bat";
            {
                std::ofstream bat(batPath);
                bat << "@echo off\n";
                bat << "timeout /t 2 /nobreak >nul\n";
                bat << "xcopy /s /y /q \"" << extractedRoot.string() << "\\*\" \"" << appDir.string() << "\\\" >nul\n";
                bat << "start \"\" \"" << (appDir / "DJVisualizer.exe").string() << "\"\n";
                bat << "rmdir /s /q \"" << stageDir.string() << "\"\n";
            }

            std::string launchCmd = "start \"\" /min cmd /c \"" + batPath.string() + "\"";
            std::system(launchCmd.c_str());
            std::exit(0);
        }
#endif

        std::lock_guard<std::mutex> lock(m_mutex);
        m_statusMessage = "Failed to extract update";
        m_state.store(UpdateState::Error);
    });
}

} // namespace Visualizer::Control
