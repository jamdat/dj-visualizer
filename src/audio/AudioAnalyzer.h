#pragma once

#include "../../third_party/fft/FastFourierTransform.h"
#include <vector>
#include <array>
#include <span>

namespace Visualizer::Audio {

struct AudioSpectrumBands {
    float subBass = 0.0f;   // 20 - 60 Hz
    float bass = 0.0f;      // 60 - 250 Hz (Kick drum punch)
    float lowMid = 0.0f;    // 250 - 500 Hz (Basslines / synths)
    float highMid = 0.0f;   // 500 - 2000 Hz (Vocals / leads)
    float treble = 0.0f;    // 2000 - 16000 Hz (Hi-hats / cymbals)
    float overallRMS = 0.0f;// Overall volume RMS
};

class AudioAnalyzer {
public:
    explicit AudioAnalyzer(size_t fftSize = 1024, uint32_t sampleRate = 48000);

    void setSampleRate(uint32_t sampleRate);
    void setSmoothing(float attack, float decay);
    void setGain(float gain) { m_gain = gain; }
    float getGain() const { return m_gain; }

    // Ingest latest PCM samples and update spectrum & bands
    void process(std::span<const float> samples, float deltaTime);

    // Get smoothed 5-band frequency values
    const AudioSpectrumBands& getBands() const { return m_bands; }

    // Get continuous energy accumulator for shader time warping
    float getEnergyAccumulator() const { return m_energyAccumulator; }

    // Get 512-bin smoothed spectrum for GPU texture (Row 0)
    const std::vector<float>& getSpectrum() const { return m_smoothedSpectrum; }

    // Get 512-sample waveform for GPU texture (Row 1)
    const std::vector<float>& getWaveform() const { return m_waveform; }

    // Get raw unsmoothed FFT magnitudes
    const std::vector<float>& getRawMagnitudes() const { return m_rawMagnitudes; }

private:
    void computeBands();
    size_t freqToBin(float freqHz) const;

    size_t m_fftSize = 1024;
    uint32_t m_sampleRate = 48000;
    float m_gain = 1.8f;
    float m_attackRate = 0.75f;
    float m_decayRate = 0.15f;

    DSP::FastFourierTransform m_fft;

    std::vector<float> m_rawMagnitudes;
    std::vector<float> m_smoothedSpectrum;
    std::vector<float> m_waveform;

    AudioSpectrumBands m_bands;
    float m_energyAccumulator = 0.0f;
};

} // namespace Visualizer::Audio
