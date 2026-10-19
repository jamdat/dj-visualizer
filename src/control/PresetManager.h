#pragma once

#include "../graphics/ConstantBuffers.h"
#include <string>
#include <vector>

namespace Visualizer::Control {

struct VisualizerPreset {
    std::string name = "Default";
    std::string sceneName = "ModularMixer";
    int sceneIndex = 0;

    // Ingredient Layers & Modifiers
    int backgroundType = 0;      // 0 = Starfield & Nebula, 1 = Warp Tunnel, 2 = LED Soundwave Void, 3 = Stage Arena, 4 = Obsidian Ocean
    int shapeType = 0;           // 0 = Liquid Blobs, 1 = Alive Pyramid, 2 = Gyroscope Rings, 3 = Obsidian Monolith, 4 = Torus Knot, 5 = Hypercube, 6 = Geodesic Sphere, 7 = Crystal Cluster, 8 = None
    int duplicationMode = 0;     // 0 = Single, 1 = Orbital Ring, 2 = Triad Array, 3 = Radial Kaleidoscope, 4 = Infinite Depth
    float duplicationParam0 = 1.0f; // Spacing / Radius
    float duplicationParam1 = 0.5f; // Satellite scale
    float cameraDrift = 1.0f;    // 0.0 or 1.0 macro toggle

    // Composable 3D Environment Layer Weights & Materials
    float skyLayerWeight = 1.0f;
    float warpLayerWeight = 0.0f;
    float floorLayerWeight = 1.0f;
    float soundwaveLayerWeight = 0.5f;
    float hudLayerWeight = 0.3f;
    int materialMode = 0;
    float stageTrussWeight = 0.5f;

    float fluidityAmount = 1.0f; // 0.0 to 2.0
    float particleIntensity = 1.0f; // 0.0 to 2.0
    float laserIntensity = 1.0f; // 0.0 to 2.0
    float vortexTwist = 0.0f;    // -3.14 to 3.14

    float bloomIntensity = 1.0f;
    float glitchRate = 0.3f;
    float wireframeDensity = 1.0f;
    float strobeFlash = 0.0f;

    float customParam0 = 1.0f; // Zoom
    float customParam1 = 1.0f; // Speed
    float customParam2 = 0.5f; // Density
    float customParam3 = 1.0f; // Glow
    float customParam4 = 0.0f; // Rotation
    float customParam5 = 1.0f; // Sensitivity
    float customParam6 = 0.0f; // Spread
    float customParam7 = 0.0f; // Glitch Trigger

    float colorPrimary[4]   = { 1.0f, 0.2f, 0.0f, 1.0f }; // Daft Orange
    float colorSecondary[4] = { 0.0f, 0.8f, 1.0f, 1.0f }; // Cyan
    float colorAccent[4]    = { 1.0f, 0.9f, 0.1f, 1.0f }; // Gold

    float audioGain = 1.8f;
    bool autoPilot = false;

    // Convert to Constant Buffer params
    void applyToParams(Graphics::AudioVisualizerParams& params) const;
    void captureFromParams(const Graphics::AudioVisualizerParams& params, const std::string& scene, float gain, bool autoP);
};

class PresetManager {
public:
    explicit PresetManager(const std::string& presetsDirectory = "presets");

    // Initialize preset directory and generate defaults if empty
    void initialize();

    // List all available preset names
    std::vector<std::string> getPresetNames();

    // Save and load
    bool savePreset(const VisualizerPreset& preset);
    bool loadPreset(const std::string& presetName, VisualizerPreset& outPreset);

    // Built-in preset generators
    static VisualizerPreset createDaftAlivePreset();
    static VisualizerPreset createLiquidCyberPreset();
    static VisualizerPreset createSynthwaveHorizonPreset();
    static VisualizerPreset createGyroLaserArenaPreset();
    static VisualizerPreset createAcidPlasmaPreset();
    static VisualizerPreset createRandomDropRecipe();

private:
    std::string m_presetsDir;
};

} // namespace Visualizer::Control

