#include "AudioAnalyzer.h"
#include <cmath>
#include <algorithm>

namespace Visualizer::Audio {

AudioAnalyzer::AudioAnalyzer(size_t fftSize, uint32_t sampleRate)
    : m_fftSize(fftSize), m_sampleRate(sampleRate), m_fft(fftSize) {
    m_rawMagnitudes.resize(m_fftSize / 2, 0.0f);
    m_smoothedSpectrum.resize(512, 0.0f);
    m_waveform.resize(512, 0.0f);
}

void AudioAnalyzer::setSampleRate(uint32_t sampleRate) {
    m_sampleRate = sampleRate;
}

void AudioAnalyzer::setSmoothing(float attack, float decay) {
    m_attackRate = std::clamp(attack, 0.01f, 1.0f);
    m_decayRate = std::clamp(decay, 0.001f, 0.99f);
}

size_t AudioAnalyzer::freqToBin(float freqHz) const {
    float binWidth = static_cast<float>(m_sampleRate) / static_cast<float>(m_fftSize);
    size_t bin = static_cast<size_t>(freqHz / binWidth);
    return (std::min)(bin, m_fftSize / 2 - 1);
}

void AudioAnalyzer::process(std::span<const float> samples, float deltaTime) {
    if (samples.empty()) return;

    // 1. Extract and normalize waveform for texture row 1
    size_t waveSamples = (std::min)(samples.size(), static_cast<size_t>(512));
    size_t step = samples.size() / 512;
    if (step == 0) step = 1;

    float rmsSum = 0.0f;
    for (size_t i = 0; i < 512; ++i) {
        size_t srcIdx = (i * step) % samples.size();
        float s = samples[srcIdx] * m_gain;
        m_waveform[i] = std::clamp(s, -1.0f, 1.0f);
        rmsSum += s * s;
    }
    m_bands.overallRMS = std::sqrt(rmsSum / 512.0f);

    // 2. Perform FFT
    m_fft.process(samples, m_rawMagnitudes);

    // 3. Resample and smooth into 512 logarithmic/perceptual spectrum bins
    size_t numBins = m_fftSize / 2;
    for (size_t i = 0; i < 512; ++i) {
        // Logarithmic frequency mapping from ~20Hz to ~18kHz
        float normIdx = static_cast<float>(i) / 511.0f;
        float logFactor = std::pow(normIdx, 1.8f); // slight curve for bass expansion
        size_t binIdx = static_cast<size_t>(logFactor * static_cast<float>(numBins - 1));
        binIdx = (std::min)(binIdx, numBins - 1);

        // Pre-emphasis high frequency boost (compensates for 1/f audio rolloff)
        float boost = 1.0f + 2.5f * normIdx;
        float targetMag = m_rawMagnitudes[binIdx] * m_gain * boost;

        // Non-linear amplitude scaling (compression for visual punch)
        targetMag = std::sqrt(targetMag);
        targetMag = std::clamp(targetMag, 0.0f, 1.5f);

        // Attack / Decay exponential smoothing
        float current = m_smoothedSpectrum[i];
        if (targetMag > current) {
            m_smoothedSpectrum[i] = current + (targetMag - current) * m_attackRate;
        } else {
            m_smoothedSpectrum[i] = current + (targetMag - current) * m_decayRate;
        }
    }

    // 4. Compute 5 frequency bands
    computeBands();

    // 5. Update energy accumulator for continuous shader time drive
    float totalBandEnergy = (m_bands.subBass * 1.5f + m_bands.bass * 1.2f + m_bands.lowMid + m_bands.highMid * 0.8f + m_bands.treble * 0.6f);
    m_energyAccumulator += (0.5f + totalBandEnergy * 2.0f) * deltaTime;
}

void AudioAnalyzer::computeBands() {
    size_t binSubStart = freqToBin(20.0f);
    size_t binSubEnd   = freqToBin(60.0f);
    size_t binBassEnd  = freqToBin(250.0f);
    size_t binLowMidEnd= freqToBin(500.0f);
    size_t binHiMidEnd = freqToBin(2000.0f);
    size_t binTrebEnd  = freqToBin(16000.0f);

    auto averageRange = [this](size_t start, size_t end) -> float {
        if (start >= end) end = start + 1;
        end = (std::min)(end, m_rawMagnitudes.size());
        float sum = 0.0f;
        for (size_t i = start; i < end; ++i) {
            sum += m_rawMagnitudes[i];
        }
        return sum / static_cast<float>(end - start);
    };

    float rawSub = averageRange(binSubStart, binSubEnd) * m_gain * 3.5f;
    float rawBass = averageRange(binSubEnd, binBassEnd) * m_gain * 3.0f;
    float rawLowMid = averageRange(binBassEnd, binLowMidEnd) * m_gain * 2.2f;
    float rawHiMid = averageRange(binLowMidEnd, binHiMidEnd) * m_gain * 2.8f;
    float rawTreb = averageRange(binHiMidEnd, binTrebEnd) * m_gain * 4.5f;

    auto smoothBand = [this](float target, float current) -> float {
        target = (std::min)(target, 2.0f);
        if (target > current) {
            return current + (target - current) * m_attackRate;
        } else {
            return current + (target - current) * m_decayRate;
        }
    };

    m_bands.subBass = smoothBand(rawSub, m_bands.subBass);
    m_bands.bass    = smoothBand(rawBass, m_bands.bass);
    m_bands.lowMid  = smoothBand(rawLowMid, m_bands.lowMid);
    m_bands.highMid = smoothBand(rawHiMid, m_bands.highMid);
    m_bands.treble  = smoothBand(rawTreb, m_bands.treble);
}

} // namespace Visualizer::Audio
