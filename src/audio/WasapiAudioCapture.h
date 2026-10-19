#pragma once

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <mmdeviceapi.h>
    #include <audioclient.h>
#endif

#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <span>
#include <cstdint>

namespace Visualizer::Audio {

struct AudioDeviceInfo {
    std::wstring id;
    std::wstring name;
    bool isLoopback = false;
};

class WasapiAudioCapture {
public:
    WasapiAudioCapture();
    ~WasapiAudioCapture();

    // Enumerate available loopback render outputs and line-in recording devices
    std::vector<AudioDeviceInfo> enumerateDevices();

    // Start audio capture (loopback or line-in)
    bool start(const std::wstring& deviceId = L"", bool isLoopback = true);
    void stop();

    bool isRunning() const { return m_isRunning.load(); }
    uint32_t getSampleRate() const { return m_sampleRate; }
    uint32_t getChannels() const { return m_channels; }

    // Read newest captured mono PCM float samples into output buffer
    size_t getLatestSamples(std::vector<float>& outSamples, size_t maxCount = 2048);

    // Thread-safe ringbuffer push used by platform audio callbacks (CoreAudio / ScreenCaptureKit)
    void pushMonoSamples(const float* samples, size_t count, uint32_t sampleRate = 48000);

private:
    void cleanup();

    std::atomic<bool> m_isRunning{false};
    std::thread m_captureThread;

#if defined(_WIN32)
    IMMDeviceEnumerator* m_deviceEnumerator = nullptr;
    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioCaptureClient* m_captureClient = nullptr;
    WAVEFORMATEX* m_waveFormat = nullptr;
#elif defined(__APPLE__)
    void* m_macAudioState = nullptr;
#endif

    uint32_t m_sampleRate = 48000;
    uint32_t m_channels = 2;

    std::mutex m_sampleMutex;
    std::vector<float> m_ringBuffer;
    size_t m_writeIndex = 0;
    size_t m_ringBufferSize = 16384;
};

} // namespace Visualizer::Audio
