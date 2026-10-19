#pragma once

#include "../graphics/D3D11Renderer.h"
#include "../audio/WasapiAudioCapture.h"
#include "../audio/AudioAnalyzer.h"
#include "../audio/BeatDetector.h"
#include "../control/PresetManager.h"
#include "../control/AutoPilot.h"
#include "../control/MidiController.h"
#include "../control/AutoUpdater.h"
#include <string>
#include <vector>

struct GLFWwindow;

namespace Visualizer::UI {

class DjDashboard {
public:
    DjDashboard(
        Graphics::WindowManager* windowManager,
        Graphics::D3D11Renderer* renderer,
        Audio::WasapiAudioCapture* audioCapture,
        Audio::AudioAnalyzer* audioAnalyzer,
        Audio::BeatDetector* beatDetector,
        Control::PresetManager* presetManager,
        Control::AutoPilot* autoPilot,
        Control::MidiController* midiController
    );
    ~DjDashboard();

    bool initialize(GLFWwindow* window, void* device = nullptr, void* context = nullptr);
    void shutdown();

    // Render ImGui DJ Dashboard Frame
    void render(
        float deltaTime,
        Graphics::AudioVisualizerParams& params
    );

    // Apply custom Daft Punk dark neon style with dynamic scale
    void applyCustomTheme(float scale = 1.0f);
    float getFPS() const { return m_fps; }
    float getUiScale() const { return m_uiScale; }
    void setUiScale(float scale) { m_uiScale = scale; }
    bool isVsyncEnabled() const { return m_vsync; }

    void triggerSpacebarDrop();

private:
    void renderHeaderBar();
    void renderTopStatusBar(float deltaTime);
    void renderMoodButtons();
    void renderTextAndPalettePanel();
    void renderKidSafeAndLayerControls();
    void renderAudioSpectrumPanel();
    void renderProjectorOutputPanel();
    void renderLivePreview();

    Graphics::WindowManager* m_windowManager;
    Graphics::D3D11Renderer* m_renderer;
    Audio::WasapiAudioCapture* m_audioCapture;
    Audio::AudioAnalyzer* m_audioAnalyzer;
    Audio::BeatDetector* m_beatDetector;
    Control::PresetManager* m_presetManager;
    Control::AutoPilot* m_autoPilot;
    Control::MidiController* m_midiController;
    Control::AutoUpdater m_autoUpdater;

    bool m_vsync = true;
    bool m_isInitialized = false;
    int m_selectedMonitor = 1;
    int m_selectedAudioDevice = 0;
    std::vector<Graphics::MonitorInfo> m_monitors;
    std::vector<Audio::AudioDeviceInfo> m_audioDevices;
    std::vector<std::string> m_presetList;
    int m_currentPresetIndex = 0;
    char m_newPresetName[64] = "MyDadPunkVibe";

    float m_fps = 60.0f;
    float m_frameTimeMs = 16.6f;
    float m_uiScale = 1.0f;
    float m_appliedUiScale = 1.0f;
    int m_selectedScaleIndex = 0;
    float m_silentWarningTimer = 0.0f;
};

} // namespace Visualizer::UI
