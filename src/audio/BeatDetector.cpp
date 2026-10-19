#include "BeatDetector.h"
#include <numeric>
#include <cmath>
#include <algorithm>

namespace Visualizer::Audio {

BeatDetector::BeatDetector() {
    m_fluxHistory.resize(m_historySize, 0.05f);
}

void BeatDetector::update(const AudioSpectrumBands& bands, float deltaTime) {
    m_timeSinceLastBeat += deltaTime;
    m_isBeat = false;

    // 1. Calculate weighted low-frequency spectral flux
    float subFlux = (std::max)(0.0f, bands.subBass - m_prevSubBass);
    float bassFlux = (std::max)(0.0f, bands.bass - m_prevBass);
    float instantFlux = subFlux * 1.6f + bassFlux * 1.2f;

    m_prevSubBass = bands.subBass;
    m_prevBass = bands.bass;

    // 2. Running statistical threshold (Mean + Variance * Sensitivity)
    m_fluxHistory.push_back(instantFlux);
    if (m_fluxHistory.size() > m_historySize) {
        m_fluxHistory.pop_front();
    }

    float mean = std::accumulate(m_fluxHistory.begin(), m_fluxHistory.end(), 0.0f) / static_cast<float>(m_fluxHistory.size());
    float variance = 0.0f;
    for (float f : m_fluxHistory) {
        float diff = f - mean;
        variance += diff * diff;
    }
    variance = std::sqrt(variance / static_cast<float>(m_fluxHistory.size()));

    float threshold = mean + variance * m_sensitivity;

    // 3. Peak detection trigger
    if (instantFlux > threshold && instantFlux > 0.08f && m_timeSinceLastBeat >= m_minBeatInterval) {
        m_isBeat = true;
        m_beatPulse = 1.0f;

        // Track BPM interval
        if (m_timeSinceLastBeat < 2.0f && m_timeSinceLastBeat > 0.25f) {
            m_beatIntervals.push_back(m_timeSinceLastBeat);
            if (m_beatIntervals.size() > 8) m_beatIntervals.pop_front();

            float avgInterval = std::accumulate(m_beatIntervals.begin(), m_beatIntervals.end(), 0.0f) / static_cast<float>(m_beatIntervals.size());
            if (avgInterval > 0.001f) {
                float measuredBpm = 60.0f / avgInterval;
                m_estimatedBpm = m_estimatedBpm * 0.85f + measuredBpm * 0.15f;
            }
        }

        // 4. Drop Detection: high low-end flux when coming out of a quiet/breakdown section
        if (instantFlux > mean * 2.8f && bands.subBass > 0.4f) {
            m_dropIntensity = 1.0f;
        }

        m_timeSinceLastBeat = 0.0f;
    }

    // 5. Exponential envelope decays
    m_beatPulse = (std::max)(0.0f, m_beatPulse - deltaTime * m_decaySpeed);
    m_dropIntensity = (std::max)(0.0f, m_dropIntensity - deltaTime * 1.5f);
}

void BeatDetector::tapTempo() {
    auto now = std::chrono::steady_clock::now();
    m_tapTimes.push_back(now);
    if (m_tapTimes.size() > 6) m_tapTimes.pop_front();

    if (m_tapTimes.size() >= 2) {
        float totalSec = 0.0f;
        for (size_t i = 1; i < m_tapTimes.size(); ++i) {
            std::chrono::duration<float> diff = m_tapTimes[i] - m_tapTimes[i - 1];
            totalSec += diff.count();
        }
        float avgSec = totalSec / static_cast<float>(m_tapTimes.size() - 1);
        if (avgSec > 0.2f && avgSec < 2.0f) {
            m_estimatedBpm = 60.0f / avgSec;
        }
    }
}

} // namespace Visualizer::Audio
