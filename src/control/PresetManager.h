#pragma once

#include "../graphics/ConstantBuffers.h"
#include <string>
#include <vector>

namespace Visualizer::Control {

struct VisualizerPreset {
    std::string name = "Default";
    std::string sceneName = "ModularMixer";
    int sceneIndex = 0;

    // Dad Punk layer weights & settings
    float pyramidMultiplier = 1.0f;
    float logoMultiplier = 1.0f;
    float laserMultiplier = 1.0f;
    float floorMultiplier = 1.0f;
    float skyMultiplier = 1.0f;
    float particleMultiplier = 1.0f;

    int pyramidPattern = 0;
    int logoStyle = 0;
    int paletteIndex = 0;
    float masterBrightness = 0.85f;
    std::string customText = "DADT PUNK";

    float audioGain = 1.8f;
    bool autoPilot = true;
};

class PresetManager {
public:
    explicit PresetManager(const std::string& presetsDirectory = "presets");

    void initialize();
    std::vector<std::string> getPresetNames();
    bool savePreset(const VisualizerPreset& preset);
    bool loadPreset(const std::string& presetName, VisualizerPreset& outPreset);

    // 6 Curated Scene Presets
    static VisualizerPreset createDaftAlivePreset();
    static VisualizerPreset createTronGridPreset();
    static VisualizerPreset createSpookyPunkPreset();
    static VisualizerPreset createLedWallPreset();
    static VisualizerPreset createDadPunkHeroPreset();
    static VisualizerPreset createChillAttractPreset();

private:
    std::string m_presetsDir;
};

} // namespace Visualizer::Control
