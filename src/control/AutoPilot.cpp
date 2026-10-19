#include "AutoPilot.h"
#include <cmath>
#include <algorithm>

namespace Visualizer::Control {

AutoPilot::AutoPilot() {
    initScenes();
}

void AutoPilot::initScenes() {
    m_scenes.clear();

    // Scene 0: 🔺 Alive Pyramid (Daft Alive 2007 Concert Rig)
    {
        SceneDefinition s;
        s.name = "Alive Pyramid";
        s.icon = "🔺";
        s.pyramidWeight = 1.0f;
        s.logoWeight = 0.35f;       // Small top/apex logo
        s.laserIntensity = 1.2f;
        s.floorLayerWeight = 1.0f;
        s.skyLayerWeight = 0.4f;
        s.particleIntensity = 0.6f;
        s.defaultPattern = 0;       // VU strips
        s.defaultPalette = 0;       // Daft Red/Gold
        m_scenes.push_back(s);
    }

    // Scene 1: 🏍️ TRON Grid Run (TRON: Legacy High-Speed Highway)
    {
        SceneDefinition s;
        s.name = "TRON Grid Run";
        s.icon = "🏍️";
        s.floorLayerWeight = 1.3f;
        s.trailsWeight = 1.0f;       // Light-cycle trails
        s.particleIntensity = 1.0f;  // Sparks
        s.horizonWeight = 1.0f;
        s.skyLayerWeight = 0.3f;
        s.defaultPalette = 1;       // TRON Cyan/Orange
        m_scenes.push_back(s);
    }

    // Scene 2: 🎃 Spooky Punk (Halloween Trunk-or-Treat Special)
    {
        SceneDefinition s;
        s.name = "Spooky Punk";
        s.icon = "🎃";
        s.spookyWeight = 1.0f;       // Full moon + bats + jack-o'-lanterns
        s.pyramidWeight = 0.85f;
        s.defaultPattern = 1;       // Rising chevrons
        s.laserIntensity = 0.6f;
        s.skyLayerWeight = 1.0f;
        s.floorLayerWeight = 0.8f;
        s.particleIntensity = 1.2f;  // Bats flapping
        s.logoWeight = 0.5f;
        s.defaultPalette = 2;       // Halloween Orange/Purple/Green
        m_scenes.push_back(s);
    }

    // Scene 3: 📟 LED Wall (Full-Screen Stadium LED Matrix)
    {
        SceneDefinition s;
        s.name = "LED Wall";
        s.icon = "📟";
        s.ledWallWeight = 1.2f;
        s.logoWeight = 1.0f;
        s.laserIntensity = 0.5f;
        s.floorLayerWeight = 0.4f;
        s.defaultPalette = 0;       // Gold/Amber
        m_scenes.push_back(s);
    }

    // Scene 4: 🤖 DAD PUNK Hero (Centerpiece Title Screen)
    {
        SceneDefinition s;
        s.name = "DAD PUNK Hero";
        s.icon = "🤖";
        s.logoWeight = 1.35f;       // Massive center logo
        s.pyramidWeight = 0.7f;
        s.laserIntensity = 1.2f;
        s.floorLayerWeight = 1.0f;
        s.skyLayerWeight = 0.6f;
        s.particleIntensity = 0.8f;
        s.defaultPalette = 0;       // Daft Red/Gold
        m_scenes.push_back(s);
    }

    // Scene 5: 🌙 Chill Attract (Intermission / Between Songs)
    {
        SceneDefinition s;
        s.name = "Chill Attract";
        s.icon = "🌙";
        s.skyLayerWeight = 1.0f;     // Subtle starfield
        s.floorLayerWeight = 0.6f;   // Slow grid
        s.logoWeight = 0.55f;       // Soft glowing logo
        s.pyramidWeight = 0.25f;
        s.particleIntensity = 0.3f;
        s.defaultPattern = 3;       // Solid subtle glow
        s.defaultPalette = 3;       // Chrome Silver/Blue
        m_scenes.push_back(s);
    }
}

void AutoPilot::selectScene(int sceneIndex, float holdSeconds) {
    if (sceneIndex < 0 || sceneIndex >= static_cast<int>(m_scenes.size())) return;

    m_director.targetScene = sceneIndex;
    m_director.transitionProgress = 0.0f;
    m_director.timeInCurrentScene = 0.0f;

    if (holdSeconds > 0.0f) {
        m_user.manualScene = sceneIndex;
        m_user.manualHoldTimer = holdSeconds;
    } else {
        m_user.manualScene = -1;
        m_user.manualHoldTimer = 0.0f;
    }
}

void AutoPilot::triggerDropTransition() {
    int nextScene = 0;
    if (m_director.currentScene == 5) {
        // Leaving chill mode on drop -> jump to Alive Pyramid or Hero
        nextScene = 0;
    } else {
        // Cycle among active energetic scenes 0..4
        nextScene = (m_director.currentScene + 1) % 5;
    }

    selectScene(nextScene, 30.0f);
}

float AutoPilot::getTimeRemainingInScene() const {
    if (m_user.manualScene >= 0 && m_user.manualHoldTimer > 0.0f) {
        return m_user.manualHoldTimer;
    }
    return (std::max)(0.0f, m_director.phraseInterval - m_director.timeInCurrentScene);
}

void AutoPilot::update(
    float deltaTime,
    const Audio::BeatDetector& beatDetector,
    const AudioFrameData& audioData
) {
    // 1. Crossfade transition progress
    if (m_director.transitionProgress < 1.0f) {
        m_director.transitionProgress += deltaTime * 1.5f; // ~0.66s crossfade
        if (m_director.transitionProgress >= 1.0f) {
            m_director.transitionProgress = 1.0f;
            m_director.currentScene = m_director.targetScene;
        }
    }

    // 2. Manual hold countdown
    if (m_user.manualScene >= 0) {
        m_user.manualHoldTimer -= deltaTime;
        if (m_user.manualHoldTimer <= 0.0f) {
            m_user.manualHoldTimer = 0.0f;
            m_user.manualScene = -1; // Resume Auto-Pilot smoothly
        }
    }

    // 3. Audio energy tracking & silence detection
    float instantEnergy = (audioData.subBass * 0.4f + audioData.bass * 0.4f + audioData.highMid * 0.2f);
    m_director.smoothedEnergy += (instantEnergy - m_director.smoothedEnergy) * (std::min)(1.0f, deltaTime * 4.0f);

    bool isSilent = audioData.isSilent || (audioData.rms < 0.005f);
    if (isSilent) {
        m_director.silenceTimer += deltaTime;
        // After 4s of silence, auto-transition to Chill Attract (Scene 5) if not in manual hold
        if (m_director.silenceTimer >= 4.0f && m_director.currentScene != 5 && m_user.manualScene < 0) {
            m_director.targetScene = 5;
            m_director.transitionProgress = 0.0f;
        }
    } else {
        m_director.silenceTimer = 0.0f;
        // If music returns while in Chill Attract, wake up into Scene 0 (Alive Pyramid)
        if (m_director.currentScene == 5 && m_director.targetScene == 5 && m_user.manualScene < 0 && audioData.rms > 0.015f) {
            m_director.targetScene = 0;
            m_director.transitionProgress = 0.0f;
            m_director.timeInCurrentScene = 0.0f;
        }
    }

    // 4. Auto-Pilot phrase transition logic (when not holding manual scene and not silent)
    if (m_director.isAutoPilotEnabled && m_user.manualScene < 0 && !isSilent) {
        m_director.timeInCurrentScene += deltaTime;

        // Check if 30s phrase has elapsed
        if (m_director.timeInCurrentScene >= m_director.phraseInterval) {
            // Trigger transition on beat pulse or after 4s timeout
            if (beatDetector.getBeatPulse() > 0.65f || m_director.timeInCurrentScene >= m_director.phraseInterval + 4.0f) {
                // Cycle scenes 0..4 (skip chill attract when music is playing)
                int nextScene = (m_director.currentScene + 1) % 5;
                m_director.targetScene = nextScene;
                m_director.transitionProgress = 0.0f;
                m_director.timeInCurrentScene = 0.0f;
            }
        }
    }

    // 5. Photosafety Limiter Monitoring
    float currLuminance = (audioData.beatPulse * 0.5f + audioData.dropIntensity * 0.8f);
    float deltaLum = std::abs(currLuminance - m_lastLuminance);
    m_lastLuminance = currLuminance;

    if (deltaLum > 0.45f) {
        m_flashRateCounter += 1.0f;
    }
    m_flashRateCounter = (std::max)(0.0f, m_flashRateCounter - deltaTime * 3.0f);

    // If flash rate exceeds safe 3 flashes per second threshold, damp transitions
    if (m_flashRateCounter > 3.0f) {
        m_safetyDamping = 0.65f;
    } else {
        m_safetyDamping = (std::min)(1.0f, m_safetyDamping + deltaTime * 0.5f);
    }
}

void AutoPilot::resolveGpuParams(
    const AudioFrameData& audioData,
    Graphics::AudioVisualizerParams& outParams
) {
    if (m_scenes.empty()) return;

    int curIdx = std::clamp(m_director.currentScene, 0, static_cast<int>(m_scenes.size()) - 1);
    int tarIdx = std::clamp(m_director.targetScene, 0, static_cast<int>(m_scenes.size()) - 1);

    const auto& sCurr = m_scenes[curIdx];
    const auto& sTarg = m_scenes[tarIdx];

    float t = std::clamp(m_director.transitionProgress, 0.0f, 1.0f);
    float easeT = t * t * (3.0f - 2.0f * t); // Smoothstep crossfade

    auto mixVal = [&](float a, float b) {
        return a + (b - a) * easeT;
    };

    // A. Mix composable layer weights with user multipliers
    outParams.PyramidWeight = mixVal(sCurr.pyramidWeight, sTarg.pyramidWeight) * m_user.pyramidMultiplier;
    outParams.LogoWeight = mixVal(sCurr.logoWeight, sTarg.logoWeight) * m_user.logoMultiplier;
    outParams.LaserIntensity = mixVal(sCurr.laserIntensity, sTarg.laserIntensity) * m_user.laserMultiplier;
    outParams.FloorLayerWeight = mixVal(sCurr.floorLayerWeight, sTarg.floorLayerWeight) * m_user.floorMultiplier;
    outParams.SkyLayerWeight = mixVal(sCurr.skyLayerWeight, sTarg.skyLayerWeight) * m_user.skyMultiplier;
    outParams.ParticleIntensity = mixVal(sCurr.particleIntensity, sTarg.particleIntensity) * m_user.particleMultiplier;
    outParams.TrailsWeight = mixVal(sCurr.trailsWeight, sTarg.trailsWeight);
    outParams.LedWallWeight = mixVal(sCurr.ledWallWeight, sTarg.ledWallWeight);
    outParams.HorizonWeight = mixVal(sCurr.horizonWeight, sTarg.horizonWeight);

    // Spooky weight: mix scene target and override if Halloween Mode is active
    float mixedSpooky = mixVal(sCurr.spookyWeight, sTarg.spookyWeight);
    if (m_user.halloweenMode) {
        mixedSpooky = (std::max)(mixedSpooky, 1.0f);
    }
    outParams.SpookyWeight = mixedSpooky;

    // Pattern & style modes
    outParams.PyramidPattern = (m_user.pyramidPattern != 0) ? m_user.pyramidPattern : sTarg.defaultPattern;
    outParams.LogoStyle = m_user.logoStyle;
    outParams.ShapeCrossfade = easeT;

    // B. Palette Colors (5 DJ Concert Palettes)
    static const Graphics::Float4 palettes[5][3] = {
        // 0: Daft Red/Gold
        { { 1.0f, 0.15f, 0.0f, 1.0f }, { 0.0f, 0.75f, 1.0f, 1.0f }, { 1.0f, 0.85f, 0.1f, 1.0f } },
        // 1: TRON Cyan/Orange
        { { 0.0f, 0.85f, 1.0f, 1.0f }, { 1.0f, 0.45f, 0.0f, 1.0f }, { 1.0f, 0.9f, 0.2f, 1.0f } },
        // 2: Halloween Orange/Purple/Green
        { { 1.0f, 0.42f, 0.0f, 1.0f }, { 0.6f, 0.1f, 0.85f, 1.0f }, { 0.2f, 1.0f, 0.2f, 1.0f } },
        // 3: Chrome Silver/Blue
        { { 0.85f, 0.9f, 1.0f, 1.0f }, { 0.2f, 0.5f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        // 4: Rainbow Discovery
        { { 1.0f, 0.2f, 0.4f, 1.0f }, { 0.1f, 0.9f, 0.8f, 1.0f }, { 1.0f, 0.9f, 0.1f, 1.0f } }
    };

    int pIdx = std::clamp(m_user.paletteIndex, 0, 4);
    outParams.ColorPrimary = palettes[pIdx][0];
    outParams.ColorSecondary = palettes[pIdx][1];
    outParams.ColorAccent = palettes[pIdx][2];

    // C. Pack Custom Text (16 packed ASCII characters)
    for (int i = 0; i < 4; ++i) {
        outParams.CustomText[i].x = static_cast<uint32_t>(m_user.customText[i * 4 + 0]);
        outParams.CustomText[i].y = static_cast<uint32_t>(m_user.customText[i * 4 + 1]);
        outParams.CustomText[i].z = static_cast<uint32_t>(m_user.customText[i * 4 + 2]);
        outParams.CustomText[i].w = static_cast<uint32_t>(m_user.customText[i * 4 + 3]);
    }

    // D. Ingest Audio Frame Data
    outParams.AudioSubBass = audioData.subBass;
    outParams.AudioKick = audioData.bass;
    outParams.AudioLowMid = audioData.lowMid;
    outParams.AudioHighMid = audioData.highMid;
    outParams.AudioTreble = audioData.treble;
    outParams.BeatPulse = audioData.beatPulse;
    outParams.DropIntensity = audioData.dropIntensity;
    outParams.EnergyAccumulator = audioData.energyAccumulator;
    outParams.Bpm = audioData.bpm;

    // E. Photosafety & Kid-Safe Controls
    outParams.MasterBrightness = m_user.masterBrightness;
    outParams.SafetyDamping = m_safetyDamping;
}

} // namespace Visualizer::Control
