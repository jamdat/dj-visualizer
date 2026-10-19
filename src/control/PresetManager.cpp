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
        savePreset(createTronGridPreset());
        savePreset(createSpookyPunkPreset());
        savePreset(createLedWallPreset());
        savePreset(createDadPunkHeroPreset());
        savePreset(createChillAttractPreset());
        Log("Default Dad Punk presets saved successfully");
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

static std::string SanitizePresetName(const std::string& raw) {
    std::string clean;
    for (char c : raw) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ' ') {
            clean.push_back(c);
        }
    }
    if (clean.empty()) {
        clean = "Default";
    }
    return clean;
}

bool PresetManager::savePreset(const VisualizerPreset& preset) {
    std::error_code ec;
    if (!std::filesystem::exists(m_presetsDir, ec)) {
        std::filesystem::create_directories(m_presetsDir, ec);
    }

    std::string safeName = SanitizePresetName(preset.name);

    Json::Value root(Json::Value::Object{});
    root["name"] = safeName;
    root["sceneName"] = preset.sceneName;
    root["sceneIndex"] = preset.sceneIndex;

    root["pyramidMultiplier"] = preset.pyramidMultiplier;
    root["logoMultiplier"] = preset.logoMultiplier;
    root["laserMultiplier"] = preset.laserMultiplier;
    root["floorMultiplier"] = preset.floorMultiplier;
    root["skyMultiplier"] = preset.skyMultiplier;
    root["particleMultiplier"] = preset.particleMultiplier;

    root["pyramidPattern"] = preset.pyramidPattern;
    root["logoStyle"] = preset.logoStyle;
    root["paletteIndex"] = preset.paletteIndex;
    root["halloweenMode"] = preset.halloweenMode;
    root["masterBrightness"] = preset.masterBrightness;
    root["customText"] = preset.customText;

    root["audioGain"] = preset.audioGain;
    root["autoPilot"] = preset.autoPilot;

    std::string filePath = m_presetsDir + "/" + safeName + ".json";
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << root.dump(2);
    return true;
}

bool PresetManager::loadPreset(const std::string& presetName, VisualizerPreset& outPreset) {
    std::string safeName = SanitizePresetName(presetName);
    std::string filePath = m_presetsDir + "/" + safeName + ".json";
    if (!std::filesystem::exists(filePath)) return false;

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    Json::Value root = Json::Value::parse(content);

    outPreset.name = root["name"].as_string(presetName);
    outPreset.sceneName = root["sceneName"].as_string("ModularMixer");
    outPreset.sceneIndex = root["sceneIndex"].as_int(0);

    outPreset.pyramidMultiplier = root["pyramidMultiplier"].as_float(1.0f);
    outPreset.logoMultiplier = root["logoMultiplier"].as_float(1.0f);
    outPreset.laserMultiplier = root["laserMultiplier"].as_float(1.0f);
    outPreset.floorMultiplier = root["floorMultiplier"].as_float(1.0f);
    outPreset.skyMultiplier = root["skyMultiplier"].as_float(1.0f);
    outPreset.particleMultiplier = root["particleMultiplier"].as_float(1.0f);

    outPreset.pyramidPattern = root["pyramidPattern"].as_int(0);
    outPreset.logoStyle = root["logoStyle"].as_int(0);
    outPreset.paletteIndex = root["paletteIndex"].as_int(0);
    outPreset.halloweenMode = root["halloweenMode"].as_bool(false);
    outPreset.masterBrightness = root["masterBrightness"].as_float(0.85f);
    outPreset.customText = root["customText"].as_string("DAD PUNK");

    outPreset.audioGain = root["audioGain"].as_float(1.8f);
    outPreset.autoPilot = root["autoPilot"].as_bool(true);

    return true;
}

VisualizerPreset PresetManager::createDaftAlivePreset() {
    VisualizerPreset p;
    p.name = "AlivePyramid";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 0;
    p.pyramidMultiplier = 1.0f;
    p.logoMultiplier = 0.5f;
    p.laserMultiplier = 1.2f;
    p.floorMultiplier = 1.0f;
    p.skyMultiplier = 0.4f;
    p.particleMultiplier = 0.6f;
    p.pyramidPattern = 0; // VU strips
    p.paletteIndex = 0;   // Daft Red/Gold
    p.masterBrightness = 0.85f;
    p.customText = "DAD PUNK";
    return p;
}

VisualizerPreset PresetManager::createTronGridPreset() {
    VisualizerPreset p;
    p.name = "TronGridRun";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 1;
    p.pyramidMultiplier = 0.0f;
    p.logoMultiplier = 0.0f;
    p.laserMultiplier = 0.0f;
    p.floorMultiplier = 1.3f;
    p.skyMultiplier = 0.3f;
    p.particleMultiplier = 1.0f;
    p.paletteIndex = 1;   // TRON Cyan/Orange
    p.masterBrightness = 0.85f;
    p.customText = "DAD PUNK";
    return p;
}

VisualizerPreset PresetManager::createSpookyPunkPreset() {
    VisualizerPreset p;
    p.name = "SpookyPunk";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 2;
    p.pyramidMultiplier = 0.85f;
    p.logoMultiplier = 0.5f;
    p.laserMultiplier = 0.6f;
    p.floorMultiplier = 0.8f;
    p.skyMultiplier = 1.0f;
    p.particleMultiplier = 1.2f;
    p.pyramidPattern = 1; // Rising chevrons
    p.paletteIndex = 2;   // Halloween
    p.halloweenMode = true;
    p.masterBrightness = 0.85f;
    p.customText = "DAD PUNK";
    return p;
}

VisualizerPreset PresetManager::createLedWallPreset() {
    VisualizerPreset p;
    p.name = "LedWall";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 3;
    p.pyramidMultiplier = 0.0f;
    p.logoMultiplier = 1.0f;
    p.laserMultiplier = 0.5f;
    p.floorMultiplier = 0.4f;
    p.paletteIndex = 0;   // Gold/Amber
    p.masterBrightness = 0.85f;
    p.customText = "DAD PUNK";
    return p;
}

VisualizerPreset PresetManager::createDadPunkHeroPreset() {
    VisualizerPreset p;
    p.name = "DadPunkHero";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 4;
    p.pyramidMultiplier = 0.7f;
    p.logoMultiplier = 1.35f;
    p.laserMultiplier = 1.2f;
    p.floorMultiplier = 1.0f;
    p.skyMultiplier = 0.6f;
    p.particleMultiplier = 0.8f;
    p.paletteIndex = 0;   // Daft Red/Gold
    p.masterBrightness = 0.85f;
    p.customText = "DAD PUNK";
    return p;
}

VisualizerPreset PresetManager::createChillAttractPreset() {
    VisualizerPreset p;
    p.name = "ChillAttract";
    p.sceneName = "ModularMixer";
    p.sceneIndex = 5;
    p.pyramidMultiplier = 0.25f;
    p.logoMultiplier = 0.55f;
    p.laserMultiplier = 0.0f;
    p.floorMultiplier = 0.6f;
    p.skyMultiplier = 1.0f;
    p.particleMultiplier = 0.3f;
    p.pyramidPattern = 3; // Solid glow
    p.paletteIndex = 3;   // Chrome Silver/Blue
    p.masterBrightness = 0.85f;
    p.customText = "DAD PUNK";
    return p;
}

} // namespace Visualizer::Control
