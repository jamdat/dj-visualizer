#pragma once

#include <algorithm>
#include <cmath>

namespace Visualizer::Control {

// Director for the Space Voyage stage layout (src/shaders/SpaceVoyage.hlsl).
// Keeps a voyage clock that tours kDestinationCount destinations, kLegSeconds each, with a
// hyperspace jump over the last kWarpSeconds of every leg. (The tempo-locked beat phase the
// pulsar beams use comes from main.cpp, shared with the other layouts.) Header-only: it is tiny.
class SpaceVoyage {
public:
    static constexpr int kDestinationCount = 7;
    static constexpr float kLegSeconds = 15.0f;
    static constexpr float kWarpSeconds = 3.0f;

    // Start the tour over at the first destination (arriving out of hyperspace)
    void restart() { m_clock = 0.0f; }

    void update(float deltaTime) {
        const float tourSeconds = kLegSeconds * static_cast<float>(kDestinationCount);
        m_clock += deltaTime;
        if (m_clock >= tourSeconds) m_clock -= tourSeconds;
    }

    // Start the hyperspace jump to the next destination now (no-op if already jumping)
    void warpToNext() {
        float warpStart = std::floor(m_clock / kLegSeconds) * kLegSeconds + kLegSeconds - kWarpSeconds;
        if (m_clock < warpStart) m_clock = warpStart;
    }

    float getClock() const { return m_clock; }
    int getDestinationIndex() const { return static_cast<int>(m_clock / kLegSeconds) % kDestinationCount; }
    bool isWarping() const { return std::fmod(m_clock, kLegSeconds) >= kLegSeconds - kWarpSeconds; }
    float getSecondsToNextJump() const {
        return (std::max)(0.0f, kLegSeconds - kWarpSeconds - std::fmod(m_clock, kLegSeconds));
    }

    // Order must match the destinations in SpaceVoyage.hlsl
    const char* getDestinationName() const {
        static const char* kNames[kDestinationCount] = {
            "Ringed Giant", "Asteroid Belt", "Nebula Drift", "Pulsar", "Aurora World", "Spiral Galaxy", "Event Horizon"
        };
        return kNames[getDestinationIndex()];
    }

private:
    float m_clock = 0.0f;
};

} // namespace Visualizer::Control
