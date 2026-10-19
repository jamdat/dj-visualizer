#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>

#ifndef DJVISUALIZER_VERSION
#define DJVISUALIZER_VERSION "v1.0.1"
#endif

#ifndef DJVISUALIZER_GITHUB_REPO
#define DJVISUALIZER_GITHUB_REPO "jamdat/dj-visualizer"
#endif

namespace Visualizer::Control {

enum class UpdateState {
    Idle,
    Checking,
    UpToDate,
    UpdateAvailable,
    DownloadingAndInstalling,
    ReadyToRestart,
    Error
};

class AutoUpdater {
public:
    AutoUpdater();
    ~AutoUpdater();

    // Asynchronously check GitHub Releases for a newer version
    void checkForUpdatesAsync();

    // Asynchronously download, verify, stage, and apply the update
    void downloadAndInstallUpdateAsync();

    UpdateState getState() const { return m_state.load(); }
    std::string getCurrentVersion() const { return m_currentVersion; }
    std::string getLatestVersion() const;
    std::string getStatusMessage() const;
    bool isDevBuild() const { return m_isDevBuild; }

private:
    std::string m_currentVersion = DJVISUALIZER_VERSION;
    std::string m_repoSlug = DJVISUALIZER_GITHUB_REPO;
    bool m_isDevBuild = false;

    std::atomic<UpdateState> m_state{UpdateState::Idle};
    mutable std::mutex m_mutex;
    std::string m_latestVersion;
    std::string m_downloadUrl;
    std::string m_statusMessage;

    std::thread m_workerThread;
};

} // namespace Visualizer::Control
