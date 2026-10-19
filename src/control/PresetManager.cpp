#include "PresetManager.h"
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <cstdlib>
#endif
#include "../../third_party/json/json.hpp"
#include "../core/Logger.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace Visualizer::Control {

static std::filesystem::path GetBaseAppDir() {
#if defined(_WIN32)
    wchar_t exePathBuf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    return std::filesystem::path(exePathBuf).parent_path();
#elif defined(__APPLE__)
    char pathBuf[4096];
    uint32_t size = sizeof(pathBuf);
    if (_NSGetExecutablePath(pathBuf, &size) == 0) {
        std::filesystem::path exeDir = std::filesystem::weakly_canonical(std::filesystem::path(pathBuf)).parent_path();
        std::error_code ec;
        if (std::filesystem::exists(exeDir / "../Resources/presets", ec)) {
            return std::filesystem::weakly_canonical(exeDir / "../Resources", ec);
        }
        return exeDir;
    }
    return std::filesystem::current_path();
#else
    return std::filesystem::current_path();
#endif
}

void VisualizerPreset::applyToParams(Graphics::AudioVisualizerParams& params) const {
    params.BackgroundType = backgroundType;
    params.ShapeType = shapeType;
    params.DuplicationMode = duplicationMode;
    params.DuplicationParam0 = duplicationParam0;
    params.DuplicationParam1 = duplicationParam1;
    params.CameraDrift = cameraDrift;

    params.FluidityAmount = fluidityAmount;
    params.ParticleIntensity = particleIntensity;
    params.LaserIntensity = laserIntensity;
    params.VortexTwist = vortexTwist;

    params.SkyLayerWeight = skyLayerWeight;
    params.WarpLayerWeight = warpLayerWeight;
    params.FloorLayerWeight = floorLayerWeight;
    params.SoundwaveLayerWeight = soundwaveLayerWeight;
    params.HudLayerWeight = hudLayerWeight;
    params.MaterialMode = materialMode;
    params.StageTrussWeight = stageTrussWeight;

    params.BloomIntensity = bloomIntensity;
    params.GlitchRate = glitchRate;
    params.WireframeDensity = wireframeDensity;
    params.StrobeFlash = strobeFlash;

    params.CustomParam0 = customParam0;
    params.CustomParam1 = customParam1;
    params.CustomParam2 = customParam2;
    params.CustomParam3 = customParam3;
    params.CustomParam4 = customParam4;
    params.CustomParam5 = customParam5;
    params.CustomParam6 = customParam6;
    params.CustomParam7 = customParam7;

    params.ColorPrimary   = { colorPrimary[0], colorPrimary[1], colorPrimary[2], colorPrimary[3] };
    params.ColorSecondary = { colorSecondary[0], colorSecondary[1], colorSecondary[2], colorSecondary[3] };
    params.ColorAccent    = { colorAccent[0], colorAccent[1], colorAccent[2], colorAccent[3] };
}

void VisualizerPreset::captureFromParams(const Graphics::AudioVisualizerParams& params, const std::string& scene, float gain, bool autoP) {
    sceneName = scene;
    sceneIndex = 0;
    backgroundType = params.BackgroundType;

    shapeType = params.ShapeType;
    duplicationMode = params.DuplicationMode;
    duplicationParam0 = params.DuplicationParam0;
    duplicationParam1 = params.DuplicationParam1;
    cameraDrift = params.CameraDrift;

    fluidityAmount = params.FluidityAmount;
    particleIntensity = params.ParticleIntensity;
    laserIntensity = params.LaserIntensity;
    vortexTwist = params.VortexTwist;

    skyLayerWeight = params.SkyLayerWeight;
    warpLayerWeight = params.WarpLayerWeight;
    floorLayerWeight = params.FloorLayerWeight;
    soundwaveLayerWeight = params.SoundwaveLayerWeight;
    hudLayerWeight = params.HudLayerWeight;
    materialMode = params.MaterialMode;
    stageTrussWeight = params.StageTrussWeight;

    bloomIntensity = params.BloomIntensity;
    glitchRate = params.GlitchRate;
    wireframeDensity = params.WireframeDensity;
    strobeFlash = params.StrobeFlash;

    customParam0 = params.CustomParam0;
    customParam1 = params.CustomParam1;
    customParam2 = params.CustomParam2;
    customParam3 = params.CustomParam3;
    customParam4 = params.CustomParam4;
    customParam5 = params.CustomParam5;
    customParam6 = params.CustomParam6;
    customParam7 = params.CustomParam7;

    colorPrimary[0] = params.ColorPrimary.x;
    colorPrimary[1] = params.ColorPrimary.y;
    colorPrimary[2] = params.ColorPrimary.z;
    colorPrimary[3] = params.ColorPrimary.w;

    colorSecondary[0] = params.ColorSecondary.x;
    colorSecondary[1] = params.ColorSecondary.y;
    colorSecondary[2] = params.ColorSecondary.z;
    colorSecondary[3] = params.ColorSecondary.w;

    colorAccent[0] = params.ColorAccent.x;
    colorAccent[1] = params.ColorAccent.y;
    colorAccent[2] = params.ColorAccent.z;
    colorAccent[3] = params.ColorAccent.w;

    audioGain = gain;
    autoPilot = autoP;
}

PresetManager::PresetManager(const std::string& presetsDirectory)
    : m_presetsDir(presetsDirectory) {
}

void PresetManager::initialize() {
    Log("PresetManager::initialize started for dir: " + m_presetsDir);
    std::error_code ec;
    if (!std::filesystem::exists(m_presetsDir, ec)) {
        m_presetsDir = (GetBaseAppDir() / m_presetsDir).string();
    }
    Log("Resolved presets directory to: " + m_presetsDir);
    std::filesystem::create_directories(m_presetsDir, ec);

    auto existing = getPresetNames();
    Log("Existing presets count: " + std::to_string(existing.size()));
    if (existing.empty()) {
        savePreset(createDaftAlivePreset());
        savePreset(createLiquidCyberPreset());
        savePreset(createSynthwaveHorizonPreset());
        savePreset(createGyroLaserArenaPreset());
        savePreset(createAcidPlasmaPreset());
        Log("Default presets saved successfully");
    }
}

std::vector<std::string> PresetManager::getPresetNames() {
    std::vector<std::string> names;
    std::error_code ec;
    std::filesystem::path pDir(m_presetsDir);
    if (!std::filesystem::exists(pDir, ec)) {
        pDir = GetBaseAppDir() / m_presetsDir;
    }

    if (std::filesystem::exists(pDir, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(pDir, ec)) {
            if (!ec && entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                names.push_back(entry.path().stem().string());
            }
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool PresetManager::savePreset(const VisualizerPreset& preset) {
    std::error_code ec;
    if (!std::filesystem::exists(m_presetsDir, ec)) {
        std::filesystem::create_directories(m_presetsDir, ec);
    }

    Json::Value root(Json::Value::Object{});
    root["name"] = preset.name;
    root["sceneName"] = preset.sceneName;
    root["sceneIndex"] = preset.sceneIndex;

    root["backgroundType"] = preset.backgroundType;
    root["shapeType"] = preset.shapeType;
    root["duplicationMode"] = preset.duplicationMode;
    root["duplicationParam0"] = preset.duplicationParam0;
    root["duplicationParam1"] = preset.duplicationParam1;
    root["cameraDrift"] = preset.cameraDrift;

    root["fluidityAmount"] = preset.fluidityAmount;
    root["particleIntensity"] = preset.particleIntensity;
    root["laserIntensity"] = preset.laserIntensity;
    root["vortexTwist"] = preset.vortexTwist;

    root["skyLayerWeight"] = preset.skyLayerWeight;
    root["warpLayerWeight"] = preset.warpLayerWeight;
    root["floorLayerWeight"] = preset.floorLayerWeight;
    root["soundwaveLayerWeight"] = preset.soundwaveLayerWeight;
    root["hudLayerWeight"] = preset.hudLayerWeight;
    root["materialMode"] = preset.materialMode;
    root["stageTrussWeight"] = preset.stageTrussWeight;

    root["bloomIntensity"] = preset.bloomIntensity;
    root["glitchRate"] = preset.glitchRate;
    root["wireframeDensity"] = preset.wireframeDensity;
    root["strobeFlash"] = preset.strobeFlash;

    root["customParam0"] = preset.customParam0;
    root["customParam1"] = preset.customParam1;
    root["customParam2"] = preset.customParam2;
    root["customParam3"] = preset.customParam3;
    root["customParam4"] = preset.customParam4;
    root["customParam5"] = preset.customParam5;
    root["customParam6"] = preset.customParam6;
    root["customParam7"] = preset.customParam7;

    Json::Value::Array colPrim = { preset.colorPrimary[0], preset.colorPrimary[1], preset.colorPrimary[2], preset.colorPrimary[3] };
    Json::Value::Array colSec  = { preset.colorSecondary[0], preset.colorSecondary[1], preset.colorSecondary[2], preset.colorSecondary[3] };
    Json::Value::Array colAcc  = { preset.colorAccent[0], preset.colorAccent[1], preset.colorAccent[2], preset.colorAccent[3] };

    root["colorPrimary"] = colPrim;
    root["colorSecondary"] = colSec;
    root["colorAccent"] = colAcc;

    root["audioGain"] = preset.audioGain;
    root["autoPilot"] = preset.autoPilot;

    std::string filePath = m_presetsDir + "/" + preset.name + ".json";
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << root.dump(2);
    return true;
}

bool PresetManager::loadPreset(const std::string& presetName, VisualizerPreset& outPreset) {
    std::string filePath = m_presetsDir + "/" + presetName + ".json";
    if (!std::filesystem::exists(filePath)) return false;

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    Json::Value root = Json::Value::parse(content);

    outPreset.name = root["name"].as_string(presetName);
    outPreset.sceneName = root["sceneName"].as_string("ModularMixer");
    outPreset.sceneIndex = root["sceneIndex"].as_int(0);

    outPreset.backgroundType = root["backgroundType"].as_int(0);
    outPreset.shapeType = root["shapeType"].as_int(0);
    outPreset.duplicationMode = root["duplicationMode"].as_int(0);
    outPreset.duplicationParam0 = root["duplicationParam0"].as_float(1.0f);
    outPreset.duplicationParam1 = root["duplicationParam1"].as_float(0.5f);
    outPreset.cameraDrift = root["cameraDrift"].as_float(1.0f);

    outPreset.fluidityAmount = root["fluidityAmount"].as_float(1.0f);
    outPreset.particleIntensity = root["particleIntensity"].as_float(1.0f);
    outPreset.laserIntensity = root["laserIntensity"].as_float(1.0f);
    outPreset.vortexTwist = root["vortexTwist"].as_float(0.0f);

    if (root.contains("floorLayerWeight")) {
        outPreset.skyLayerWeight = root["skyLayerWeight"].as_float(1.0f);
        outPreset.warpLayerWeight = root["warpLayerWeight"].as_float(0.0f);
        outPreset.floorLayerWeight = root["floorLayerWeight"].as_float(1.0f);
        outPreset.soundwaveLayerWeight = root["soundwaveLayerWeight"].as_float(0.5f);
        outPreset.hudLayerWeight = root["hudLayerWeight"].as_float(0.3f);
        outPreset.materialMode = root["materialMode"].as_int(0);
        outPreset.stageTrussWeight = root["stageTrussWeight"].as_float(0.5f);
    } else {
        // Smart defaults derived from legacy outPreset.backgroundType to upgrade existing presets
        switch (outPreset.backgroundType) {
            case 0: // Starfield & Nebula
                outPreset.skyLayerWeight = 1.2f;
                outPreset.warpLayerWeight = 0.0f;
                outPreset.floorLayerWeight = 0.6f;
                outPreset.soundwaveLayerWeight = 0.4f;
                outPreset.hudLayerWeight = 0.3f;
                outPreset.materialMode = 0;
                outPreset.stageTrussWeight = 0.0f;
                break;
            case 1: // Warp Tunnel
                outPreset.skyLayerWeight = 0.4f;
                outPreset.warpLayerWeight = 1.2f;
                outPreset.floorLayerWeight = 0.0f;
                outPreset.soundwaveLayerWeight = 0.4f;
                outPreset.hudLayerWeight = 0.5f;
                outPreset.materialMode = 0;
                outPreset.stageTrussWeight = 0.0f;
                break;
            case 2: // LED Soundwave Void
                outPreset.skyLayerWeight = 0.4f;
                outPreset.warpLayerWeight = 0.0f;
                outPreset.floorLayerWeight = 1.0f;
                outPreset.soundwaveLayerWeight = 1.2f;
                outPreset.hudLayerWeight = 0.4f;
                outPreset.materialMode = 0;
                outPreset.stageTrussWeight = 0.3f;
                break;
            case 3: // Stage Arena
                outPreset.skyLayerWeight = 0.3f;
                outPreset.warpLayerWeight = 0.0f;
                outPreset.floorLayerWeight = 1.2f;
                outPreset.soundwaveLayerWeight = 0.8f;
                outPreset.hudLayerWeight = 0.2f;
                outPreset.materialMode = 1;
                outPreset.stageTrussWeight = 1.3f;
                break;
            case 4: // Obsidian Ocean
                outPreset.skyLayerWeight = 1.0f;
                outPreset.warpLayerWeight = 0.0f;
                outPreset.floorLayerWeight = 1.3f;
                outPreset.soundwaveLayerWeight = 0.3f;
                outPreset.hudLayerWeight = 0.3f;
                outPreset.materialMode = 0;
                outPreset.stageTrussWeight = 0.0f;
                break;
            default:
                outPreset.skyLayerWeight = 1.0f;
                outPreset.warpLayerWeight = 0.0f;
                outPreset.floorLayerWeight = 1.0f;
                outPreset.soundwaveLayerWeight = 0.5f;
                outPreset.hudLayerWeight = 0.3f;
                outPreset.materialMode = 0;
                outPreset.stageTrussWeight = 0.5f;
                break;
        }
    }

    outPreset.bloomIntensity = root["bloomIntensity"].as_float(1.0f);
    outPreset.glitchRate = root["glitchRate"].as_float(0.3f);
    outPreset.wireframeDensity = root["wireframeDensity"].as_float(1.0f);
    outPreset.strobeFlash = root["strobeFlash"].as_float(0.0f);

    outPreset.customParam0 = root["customParam0"].as_float(1.0f);
    outPreset.customParam1 = root["customParam1"].as_float(1.0f);
    outPreset.customParam2 = root["customParam2"].as_float(0.5f);
    outPreset.customParam3 = root["customParam3"].as_float(1.0f);
    outPreset.customParam4 = root["customParam4"].as_float(0.0f);
    outPreset.customParam5 = root["customParam5"].as_float(1.0f);
    outPreset.customParam6 = root["customParam6"].as_float(0.0f);
    outPreset.customParam7 = root["customParam7"].as_float(0.0f);

    auto loadCol = [](const Json::Value& arr, float* out) {
        if (arr.is_array() && arr.as_array().size() >= 4) {
            out[0] = arr.as_array()[0].as_float(1.0f);
            out[1] = arr.as_array()[1].as_float(1.0f);
            out[2] = arr.as_array()[2].as_float(1.0f);
            out[3] = arr.as_array()[3].as_float(1.0f);
        }
    };

    loadCol(root["colorPrimary"], outPreset.colorPrimary);
    loadCol(root["colorSecondary"], outPreset.colorSecondary);
    loadCol(root["colorAccent"], outPreset.colorAccent);

    outPreset.audioGain = root["audioGain"].as_float(1.8f);
    outPreset.autoPilot = root["autoPilot"].as_bool(false);

    return true;
}

VisualizerPreset PresetManager::createDaftAlivePreset() {
    VisualizerPreset p;
    p.name = "DaftAlivePyramid";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 0;
    p.backgroundType = 3; // Concert Stage Arena
    p.shapeType = 0;      // 0 = Alive Pyramid
    p.materialMode = 1;   // 1 = Alive LED Matrix
    p.skyLayerWeight = 0.4f;
    p.warpLayerWeight = 0.0f;
    p.floorLayerWeight = 1.2f;
    p.soundwaveLayerWeight = 0.8f;
    p.stageTrussWeight = 1.2f;
    p.hudLayerWeight = 0.2f;
    p.laserIntensity = 1.4f;
    p.duplicationMode = 0; // Single
    p.cameraDrift = 1.0f;
    p.fluidityAmount = 0.0f;
    p.particleIntensity = 0.6f;
    p.vortexTwist = 0.0f;
    p.bloomIntensity = 1.35f;
    p.glitchRate = 0.25f;
    p.wireframeDensity = 1.2f;
    p.customParam0 = 1.0f; // Zoom
    p.customParam1 = 1.0f; // Speed
    p.customParam2 = 0.5f; // Density
    p.customParam3 = 1.0f; // Glow
    p.customParam4 = 0.0f; // Rotation
    p.customParam5 = 1.0f; // Sensitivity
    p.customParam6 = 0.0f; // Spread
    p.customParam7 = 0.0f; // Glitch Override
    p.colorPrimary[0] = 1.0f; p.colorPrimary[1] = 0.15f; p.colorPrimary[2] = 0.0f; p.colorPrimary[3] = 1.0f; // Red/Orange
    p.colorSecondary[0] = 0.0f; p.colorSecondary[1] = 0.7f; p.colorSecondary[2] = 1.0f; p.colorSecondary[3] = 1.0f; // Cyan
    p.colorAccent[0] = 1.0f; p.colorAccent[1] = 0.85f; p.colorAccent[2] = 0.1f; p.colorAccent[3] = 1.0f; // Gold
    return p;
}

VisualizerPreset PresetManager::createLiquidCyberPreset() {
    VisualizerPreset p;
    p.name = "LiquidCyber";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 0;
    p.backgroundType = 1; // Hyperspace Warp Tunnel
    p.shapeType = 1;      // 1 = TRON Recognizer
    p.materialMode = 0;   // 0 = Smoked Obsidian & Neon
    p.skyLayerWeight = 0.6f;
    p.warpLayerWeight = 0.7f;
    p.floorLayerWeight = 1.3f;
    p.soundwaveLayerWeight = 0.6f;
    p.hudLayerWeight = 0.6f;
    p.stageTrussWeight = 0.0f;
    p.laserIntensity = 0.8f;
    p.duplicationMode = 1; // Orbital Ring
    p.cameraDrift = 1.0f;
    p.fluidityAmount = 0.0f;
    p.particleIntensity = 1.2f;
    p.vortexTwist = 0.0f;
    p.bloomIntensity = 1.4f;
    p.glitchRate = 0.2f;
    p.wireframeDensity = 1.0f;
    p.colorPrimary[0] = 1.0f; p.colorPrimary[1] = 0.05f; p.colorPrimary[2] = 0.55f; p.colorPrimary[3] = 1.0f; // Neon Pink
    p.colorSecondary[0] = 0.0f; p.colorSecondary[1] = 0.95f; p.colorSecondary[2] = 0.9f; p.colorSecondary[3] = 1.0f; // Cyan
    p.colorAccent[0] = 1.0f; p.colorAccent[1] = 0.4f; p.colorAccent[2] = 0.0f; p.colorAccent[3] = 1.0f; // Sunset Orange
    return p;
}

VisualizerPreset PresetManager::createSynthwaveHorizonPreset() {
    VisualizerPreset p;
    p.name = "SynthwaveHorizon";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 0;
    p.backgroundType = 0; // Starfield & Nebula
    p.shapeType = 3;      // 3 = Holographic HUD Core
    p.materialMode = 2;   // 2 = Sci-Fi HUD
    p.skyLayerWeight = 1.2f;
    p.warpLayerWeight = 0.0f;
    p.floorLayerWeight = 1.0f;
    p.soundwaveLayerWeight = 0.9f;
    p.hudLayerWeight = 1.0f;
    p.stageTrussWeight = 0.0f;
    p.laserIntensity = 0.5f;
    p.duplicationMode = 0; // Single
    p.cameraDrift = 1.0f;
    p.fluidityAmount = 0.0f;
    p.particleIntensity = 1.0f;
    p.vortexTwist = 0.0f;
    p.bloomIntensity = 1.25f;
    p.glitchRate = 0.15f;
    p.wireframeDensity = 1.2f;
    p.colorPrimary[0] = 1.0f; p.colorPrimary[1] = 0.2f; p.colorPrimary[2] = 0.5f; p.colorPrimary[3] = 1.0f; // Magenta
    p.colorSecondary[0] = 0.0f; p.colorSecondary[1] = 0.8f; p.colorSecondary[2] = 1.0f; p.colorSecondary[3] = 1.0f; // Sky Blue
    p.colorAccent[0] = 1.0f; p.colorAccent[1] = 0.8f; p.colorAccent[2] = 0.1f; p.colorAccent[3] = 1.0f; // Warm Gold
    return p;
}

VisualizerPreset PresetManager::createGyroLaserArenaPreset() {
    VisualizerPreset p;
    p.name = "GyroLaserArena";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 0;
    p.backgroundType = 3; // Stage Arena
    p.shapeType = 2;      // 2 = Identity Disc Gyro
    p.materialMode = 0;   // 0 = Smoked Obsidian
    p.skyLayerWeight = 0.3f;
    p.warpLayerWeight = 0.0f;
    p.floorLayerWeight = 1.2f;
    p.soundwaveLayerWeight = 0.6f;
    p.stageTrussWeight = 1.3f;
    p.hudLayerWeight = 0.5f;
    p.laserIntensity = 1.8f;
    p.duplicationMode = 2; // Triad Array
    p.cameraDrift = 1.0f;
    p.fluidityAmount = 0.0f;
    p.particleIntensity = 1.0f;
    p.vortexTwist = 0.0f;
    p.bloomIntensity = 1.6f;
    p.glitchRate = 0.3f;
    p.wireframeDensity = 1.0f;
    p.colorPrimary[0] = 0.2f; p.colorPrimary[1] = 1.0f; p.colorPrimary[2] = 0.0f; p.colorPrimary[3] = 1.0f; // Acid Green
    p.colorSecondary[0] = 0.7f; p.colorSecondary[1] = 0.0f; p.colorSecondary[2] = 1.0f; p.colorSecondary[3] = 1.0f; // Violet
    p.colorAccent[0] = 1.0f; p.colorAccent[1] = 1.0f; p.colorAccent[2] = 0.0f; p.colorAccent[3] = 1.0f; // Yellow
    return p;
}

VisualizerPreset PresetManager::createAcidPlasmaPreset() {
    VisualizerPreset p;
    p.name = "AcidPlasma";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 0;
    p.backgroundType = 2; // Refined LED Soundwave Void
    p.shapeType = 5;      // 5 = Hex Data Monoliths
    p.materialMode = 0;   // 0 = Smoked Obsidian
    p.skyLayerWeight = 0.8f;
    p.warpLayerWeight = 0.4f;
    p.floorLayerWeight = 1.2f;
    p.soundwaveLayerWeight = 1.2f;
    p.stageTrussWeight = 0.0f;
    p.hudLayerWeight = 0.7f;
    p.laserIntensity = 1.0f;
    p.duplicationMode = 0; // Single
    p.cameraDrift = 1.0f;
    p.fluidityAmount = 0.0f;
    p.particleIntensity = 1.5f;
    p.vortexTwist = 0.0f;
    p.bloomIntensity = 1.5f;
    p.glitchRate = 0.25f;
    p.wireframeDensity = 0.9f;
    p.colorPrimary[0] = 0.0f; p.colorPrimary[1] = 1.0f; p.colorPrimary[2] = 0.7f; p.colorPrimary[3] = 1.0f; // Emerald Cyan
    p.colorSecondary[0] = 1.0f; p.colorSecondary[1] = 0.0f; p.colorSecondary[2] = 0.4f; p.colorSecondary[3] = 1.0f; // Hot Pink
    p.colorAccent[0] = 0.1f; p.colorAccent[1] = 0.7f; p.colorAccent[2] = 1.0f; p.colorAccent[3] = 1.0f; // Electric Blue
    return p;
}

VisualizerPreset PresetManager::createRandomDropRecipe() {
    VisualizerPreset p;
    static const char* dropNames[] = {
        "Liquid Surge", "Vortex Blast", "Laser Ignition", "Cyber Warp",
        "Plasma Storm", "Alive Apex", "Quantum Ripple", "Tesseract Shift", "Prism Burst",
        "Obsidian Grid", "Holo Shield", "Data Monoliths", "Neon Conduit"
    };
    int nameIdx = rand() % (sizeof(dropNames) / sizeof(dropNames[0]));
    p.name = dropNames[nameIdx];
    p.sceneName = "ModularMixer";
    p.sceneIndex = 0;

    // Pick Hero Shape: 0..8
    // 0=Alive Pyramid, 1=TRON Recognizer, 2=Identity Disc Gyro, 3=Holo HUD Core, 4=4D Tesseract, 5=Hex Data Monoliths, 6=Cyber Light-Ribbon, 7=Liquid Mercury, 8=No Shape
    p.shapeType = rand() % 9;

    // Clean Material Mode matching
    // 0 = Smoked Obsidian & Neon (TRON), 1 = Alive LED Matrix (Daft), 2 = Sci-Fi HUD, 3 = Liquid Chrome
    if (p.shapeType == 0) {
        p.materialMode = (rand() % 3 == 0) ? 0 : 1; // Alive LED Matrix or Obsidian
    } else if (p.shapeType == 3) {
        p.materialMode = 2; // Sci-Fi HUD
    } else if (p.shapeType == 7) {
        p.materialMode = 3; // Liquid Chrome
    } else {
        int matChoices[] = { 0, 0, 1, 2, 3 };
        p.materialMode = matChoices[rand() % 5];
    }

    // Complementary Multi-Layer Environment Presets (Always at least 2 active layers)
    int envMix = rand() % 5;
    p.skyLayerWeight = 0.0f;
    p.warpLayerWeight = 0.0f;
    p.floorLayerWeight = 0.0f;
    p.soundwaveLayerWeight = 0.0f;
    p.stageTrussWeight = 0.0f;
    p.hudLayerWeight = 0.2f + (float)(rand() % 7) * 0.1f; // 0.2 to 0.8

    switch (envMix) {
        case 0: // Deep Space & 3D Mirror Floor (Cosmic Arena)
            p.backgroundType = 0;
            p.skyLayerWeight = 1.0f + (float)(rand() % 5) * 0.1f;
            p.floorLayerWeight = 0.9f + (float)(rand() % 5) * 0.1f;
            p.soundwaveLayerWeight = (rand() % 2 == 0) ? 0.6f : 0.0f;
            break;
        case 1: // Hyperdrive Warp Conduit & HUD Telemetry
            p.backgroundType = 1;
            p.warpLayerWeight = 0.8f + (float)(rand() % 6) * 0.1f;
            p.hudLayerWeight = 0.7f + (float)(rand() % 6) * 0.1f;
            p.skyLayerWeight = 0.4f;
            break;
        case 2: // Concert Stage: Obsidian Floor + Overhead Truss + Soundwave Wall
            p.backgroundType = 3;
            p.floorLayerWeight = 1.1f + (float)(rand() % 4) * 0.1f;
            p.stageTrussWeight = 1.0f + (float)(rand() % 4) * 0.1f;
            p.soundwaveLayerWeight = 0.7f + (float)(rand() % 6) * 0.1f;
            p.skyLayerWeight = 0.3f;
            break;
        case 3: // 3D TRON Grid Floor + Soundwave Horizon Wall
            p.backgroundType = 2;
            p.floorLayerWeight = 1.2f;
            p.soundwaveLayerWeight = 1.0f + (float)(rand() % 4) * 0.1f;
            p.skyLayerWeight = 0.5f;
            break;
        case 4: default: // Warp + Mirror Floor + Truss
            p.backgroundType = 1;
            p.warpLayerWeight = 0.7f + (float)(rand() % 5) * 0.1f;
            p.floorLayerWeight = 1.0f + (float)(rand() % 4) * 0.1f;
            p.stageTrussWeight = (rand() % 2 == 0) ? 0.8f : 0.0f;
            break;
    }

    // Volumetric Lasers (Always viable across multi-layer environments)
    p.laserIntensity = (rand() % 3 != 0) ? (0.8f + (float)(rand() % 10) * 0.1f) : 0.0f;
    p.customParam6 = ((float)(rand() % 200) / 100.0f - 1.0f); // Fan spread

    // Particle Swarm
    p.particleIntensity = 0.6f + (float)(rand() % 12) * 0.1f;

    // Modifier Anti-Clash Rules:
    // Never enable fluidityAmount > 0.2f, vortexTwist != 0.0f, and duplicationMode == 3 all at once.
    // Pick at most ONE spatial modifier per recipe.
    int modChoice = rand() % 4; // 0 = Clean, 1 = Fluid morph, 2 = Vortex twist, 3 = Duplication
    p.fluidityAmount = 0.0f;
    p.vortexTwist = 0.0f;
    p.duplicationMode = 0;
    p.duplicationParam0 = 1.0f;
    p.duplicationParam1 = 0.5f;

    if (modChoice == 1) {
        p.fluidityAmount = 0.8f + (float)(rand() % 6) * 0.1f;
    } else if (modChoice == 2) {
        float twistSign = (rand() % 2 == 0) ? 1.0f : -1.0f;
        p.vortexTwist = twistSign * (0.4f + (float)(rand() % 7) * 0.1f);
    } else if (modChoice == 3) {
        p.duplicationMode = 1 + (rand() % 4); // 1..4 (Ring, Triad, Kaleidoscope, Depth)
        p.duplicationParam0 = 0.8f + (float)(rand() % 10) * 0.1f;
        p.duplicationParam1 = 0.3f + (float)(rand() % 5) * 0.1f;
    }

    p.cameraDrift = 1.0f;
    p.bloomIntensity = 1.2f + (float)(rand() % 8) * 0.1f;
    p.glitchRate = 0.15f + (float)(rand() % 25) * 0.01f;
    p.wireframeDensity = 0.9f + (float)(rand() % 5) * 0.1f;

    // Vibrant DJ concert palettes
    int paletteType = rand() % 5;
    if (paletteType == 0) { // Red & Cyan & Gold (Daft Alive)
        p.colorPrimary[0] = 1.0f; p.colorPrimary[1] = 0.15f; p.colorPrimary[2] = 0.0f; p.colorPrimary[3] = 1.0f;
        p.colorSecondary[0] = 0.0f; p.colorSecondary[1] = 0.75f; p.colorSecondary[2] = 1.0f; p.colorSecondary[3] = 1.0f;
        p.colorAccent[0] = 1.0f; p.colorAccent[1] = 0.85f; p.colorAccent[2] = 0.1f; p.colorAccent[3] = 1.0f;
    } else if (paletteType == 1) { // Neon Pink & Cyan & Sunset Orange
        p.colorPrimary[0] = 1.0f; p.colorPrimary[1] = 0.05f; p.colorPrimary[2] = 0.6f; p.colorPrimary[3] = 1.0f;
        p.colorSecondary[0] = 0.0f; p.colorSecondary[1] = 0.9f; p.colorSecondary[2] = 0.9f; p.colorSecondary[3] = 1.0f;
        p.colorAccent[0] = 1.0f; p.colorAccent[1] = 0.5f; p.colorAccent[2] = 0.0f; p.colorAccent[3] = 1.0f;
    } else if (paletteType == 2) { // Acid Green & Violet & Gold
        p.colorPrimary[0] = 0.2f; p.colorPrimary[1] = 1.0f; p.colorPrimary[2] = 0.1f; p.colorPrimary[3] = 1.0f;
        p.colorSecondary[0] = 0.7f; p.colorSecondary[1] = 0.0f; p.colorSecondary[2] = 1.0f; p.colorSecondary[3] = 1.0f;
        p.colorAccent[0] = 1.0f; p.colorAccent[1] = 0.9f; p.colorAccent[2] = 0.0f; p.colorAccent[3] = 1.0f;
    } else if (paletteType == 3) { // Electric Azure & Magenta & White
        p.colorPrimary[0] = 0.0f; p.colorPrimary[1] = 0.6f; p.colorPrimary[2] = 1.0f; p.colorPrimary[3] = 1.0f;
        p.colorSecondary[0] = 1.0f; p.colorSecondary[1] = 0.0f; p.colorSecondary[2] = 0.5f; p.colorSecondary[3] = 1.0f;
        p.colorAccent[0] = 0.9f; p.colorAccent[1] = 0.95f; p.colorAccent[2] = 1.0f; p.colorAccent[3] = 1.0f;
    } else { // Sunset Gold & Crimson & Teal
        p.colorPrimary[0] = 1.0f; p.colorPrimary[1] = 0.4f; p.colorPrimary[2] = 0.0f; p.colorPrimary[3] = 1.0f;
        p.colorSecondary[0] = 0.0f; p.colorSecondary[1] = 0.8f; p.colorSecondary[2] = 0.7f; p.colorSecondary[3] = 1.0f;
        p.colorAccent[0] = 1.0f; p.colorAccent[1] = 0.0f; p.colorAccent[2] = 0.2f; p.colorAccent[3] = 1.0f;
    }

    return p;
}

} // namespace Visualizer::Control

