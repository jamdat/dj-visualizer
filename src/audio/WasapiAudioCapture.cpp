#include "WasapiAudioCapture.h"
#include "../core/Logger.h"
#include <iostream>

#if defined(_WIN32)
#include <functiondiscoverykeys_devpkey.h>
#include <avrt.h>

#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "ole32.lib")

namespace Visualizer::Audio {

WasapiAudioCapture::WasapiAudioCapture() {
    m_ringBuffer.resize(m_ringBufferSize, 0.0f);
}

WasapiAudioCapture::~WasapiAudioCapture() {
    stop();
    cleanup();
}

std::vector<AudioDeviceInfo> WasapiAudioCapture::enumerateDevices() {
    Log("enumerateDevices started");
    std::vector<AudioDeviceInfo> devices;

    AudioDeviceInfo defaultLoopback;
    defaultLoopback.id = L"";
    defaultLoopback.name = L"[System Audio Loopback] Default Speakers";
    defaultLoopback.isLoopback = true;
    devices.push_back(defaultLoopback);

    AudioDeviceInfo defaultCapture;
    defaultCapture.id = L"";
    defaultCapture.name = L"[Input / Mixer] Default Microphone / Line-In";
    defaultCapture.isLoopback = false;
    devices.push_back(defaultCapture);

    Log("enumerateDevices completed (" + std::to_string(devices.size()) + " devices)");
    return devices;
}

bool WasapiAudioCapture::start(const std::wstring& deviceId, bool isLoopback) {
    stop();

    m_isRunning = true;
    m_captureThread = std::thread([this, deviceId, isLoopback]() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            m_isRunning = false;
            return;
        }

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                              __uuidof(IMMDeviceEnumerator), (void**)&m_deviceEnumerator);
        if (FAILED(hr) || !m_deviceEnumerator) {
            m_isRunning = false;
            CoUninitialize();
            return;
        }

        if (deviceId.empty()) {
            EDataFlow flow = isLoopback ? eRender : eCapture;
            hr = m_deviceEnumerator->GetDefaultAudioEndpoint(flow, eMultimedia, &m_device);
            if (FAILED(hr) || !m_device) {
                hr = m_deviceEnumerator->GetDefaultAudioEndpoint(flow, eConsole, &m_device);
            }
        } else {
            hr = m_deviceEnumerator->GetDevice(deviceId.c_str(), &m_device);
        }

        if (FAILED(hr) || !m_device) {
            cleanup();
            m_isRunning = false;
            CoUninitialize();
            return;
        }

        hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
        if (FAILED(hr) || !m_audioClient) {
            cleanup();
            m_isRunning = false;
            CoUninitialize();
            return;
        }

        hr = m_audioClient->GetMixFormat(&m_waveFormat);
        if (FAILED(hr) || !m_waveFormat) {
            cleanup();
            m_isRunning = false;
            CoUninitialize();
            return;
        }

        m_sampleRate = m_waveFormat->nSamplesPerSec;
        m_channels = m_waveFormat->nChannels;

        DWORD streamFlags = isLoopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
        REFERENCE_TIME bufferDuration = 1000000; // 100ms in 100ns units

        hr = m_audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            streamFlags,
            bufferDuration,
            0,
            m_waveFormat,
            nullptr
        );

        if (FAILED(hr)) {
            cleanup();
            m_isRunning = false;
            CoUninitialize();
            return;
        }

        hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_captureClient);
        if (FAILED(hr) || !m_captureClient) {
            cleanup();
            m_isRunning = false;
            CoUninitialize();
            return;
        }

        hr = m_audioClient->Start();
        if (FAILED(hr)) {
            cleanup();
            m_isRunning = false;
            CoUninitialize();
            return;
        }

        DWORD taskIndex = 0;
        HANDLE mmcssHandle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

        while (m_isRunning.load()) {
            Sleep(5);

            UINT32 packetLength = 0;
            hr = m_captureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) break;

            while (packetLength > 0) {
                BYTE* pData = nullptr;
                UINT32 numFramesAvailable = 0;
                DWORD flags = 0;

                hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
                if (FAILED(hr)) break;

                if (numFramesAvailable > 0) {
                    bool isSilent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;

                    std::lock_guard<std::mutex> lock(m_sampleMutex);
                    if (isSilent || !pData) {
                        for (UINT32 f = 0; f < numFramesAvailable; ++f) {
                            m_ringBuffer[m_writeIndex] = 0.0f;
                            m_writeIndex = (m_writeIndex + 1) % m_ringBufferSize;
                        }
                    } else {
                        bool isFloat = (m_waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
                                       (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                                        reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

                        int bitsPerSample = m_waveFormat->wBitsPerSample;
                        int channels = m_waveFormat->nChannels;

                        for (UINT32 f = 0; f < numFramesAvailable; ++f) {
                            float monoSample = 0.0f;

                            if (isFloat && bitsPerSample == 32) {
                                float* floatData = reinterpret_cast<float*>(pData + f * m_waveFormat->nBlockAlign);
                                for (int c = 0; c < channels; ++c) {
                                    monoSample += floatData[c];
                                }
                                monoSample /= static_cast<float>(channels);
                            } else if (bitsPerSample == 16) {
                                int16_t* intData = reinterpret_cast<int16_t*>(pData + f * m_waveFormat->nBlockAlign);
                                for (int c = 0; c < channels; ++c) {
                                    monoSample += static_cast<float>(intData[c]) / 32768.0f;
                                }
                                monoSample /= static_cast<float>(channels);
                            } else if (bitsPerSample == 24) {
                                uint8_t* p24 = pData + f * m_waveFormat->nBlockAlign;
                                for (int c = 0; c < channels; ++c) {
                                    int32_t val = (p24[c * 3 + 2] << 24) | (p24[c * 3 + 1] << 16) | (p24[c * 3] << 8);
                                    monoSample += static_cast<float>(val) / 2147483648.0f;
                                }
                                monoSample /= static_cast<float>(channels);
                            }

                            m_ringBuffer[m_writeIndex] = monoSample;
                            m_writeIndex = (m_writeIndex + 1) % m_ringBufferSize;
                        }
                    }
                }

                m_captureClient->ReleaseBuffer(numFramesAvailable);
                hr = m_captureClient->GetNextPacketSize(&packetLength);
                if (FAILED(hr)) break;
            }
        }

        if (mmcssHandle) AvRevertMmThreadCharacteristics(mmcssHandle);
        if (m_audioClient) m_audioClient->Stop();
        cleanup();
        CoUninitialize();
    });

    return true;
}

void WasapiAudioCapture::stop() {
    m_isRunning = false;
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
}

void WasapiAudioCapture::cleanup() {
    if (m_waveFormat) {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
    }
    if (m_captureClient) {
        m_captureClient->Release();
        m_captureClient = nullptr;
    }
    if (m_audioClient) {
        m_audioClient->Release();
        m_audioClient = nullptr;
    }
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    if (m_deviceEnumerator) {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }
}

void WasapiAudioCapture::pushMonoSamples(const float* samples, size_t count, uint32_t sampleRate) {
    if (!samples || count == 0) return;
    std::lock_guard<std::mutex> lock(m_sampleMutex);
    m_sampleRate = sampleRate;
    for (size_t i = 0; i < count; ++i) {
        m_ringBuffer[m_writeIndex] = samples[i];
        m_writeIndex = (m_writeIndex + 1) % m_ringBufferSize;
    }
}

size_t WasapiAudioCapture::getLatestSamples(std::vector<float>& outSamples, size_t maxCount) {
    std::lock_guard<std::mutex> lock(m_sampleMutex);
    outSamples.resize(maxCount);
    size_t startPos = (m_writeIndex + m_ringBufferSize - maxCount) % m_ringBufferSize;

    for (size_t i = 0; i < maxCount; ++i) {
        outSamples[i] = m_ringBuffer[(startPos + i) % m_ringBufferSize];
    }
    return maxCount;
}

} // namespace Visualizer::Audio
#endif
