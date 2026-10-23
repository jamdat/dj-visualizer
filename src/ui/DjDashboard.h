#pragma once

#include "../graphics/D3D11Renderer.h"
#include "../audio/WasapiAudioCapture.h"
#include "../audio/AudioAnalyzer.h"
#include "../audio/BeatDetector.h"
#include "../control/PresetManager.h"
#include "../control/AutoPilot.h"
#include "../control/MidiController.h"
#include "../control/AutoUpdater.h"
#include "../control/SpaceVoyage.h"
#include <string>
#include <vector>

struct GLFWwindow;

namespace Visualizer::UI {

// Which scene shader the stage renders: ModularMixer (Default) or a standalone layout
enum class StageLayout { Default, SpaceVoyage, Chemistry, Quantum, DaftPunk, DanceParty };

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
        Control::MidiController* midiController,
        Control::SpaceVoyage* spaceVoyage
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
    int getTargetFps() const { return m_targetFps; }
    void setTargetFps(int fps) { m_targetFps = fps; }
    StageLayout getStageLayout() const { return m_stageLayout; }

    // Loop Visuals: call every frame (the dashboard isn't rendered in fullscreen); the fade
    // (0..1) dips the stage to black around each automatic layout switch
    void updateStageLoop(float deltaTime);
    float getStageFade() const;

    void triggerSpacebarDrop();

private:
    void setStageLayout(StageLayout layout);
    void renderHeaderBar();
    void renderTopStatusBar(float deltaTime);
    void renderVisualMixerPanel();
    void renderConsolidatedDetailsPanel();
    void renderAudioSpectrumPanel();
    void renderProjectorOutputPanel();
    void openStageWindowOn(int monitorIndex);
    void renderLivePreview();

    Graphics::WindowManager* m_windowManager;
    Graphics::D3D11Renderer* m_renderer;
    Audio::WasapiAudioCapture* m_audioCapture;
    Audio::AudioAnalyzer* m_audioAnalyzer;
    Audio::BeatDetector* m_beatDetector;
    Control::PresetManager* m_presetManager;
    Control::AutoPilot* m_autoPilot;
    Control::MidiController* m_midiController;
    Control::SpaceVoyage* m_spaceVoyage;
    Control::AutoUpdater m_autoUpdater;

    bool m_vsync = true;
    bool m_isInitialized = false;
    int m_selectedMonitor = 1;
    int m_selectedAudioDevice = 0;
    std::vector<Graphics::MonitorInfo> m_monitors;
    float m_monitorRefreshTimer = 0.0f;
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
    bool m_showUpdateConfirmModal = false;
    int m_targetFps = 144;
    int m_fpsLimitIndex = 3;
    StageLayout m_stageLayout = StageLayout::Default;

    static constexpr float kLoopFadeSeconds = 0.6f;
    bool m_loopVisuals = false;
    float m_loopIntervalSec = 60.0f;   // time on each visual
    float m_loopTimer = 0.0f;
    float m_loopFadeIn = kLoopFadeSeconds;
    int m_selectedIngredient = 0; // 0=HeroText, 1=Vignette, 2=Triangle, 3=Lasers, 4=Particles, 5=Grid, 6=Sun, 7=Palettes
};

} // namespace Visualizer::UI
