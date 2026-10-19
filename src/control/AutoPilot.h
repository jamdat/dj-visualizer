#pragma once

#include "../graphics/ConstantBuffers.h"
#include "../audio/BeatDetector.h"
#include "PresetManager.h"
#include <string>
#include <vector>

namespace Visualizer::Control {

class AutoPilot {
public:
    AutoPilot();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    void setSwitchInterval(float seconds) { m_switchInterval = seconds; }
    float getSwitchInterval() const { return m_switchInterval; }

    // Quantize to Next Beat Toggle & Pending Queue
    void setQuantizeToBeat(bool quantize) { m_quantizeToBeat = quantize; }
    bool isQuantizeToBeat() const { return m_quantizeToBeat; }

    bool hasPendingChanges() const { return m_hasPendingChanges; }
    const VisualizerPreset& getPendingRecipe() const { return m_pendingRecipe; }
    
    // Stage a recipe to be applied on the next beat (when Quantize is ON)
    void stageRecipe(const VisualizerPreset& recipe);
    
    // Trigger instant transition (Spacebar or Manual Drop)
    void triggerDropTransition(Graphics::AudioVisualizerParams& params);

    // Re-roll upcoming drop recipe
    void rollNextQueuedRecipe();
    const VisualizerPreset& getQueuedRecipe() const { return m_queuedRecipe; }

    // Progress towards next automatic drop (0.0 to 1.0)
    float getDropProgress() const;

    // Update auto-pilot and check for beat-quantized transitions
    void update(
        float deltaTime,
        const Audio::BeatDetector& beatDetector,
        Graphics::AudioVisualizerParams& params
    );

    bool isInterpolating() const { return m_isInterpolating; }
    float getSmoothedEnergy() const { return m_smoothedEnergy; }

private:
    bool m_enabled = false;
    bool m_quantizeToBeat = false; // When false: instant changes. When true: queues to next beat
    bool m_hasPendingChanges = false;
    bool m_isInterpolating = false;
    float m_smoothedEnergy = 0.0f;
    float m_switchInterval = 25.0f;
    float m_timeInCurrentScene = 0.0f;
    float m_cameraAngle = 0.0f;
    float m_dropCooldown = 0.0f;
    float m_beatTransitionCooldown = 0.0f;

    VisualizerPreset m_activeTargetRecipe;
    VisualizerPreset m_pendingRecipe;
    VisualizerPreset m_queuedRecipe;
};

} // namespace Visualizer::Control


