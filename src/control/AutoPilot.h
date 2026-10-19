#pragma once

#include "../graphics/ConstantBuffers.h"
#include "../audio/BeatDetector.h"
#include <string>
#include <vector>
#include <array>
#include <cstring>

namespace Visualizer::Control {

struct UserSettings {
    float masterBrightness = 0.85f;    // Kid-safe 85% default cap
    char customText[17] = "DAD PUNK";  // 16 chars + null terminator
    int paletteIndex = 0;              // 0: Daft Red/Gold, 1: TRON Cyan/Orange, 2: Halloween, 3: Chrome, 4: Rainbow
    bool halloweenMode = false;        // Spooky FX override

    // Manual scene selection (-1 = Auto, 0..5 = Scene 1..6)
    int manualScene = -1;
    float manualHoldTimer = 0.0f;      // 30s countdown

    // Fine-tune layer weight multipliers
    float pyramidMultiplier = 1.0f;
    float logoMultiplier = 1.0f;
    float laserMultiplier = 1.0f;
    float floorMultiplier = 1.0f;
    float skyMultiplier = 1.0f;
    float particleMultiplier = 1.0f;

    int pyramidPattern = 0;            // 0=VU Strips, 1=Chevrons, 2=Sweep, 3=Solid, 4=Rainbow
    int logoStyle = 0;                 // 0=LED Dot-Matrix, 1=Chrome Bevel
};

struct SceneDefinition {
    std::string name;
    std::string icon;
    float pyramidWeight = 0.0f;
    float logoWeight = 0.0f;
    float spookyWeight = 0.0f;
    float trailsWeight = 0.0f;
    float laserIntensity = 0.0f;
    float skyLayerWeight = 0.0f;
    float floorLayerWeight = 0.0f;
    float particleIntensity = 0.0f;
    float ledWallWeight = 0.0f;
    float horizonWeight = 0.0f;
    int defaultPattern = 0;
    int defaultPalette = 0;
};

struct DirectorState {
    int currentScene = 0;
    int targetScene = 0;
    float transitionProgress = 1.0f;   // 1.0 = fully transitioned
    float timeInCurrentScene = 0.0f;
    float phraseInterval = 30.0f;      // 30s auto switch
    float silenceTimer = 0.0f;
    float smoothedEnergy = 0.0f;
    bool isAutoPilotEnabled = true;
};

struct AudioFrameData {
    float subBass = 0.0f;
    float bass = 0.0f;
    float lowMid = 0.0f;
    float highMid = 0.0f;
    float treble = 0.0f;
    float beatPulse = 0.0f;
    float dropIntensity = 0.0f;
    float energyAccumulator = 0.0f;
    float bpm = 128.0f;
    float rms = 0.0f;
    bool isSilent = false;
};

class AutoPilot {
public:
    AutoPilot();

    void setEnabled(bool enabled) { m_director.isAutoPilotEnabled = enabled; }
    bool isEnabled() const { return m_director.isAutoPilotEnabled; }

    void selectScene(int sceneIndex, float holdSeconds = 30.0f);
    void triggerDropTransition();

    void update(
        float deltaTime,
        const Audio::BeatDetector& beatDetector,
        const AudioFrameData& audioData
    );

    // Pure resolution function: Combines UserSettings + DirectorState + Audio -> GpuParams
    void resolveGpuParams(
        const AudioFrameData& audioData,
        Graphics::AudioVisualizerParams& outParams
    );

    UserSettings& getUserSettings() { return m_user; }
    const UserSettings& getUserSettings() const { return m_user; }

    const DirectorState& getDirectorState() const { return m_director; }
    const std::vector<SceneDefinition>& getScenes() const { return m_scenes; }

    // Helpers
    int getCurrentSceneIndex() const { return m_director.currentScene; }
    const std::string& getCurrentSceneName() const { return m_scenes[m_director.currentScene].name; }
    float getTimeRemainingInScene() const;
    bool isManualHold() const { return m_user.manualScene >= 0 && m_user.manualHoldTimer > 0.0f; }
    float getManualHoldTimeRemaining() const { return m_user.manualHoldTimer; }
    void cancelManualHold() { m_user.manualScene = -1; m_user.manualHoldTimer = 0.0f; }

private:
    void initScenes();

    UserSettings m_user;
    DirectorState m_director;
    std::vector<SceneDefinition> m_scenes;

    // Photosafety state
    float m_lastLuminance = 0.0f;
    float m_flashRateCounter = 0.0f;
    float m_safetyDamping = 1.0f;
};

} // namespace Visualizer::Control
