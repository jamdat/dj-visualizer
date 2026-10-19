#pragma once

#include "AudioAnalyzer.h"
#include <vector>
#include <deque>
#include <chrono>

namespace Visualizer::Audio {

class BeatDetector {
public:
    BeatDetector();

    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    float getSensitivity() const { return m_sensitivity; }

    void setDecaySpeed(float speed) { m_decaySpeed = speed; }

    // Process audio frame and update beat flags and envelopes
    void update(const AudioSpectrumBands& bands, float deltaTime);

    // Is current frame a beat / kick onset?
    bool isBeat() const { return m_isBeat; }

    // Normalized envelope value [0.0 .. 1.0] for pulsing animations
    float getBeatPulse() const { return m_beatPulse; }

    // Drop intensity [0.0 .. 1.0] when a massive drop hits
    float getDropIntensity() const { return m_dropIntensity; }

    // Estimated BPM (beats per minute)
    float getBpm() const { return m_estimatedBpm; }

    // Manual tap tempo
    void tapTempo();

private:
    float m_sensitivity = 1.35f;
    float m_decaySpeed = 6.0f; // envelope decay rate
    bool m_isBeat = false;
    float m_beatPulse = 0.0f;
    float m_dropIntensity = 0.0f;
    float m_estimatedBpm = 128.0f;

    float m_prevSubBass = 0.0f;
    float m_prevBass = 0.0f;
    float m_timeSinceLastBeat = 0.0f;
    float m_minBeatInterval = 0.22f; // ~270 BPM upper limit lock

    std::deque<float> m_fluxHistory;
    size_t m_historySize = 45; // ~0.75s history window

    std::deque<std::chrono::steady_clock::time_point> m_tapTimes;
    std::deque<float> m_beatIntervals;
};

} // namespace Visualizer::Audio
