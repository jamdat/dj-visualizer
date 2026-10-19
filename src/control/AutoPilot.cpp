#include "AutoPilot.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace Visualizer::Control {

AutoPilot::AutoPilot() {
    rollNextQueuedRecipe();
    m_activeTargetRecipe = m_queuedRecipe;
}

void AutoPilot::rollNextQueuedRecipe() {
    m_queuedRecipe = PresetManager::createRandomDropRecipe();
}

void AutoPilot::stageRecipe(const VisualizerPreset& recipe) {
    m_pendingRecipe = recipe;
    m_hasPendingChanges = true;
}

void AutoPilot::triggerDropTransition(Graphics::AudioVisualizerParams& params) {
    VisualizerPreset target;
    if (m_hasPendingChanges) {
        target = m_pendingRecipe;
        m_hasPendingChanges = false;
    } else {
        target = m_queuedRecipe;
        rollNextQueuedRecipe();
    }

    m_activeTargetRecipe = target;

    // Set discrete integer modes immediately
    params.ShapeType = target.shapeType;
    params.MaterialMode = target.materialMode;
    params.DuplicationMode = target.duplicationMode;
    params.BackgroundType = target.backgroundType;

    // Trigger punchy drop visual impact and crossfade envelope
    params.ShapeCrossfade = 0.0f; // Crossfades back 0.0 -> 1.0
    params.BloomIntensity = 2.4f;
    params.GlitchRate = 0.5f;
    m_isInterpolating = true;

    m_dropCooldown = 4.5f;
    m_timeInCurrentScene = 0.0f;
}

float AutoPilot::getDropProgress() const {
    if (m_switchInterval <= 0.001f) return 0.0f;
    return (std::min)(1.0f, m_timeInCurrentScene / m_switchInterval);
}

void AutoPilot::update(
    float deltaTime,
    const Audio::BeatDetector& beatDetector,
    Graphics::AudioVisualizerParams& params
) {
    m_dropCooldown = (std::max)(0.0f, m_dropCooldown - deltaTime);
    m_beatTransitionCooldown = (std::max)(0.0f, m_beatTransitionCooldown - deltaTime);

    // Compute smoothed audio energy metric from SubBass, Kick, HighMid, and BeatPulse
    float instantEnergy = (params.AudioSubBass * 0.35f + params.AudioKick * 0.35f + params.AudioHighMid * 0.15f + beatDetector.getBeatPulse() * 0.25f);
    m_smoothedEnergy += (instantEnergy - m_smoothedEnergy) * (std::min)(1.0f, deltaTime * 5.0f);

    // Smoothly decay transient bloom and glitch rate back toward baseline
    float targetBloom = m_activeTargetRecipe.bloomIntensity;
    if (params.BloomIntensity > targetBloom + 0.1f) {
        params.BloomIntensity = (std::max)(targetBloom, params.BloomIntensity - deltaTime * 1.5f);
    }
    float targetGlitch = m_activeTargetRecipe.glitchRate;
    if (params.GlitchRate > targetGlitch + 0.05f) {
        params.GlitchRate = (std::max)(targetGlitch, params.GlitchRate - deltaTime * 0.6f);
    }

    // Smoothly restore ShapeCrossfade to 1.0
    if (params.ShapeCrossfade < 1.0f) {
        params.ShapeCrossfade = (std::min)(1.0f, params.ShapeCrossfade + deltaTime * 2.5f);
    }

    // Camera Orbit Animation (Only when Camera Drift is active)
    if (params.CameraDrift > 0.5f) {
        m_cameraAngle += deltaTime * 0.25f * (1.0f + beatDetector.getBeatPulse() * 0.4f);
        params.CustomParam4 = std::sin(m_cameraAngle) * 0.4f;
    }

    // 1. Check for Beat-Quantized Pending Transitions
    if (m_quantizeToBeat && m_hasPendingChanges) {
        // Trigger synchronized exactly on beat onset pulse
        if (beatDetector.getBeatPulse() > 0.75f && m_beatTransitionCooldown <= 0.0f) {
            m_activeTargetRecipe = m_pendingRecipe;

            params.ShapeType = m_pendingRecipe.shapeType;
            params.MaterialMode = m_pendingRecipe.materialMode;
            params.DuplicationMode = m_pendingRecipe.duplicationMode;
            params.BackgroundType = m_pendingRecipe.backgroundType;

            params.ShapeCrossfade = 0.0f;
            params.BloomIntensity = (std::min)(2.8f, params.BloomIntensity + 0.5f);
            m_hasPendingChanges = false;
            m_isInterpolating = true;
            m_beatTransitionCooldown = 0.25f;
        }
    }

    // 2. Smooth Parameter Interpolation towards m_activeTargetRecipe
    if (m_isInterpolating) {
        float t = (std::min)(1.0f, deltaTime * 3.5f);
        auto lerpVal = [](float current, float target, float rate) {
            return current + (target - current) * rate;
        };

        params.SkyLayerWeight = lerpVal(params.SkyLayerWeight, m_activeTargetRecipe.skyLayerWeight, t);
        params.WarpLayerWeight = lerpVal(params.WarpLayerWeight, m_activeTargetRecipe.warpLayerWeight, t);
        params.FloorLayerWeight = lerpVal(params.FloorLayerWeight, m_activeTargetRecipe.floorLayerWeight, t);
        params.SoundwaveLayerWeight = lerpVal(params.SoundwaveLayerWeight, m_activeTargetRecipe.soundwaveLayerWeight, t);
        params.HudLayerWeight = lerpVal(params.HudLayerWeight, m_activeTargetRecipe.hudLayerWeight, t);
        params.StageTrussWeight = lerpVal(params.StageTrussWeight, m_activeTargetRecipe.stageTrussWeight, t);

        params.LaserIntensity = lerpVal(params.LaserIntensity, m_activeTargetRecipe.laserIntensity, t);
        params.ParticleIntensity = lerpVal(params.ParticleIntensity, m_activeTargetRecipe.particleIntensity, t);
        params.FluidityAmount = lerpVal(params.FluidityAmount, m_activeTargetRecipe.fluidityAmount, t);
        params.VortexTwist = lerpVal(params.VortexTwist, m_activeTargetRecipe.vortexTwist, t);

        params.CustomParam0 = lerpVal(params.CustomParam0, m_activeTargetRecipe.customParam0, t);
        params.CustomParam1 = lerpVal(params.CustomParam1, m_activeTargetRecipe.customParam1, t);
        params.DuplicationParam0 = lerpVal(params.DuplicationParam0, m_activeTargetRecipe.duplicationParam0, t);
        params.DuplicationParam1 = lerpVal(params.DuplicationParam1, m_activeTargetRecipe.duplicationParam1, t);

        params.ColorPrimary.x = lerpVal(params.ColorPrimary.x, m_activeTargetRecipe.colorPrimary[0], t);
        params.ColorPrimary.y = lerpVal(params.ColorPrimary.y, m_activeTargetRecipe.colorPrimary[1], t);
        params.ColorPrimary.z = lerpVal(params.ColorPrimary.z, m_activeTargetRecipe.colorPrimary[2], t);
        params.ColorPrimary.w = lerpVal(params.ColorPrimary.w, m_activeTargetRecipe.colorPrimary[3], t);

        params.ColorSecondary.x = lerpVal(params.ColorSecondary.x, m_activeTargetRecipe.colorSecondary[0], t);
        params.ColorSecondary.y = lerpVal(params.ColorSecondary.y, m_activeTargetRecipe.colorSecondary[1], t);
        params.ColorSecondary.z = lerpVal(params.ColorSecondary.z, m_activeTargetRecipe.colorSecondary[2], t);
        params.ColorSecondary.w = lerpVal(params.ColorSecondary.w, m_activeTargetRecipe.colorSecondary[3], t);

        params.ColorAccent.x = lerpVal(params.ColorAccent.x, m_activeTargetRecipe.colorAccent[0], t);
        params.ColorAccent.y = lerpVal(params.ColorAccent.y, m_activeTargetRecipe.colorAccent[1], t);
        params.ColorAccent.z = lerpVal(params.ColorAccent.z, m_activeTargetRecipe.colorAccent[2], t);
        params.ColorAccent.w = lerpVal(params.ColorAccent.w, m_activeTargetRecipe.colorAccent[3], t);

        // Check if interpolation converged
        float maxDelta = 0.0f;
        auto trackDelta = [&](float curr, float target) {
            maxDelta = (std::max)(maxDelta, std::abs(curr - target));
        };
        trackDelta(params.SkyLayerWeight, m_activeTargetRecipe.skyLayerWeight);
        trackDelta(params.WarpLayerWeight, m_activeTargetRecipe.warpLayerWeight);
        trackDelta(params.FloorLayerWeight, m_activeTargetRecipe.floorLayerWeight);
        trackDelta(params.SoundwaveLayerWeight, m_activeTargetRecipe.soundwaveLayerWeight);
        trackDelta(params.HudLayerWeight, m_activeTargetRecipe.hudLayerWeight);
        trackDelta(params.StageTrussWeight, m_activeTargetRecipe.stageTrussWeight);
        trackDelta(params.LaserIntensity, m_activeTargetRecipe.laserIntensity);
        trackDelta(params.ParticleIntensity, m_activeTargetRecipe.particleIntensity);
        trackDelta(params.FluidityAmount, m_activeTargetRecipe.fluidityAmount);
        trackDelta(params.VortexTwist, m_activeTargetRecipe.vortexTwist);

        if (maxDelta < 0.005f) {
            m_isInterpolating = false;
        }
    }

    // 3. Energy-Aware Dynamic Layer Modulation (when Auto-Pilot is ON and not in transition)
    if (m_enabled && !m_isInterpolating) {
        if (m_smoothedEnergy > 0.55f) {
            // High Energy Drops: Boost Warp streaks, Lasers, and Particle Swarm
            float boostFactor = (m_smoothedEnergy - 0.55f) * 0.8f;
            if (m_activeTargetRecipe.laserIntensity > 0.1f) {
                params.LaserIntensity = (std::min)(2.0f, m_activeTargetRecipe.laserIntensity * (1.0f + boostFactor * 0.4f));
            }
            if (m_activeTargetRecipe.warpLayerWeight > 0.1f) {
                params.WarpLayerWeight = (std::min)(1.5f, m_activeTargetRecipe.warpLayerWeight * (1.0f + boostFactor * 0.35f));
            }
        } else if (m_smoothedEnergy < 0.35f) {
            // Low Energy Breakdowns: Emphasize clean Obsidian Floor and Deep Space Starfield
            float calmFactor = (0.35f - m_smoothedEnergy) / 0.35f;
            if (m_activeTargetRecipe.floorLayerWeight > 0.1f) {
                params.FloorLayerWeight = (std::min)(1.5f, m_activeTargetRecipe.floorLayerWeight * (1.0f + calmFactor * 0.15f));
            }
            if (m_activeTargetRecipe.skyLayerWeight > 0.1f) {
                params.SkyLayerWeight = (std::min)(1.5f, m_activeTargetRecipe.skyLayerWeight * (1.0f + calmFactor * 0.15f));
            }
            if (m_activeTargetRecipe.laserIntensity > 0.1f) {
                params.LaserIntensity = (std::max)(0.0f, m_activeTargetRecipe.laserIntensity * (1.0f - calmFactor * 0.3f));
            }
        }
    }

    // 4. Auto-Pilot Mode (Automatic phrase drop switching)
    if (m_enabled) {
        m_timeInCurrentScene += deltaTime;

        bool dropTriggered = beatDetector.getDropIntensity() > 0.82f && m_timeInCurrentScene > 12.0f && m_dropCooldown <= 0.0f;
        bool timeTriggered = m_timeInCurrentScene >= m_switchInterval;

        if (dropTriggered || timeTriggered) {
            triggerDropTransition(params);
        }
    }
}


} // namespace Visualizer::Control

