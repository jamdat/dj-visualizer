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

    // Trigger spacebar drop from window loop if desired
    void triggerSpacebarDrop(Graphics::AudioVisualizerParams& params);

private:
    void renderHeaderBar();
    void renderNextDropQueueBar(Graphics::AudioVisualizerParams& params);
    void renderHeroShapeIngredient(Graphics::AudioVisualizerParams& params);
    void renderBackgroundIngredient(Graphics::AudioVisualizerParams& params);
    void renderStageFxIngredients(Graphics::AudioVisualizerParams& params);
    void renderFluidityMotionPanel(Graphics::AudioVisualizerParams& params);
    void renderVibeRecipesAndColors(Graphics::AudioVisualizerParams& params);
    void renderAudioSpectrumPanel();
    void renderProjectorOutputPanel();
    void renderShaderCompilerStatus();
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
    char m_newPresetName[64] = "MyCustomRecipe";

    float m_fps = 60.0f;
    float m_frameTimeMs = 16.6f;
    float m_uiScale = 1.0f;
    float m_appliedUiScale = 1.0f;
    int m_selectedScaleIndex = 0; // Default to 100% / native scale
    std::string m_lastCompilerError;
    std::string m_lastReloadedShader;
    float m_reloadBannerTimer = 0.0f;
};

} // namespace Visualizer::UI
