#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <span>

namespace Visualizer::DSP {

class FastFourierTransform {
public:
    enum class WindowType {
        Rectangular,
        Hann,
        Hamming,
        BlackmanHarris
    };

    explicit FastFourierTransform(size_t fftSize = 1024)
        : m_fftSize(fftSize), m_log2Size(static_cast<size_t>(std::log2(fftSize))) {
        // Precompute twiddle factors and bit-reversal tables
        m_twiddles.resize(m_fftSize / 2);
        for (size_t i = 0; i < m_fftSize / 2; ++i) {
            double angle = -2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(m_fftSize);
            m_twiddles[i] = std::complex<float>(
                static_cast<float>(std::cos(angle)),
                static_cast<float>(std::sin(angle))
            );
        }

        m_bitReversedIndices.resize(m_fftSize);
        for (size_t i = 0; i < m_fftSize; ++i) {
            m_bitReversedIndices[i] = reverseBits(i, m_log2Size);
        }

        generateWindow(WindowType::Hann);
    }

    void setWindowType(WindowType type) {
        generateWindow(type);
    }

    size_t getFFTSize() const { return m_fftSize; }
    size_t getSpectrumBins() const { return m_fftSize / 2; }

    // Execute forward complex-to-complex or real-to-magnitude FFT
    void process(std::span<const float> inputSamples, std::vector<float>& outMagnitudes) {
        if (inputSamples.empty()) return;

        outMagnitudes.resize(m_fftSize / 2);
        std::vector<std::complex<float>> buffer(m_fftSize);

        size_t copyCount = (std::min)(inputSamples.size(), m_fftSize);
        for (size_t i = 0; i < copyCount; ++i) {
            float windowed = inputSamples[i] * m_window[i];
            buffer[m_bitReversedIndices[i]] = std::complex<float>(windowed, 0.0f);
        }
        for (size_t i = copyCount; i < m_fftSize; ++i) {
            buffer[m_bitReversedIndices[i]] = std::complex<float>(0.0f, 0.0f);
        }

        // Cooley-Tukey Radix-2 decimation in time
        for (size_t len = 2; len <= m_fftSize; len <<= 1) {
            size_t halfLen = len >> 1;
            size_t step = m_fftSize / len;
            for (size_t i = 0; i < m_fftSize; i += len) {
                for (size_t j = 0; j < halfLen; ++j) {
                    std::complex<float> u = buffer[i + j];
                    std::complex<float> v = buffer[i + j + halfLen] * m_twiddles[j * step];
                    buffer[i + j] = u + v;
                    buffer[i + j + halfLen] = u - v;
                }
            }
        }

        // Extract normalized magnitudes for bins 0 .. (N/2 - 1)
        float normFactor = 2.0f / static_cast<float>(m_fftSize);
        for (size_t i = 0; i < m_fftSize / 2; ++i) {
            float mag = std::abs(buffer[i]) * normFactor;
            outMagnitudes[i] = mag;
        }
    }

private:
    static size_t reverseBits(size_t index, size_t bits) {
        size_t result = 0;
        for (size_t i = 0; i < bits; ++i) {
            if ((index >> i) & 1) {
                result |= (1ULL << (bits - 1 - i));
            }
        }
        return result;
    }

    void generateWindow(WindowType type) {
        m_window.resize(m_fftSize);
        double n = static_cast<double>(m_fftSize);

        for (size_t i = 0; i < m_fftSize; ++i) {
            double x = static_cast<double>(i);
            switch (type) {
            case WindowType::Hann:
                m_window[i] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * x / (n - 1.0))));
                break;
            case WindowType::Hamming:
                m_window[i] = static_cast<float>(0.54 - 0.46 * std::cos(2.0 * std::numbers::pi * x / (n - 1.0)));
                break;
            case WindowType::BlackmanHarris:
                m_window[i] = static_cast<float>(
                    0.35875 
                    - 0.48829 * std::cos(2.0 * std::numbers::pi * x / (n - 1.0))
                    + 0.14128 * std::cos(4.0 * std::numbers::pi * x / (n - 1.0))
                    - 0.01168 * std::cos(6.0 * std::numbers::pi * x / (n - 1.0))
                );
                break;
            case WindowType::Rectangular:
            default:
                m_window[i] = 1.0f;
                break;
            }
        }
    }

    size_t m_fftSize;
    size_t m_log2Size;
    std::vector<std::complex<float>> m_twiddles;
    std::vector<size_t> m_bitReversedIndices;
    std::vector<float> m_window;
};

} // namespace Visualizer::DSP
