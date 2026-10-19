#if defined(__APPLE__)
#include "WasapiAudioCapture.h"
#include "../core/Logger.h"

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudio.h>
#import <AudioToolbox/AudioToolbox.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <CoreMedia/CoreMedia.h>

// =============================================================================
// MacAudioCapture.mm - Native Apple Silicon macOS Audio Engine
// 1. System Audio Loopback via ScreenCaptureKit (SCStream) on macOS 13.0+
// 2. External DJ Mixer / Audio Interface / Microphone Line-In via AVAudioEngine & CoreAudio
// =============================================================================

@interface DJSystemAudioStreamOutput : NSObject <SCStreamOutput, SCStreamDelegate>
@property (nonatomic, assign) Visualizer::Audio::WasapiAudioCapture* captureOwner;
@end

@implementation DJSystemAudioStreamOutput

- (void)stream:(SCStream *)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type API_AVAILABLE(macos(13.0)) {
    if (type != SCStreamOutputTypeAudio || !self.captureOwner || !CMSampleBufferDataIsReady(sampleBuffer)) {
        return;
    }

    CMFormatDescriptionRef formatDesc = CMSampleBufferGetFormatDescription(sampleBuffer);
    const AudioStreamBasicDescription* asbd = CMAudioFormatDescriptionGetStreamBasicDescription(formatDesc);
    if (!asbd) return;

    CMItemCount numFrames = CMSampleBufferGetNumSamples(sampleBuffer);
    if (numFrames <= 0) return;

    AudioBufferList* bufferList = nullptr;
    CMBlockBufferRef blockBuffer = nullptr;
    size_t bufferListSizeNeeded = 0;

    CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
        sampleBuffer,
        &bufferListSizeNeeded,
        nullptr,
        0,
        nullptr,
        nullptr,
        0,
        nullptr
    );

    if (bufferListSizeNeeded == 0) return;
    bufferList = (AudioBufferList*)malloc(bufferListSizeNeeded);
    if (!bufferList) return;

    OSStatus status = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
        sampleBuffer,
        nullptr,
        bufferList,
        bufferListSizeNeeded,
        kCFAllocatorDefault,
        kCFAllocatorDefault,
        kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
        &blockBuffer
    );

    if (status == noErr && bufferList->mNumberBuffers > 0) {
        uint32_t sampleRate = static_cast<uint32_t>(asbd->mSampleRate > 0 ? asbd->mSampleRate : 48000);
        std::vector<float> mono(static_cast<size_t>(numFrames), 0.0f);

        bool isNonInterleaved = (asbd->mFormatFlags & kAudioFormatFlagIsNonInterleaved) != 0;
        bool isFloat = (asbd->mFormatFlags & kAudioFormatFlagIsFloat) != 0;

        if (isFloat) {
            if (isNonInterleaved) {
                UInt32 numBufs = bufferList->mNumberBuffers;
                for (UInt32 b = 0; b < numBufs; ++b) {
                    const float* chData = static_cast<const float*>(bufferList->mBuffers[b].mData);
                    if (!chData) continue;
                    for (CMItemCount f = 0; f < numFrames; ++f) {
                        mono[f] += chData[f] / static_cast<float>(numBufs);
                    }
                }
            } else {
                const float* interleaved = static_cast<const float*>(bufferList->mBuffers[0].mData);
                UInt32 channels = bufferList->mBuffers[0].mNumberChannels;
                if (interleaved && channels > 0) {
                    for (CMItemCount f = 0; f < numFrames; ++f) {
                        float sum = 0.0f;
                        for (UInt32 c = 0; c < channels; ++c) {
                            sum += interleaved[f * channels + c];
                        }
                        mono[f] = sum / static_cast<float>(channels);
                    }
                }
            }
        }

        self.captureOwner->pushMonoSamples(mono.data(), mono.size(), sampleRate);
    }

    if (blockBuffer) CFRelease(blockBuffer);
    free(bufferList);
}

@end

namespace Visualizer::Audio {

struct MacAudioInternalState {
    AVAudioEngine* audioEngine = nil;
    id scStream = nil; // SCStream*
    DJSystemAudioStreamOutput* scOutput = nil;
    dispatch_queue_t audioQueue = nil;
};

static std::wstring CFStringToWide(CFStringRef cfStr) {
    if (!cfStr) return L"";
    char buf[512] = {};
    if (CFStringGetCString(cfStr, buf, sizeof(buf), kCFStringEncodingUTF8)) {
        std::wstring out;
        for (const char* p = buf; *p; ++p) {
            out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
        }
        return out;
    }
    return L"";
}

WasapiAudioCapture::WasapiAudioCapture() {
    m_ringBuffer.resize(m_ringBufferSize, 0.0f);
    m_macAudioState = new MacAudioInternalState();
}

WasapiAudioCapture::~WasapiAudioCapture() {
    stop();
    cleanup();
    if (m_macAudioState) {
        delete static_cast<MacAudioInternalState*>(m_macAudioState);
        m_macAudioState = nullptr;
    }
}

std::vector<AudioDeviceInfo> WasapiAudioCapture::enumerateDevices() {
    std::vector<AudioDeviceInfo> devices;

    // 1. Native macOS System Audio Loopback (ScreenCaptureKit)
    AudioDeviceInfo sysLoopback;
    sysLoopback.id = L"macos_system_loopback";
    sysLoopback.name = L"[System Audio Loopback] macOS Master Output";
    sysLoopback.isLoopback = true;
    devices.push_back(sysLoopback);

    // 2. Default Audio Input (DJ Mixer / Interface / Microphone)
    AudioDeviceInfo defaultInput;
    defaultInput.id = L"";
    defaultInput.name = L"[Input / DJ Mixer] Default macOS Audio Input";
    defaultInput.isLoopback = false;
    devices.push_back(defaultInput);

    // 3. Enumerate CoreAudio Input & Virtual Loopback Devices (e.g. Pioneer DJ, Scarlett, BlackHole)
    AudioObjectPropertyAddress propAddr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    UInt32 dataSize = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propAddr, 0, nullptr, &dataSize) == noErr && dataSize > 0) {
        UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);
        std::vector<AudioDeviceID> devIds(deviceCount);
        if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propAddr, 0, nullptr, &dataSize, devIds.data()) == noErr) {
            for (AudioDeviceID devId : devIds) {
                // Check if device has input channels
                AudioObjectPropertyAddress inStreamsAddr = {
                    kAudioDevicePropertyStreams,
                    kAudioDevicePropertyScopeInput,
                    kAudioObjectPropertyElementMain
                };
                UInt32 streamSize = 0;
                if (AudioObjectGetPropertyDataSize(devId, &inStreamsAddr, 0, nullptr, &streamSize) != noErr || streamSize == 0) {
                    continue;
                }

                CFStringRef nameRef = nullptr;
                UInt32 nameSize = sizeof(CFStringRef);
                AudioObjectPropertyAddress nameAddr = {
                    kAudioObjectPropertyName,
                    kAudioObjectPropertyScopeGlobal,
                    kAudioObjectPropertyElementMain
                };
                if (AudioObjectGetPropertyData(devId, &nameAddr, 0, nullptr, &nameSize, &nameRef) == noErr && nameRef) {
                    std::wstring devName = CFStringToWide(nameRef);
                    CFRelease(nameRef);
                    if (!devName.empty()) {
                        AudioDeviceInfo info;
                        info.id = std::to_wstring(devId);
                        info.name = L"[Audio Input] " + devName;
                        info.isLoopback = false;
                        devices.push_back(info);
                    }
                }
            }
        }
    }

    return devices;
}

static bool StartInputCaptureOnMac(WasapiAudioCapture* owner, MacAudioInternalState* state, const std::wstring& deviceId) {
    @autoreleasepool {
        state->audioEngine = [[AVAudioEngine alloc] init];
        AVAudioInputNode* inputNode = [state->audioEngine inputNode];
        if (!inputNode) {
            Log("[ERROR] macOS AVAudioInputNode unavailable");
            return false;
        }

        // If a specific CoreAudio AudioDeviceID was chosen, bind it to the input AudioUnit
        if (!deviceId.empty() && deviceId != L"macos_system_loopback") {
            try {
                AudioDeviceID targetDevId = static_cast<AudioDeviceID>(std::stoul(deviceId));
                AudioUnit au = [inputNode audioUnit];
                if (au) {
                    AudioUnitSetProperty(
                        au,
                        kAudioOutputUnitProperty_CurrentDevice,
                        kAudioUnitScope_Global,
                        0,
                        &targetDevId,
                        sizeof(targetDevId)
                    );
                }
            } catch (...) {}
        }

        AVAudioFormat* format = [inputNode inputFormatForBus:0];
        if (!format || format.channelCount == 0 || format.sampleRate <= 0) {
            Log("[ERROR] Invalid macOS input audio format");
            return false;
        }

        uint32_t sr = static_cast<uint32_t>(format.sampleRate);
        [inputNode installTapOnBus:0 bufferSize:1024 format:format block:^(AVAudioPCMBuffer* _Nonnull buffer, AVAudioTime* _Nonnull /*when*/) {
            AVAudioFrameCount frames = buffer.frameLength;
            AVAudioChannelCount chs = buffer.format.channelCount;
            if (frames == 0 || chs == 0 || !buffer.floatChannelData) return;

            std::vector<float> mono(frames, 0.0f);
            for (AVAudioChannelCount c = 0; c < chs; ++c) {
                const float* chPtr = buffer.floatChannelData[c];
                if (!chPtr) continue;
                for (AVAudioFrameCount f = 0; f < frames; ++f) {
                    mono[f] += chPtr[f] / static_cast<float>(chs);
                }
            }
            owner->pushMonoSamples(mono.data(), mono.size(), sr);
        }];

        NSError* err = nil;
        if (![state->audioEngine startAndReturnError:&err]) {
            Log("[ERROR] Failed to start AVAudioEngine input capture");
            return false;
        }
        Log("[INFO] macOS CoreAudio Input capture started (" + std::to_string(sr) + " Hz)");
        return true;
    }
}

bool WasapiAudioCapture::start(const std::wstring& deviceId, bool isLoopback) {
    stop();
    auto* state = static_cast<MacAudioInternalState*>(m_macAudioState);
    if (!state) return false;

    m_isRunning = true;

    // 1. Try macOS 13.0+ ScreenCaptureKit System Audio Loopback when isLoopback == true
    if (isLoopback) {
        if (@available(macOS 13.0, *)) {
            @autoreleasepool {
                state->audioQueue = dispatch_queue_create("com.djvisualizer.scaudio", DISPATCH_QUEUE_SERIAL);
                state->scOutput = [[DJSystemAudioStreamOutput alloc] init];
                state->scOutput.captureOwner = this;

                [SCShareableContent getShareableContentExcludingDesktopWindows:NO
                                                           onScreenWindowsOnly:YES
                                                             completionHandler:^(SCShareableContent* _Nullable shareableContent, NSError* _Nullable error) {
                    if (error || !shareableContent || shareableContent.displays.count == 0) {
                        Log("[WARN] ScreenCaptureKit shareable content unavailable, falling back to default audio input");
                        StartInputCaptureOnMac(this, state, L"");
                        return;
                    }

                    SCDisplay* primaryDisplay = shareableContent.displays.firstObject;
                    SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:primaryDisplay excludingWindows:@[]];

                    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
                    config.capturesAudio = YES;
                    config.excludesCurrentProcessAudio = NO;
                    config.sampleRate = 48000;
                    config.channelCount = 2;
                    // Minimize video overhead (we only need system audio)
                    config.width = 2;
                    config.height = 2;
                    config.minimumFrameInterval = CMTimeMake(1, 1);

                    SCStream* stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:state->scOutput];
                    NSError* addErr = nil;
                    [stream addStreamOutput:state->scOutput type:SCStreamOutputTypeAudio sampleHandlerQueue:state->audioQueue error:&addErr];
                    if (addErr) {
                        Log("[WARN] Failed to add SCStreamAudioOutput, falling back to default audio input");
                        StartInputCaptureOnMac(this, state, L"");
                        return;
                    }

                    state->scStream = stream;
                    [stream startCaptureWithCompletionHandler:^(NSError* _Nullable startErr) {
                        if (startErr) {
                            Log("[WARN] SCStream startCapture failed (permission needed), falling back to CoreAudio Input");
                            StartInputCaptureOnMac(this, state, L"");
                        } else {
                            Log("[INFO] macOS ScreenCaptureKit System Audio Loopback started (48000 Hz)");
                        }
                    }];
                }];
                return true;
            }
        }
    }

    // 2. CoreAudio Input / DJ Mixer Line-In Capture
    return StartInputCaptureOnMac(this, state, deviceId);
}

void WasapiAudioCapture::stop() {
    m_isRunning = false;
    cleanup();
}

void WasapiAudioCapture::cleanup() {
    auto* state = static_cast<MacAudioInternalState*>(m_macAudioState);
    if (!state) return;

    @autoreleasepool {
        if (state->audioEngine) {
            [[state->audioEngine inputNode] removeTapOnBus:0];
            [state->audioEngine stop];
            state->audioEngine = nil;
        }
        if (@available(macOS 13.0, *)) {
            if (state->scStream) {
                SCStream* stream = (SCStream*)state->scStream;
                [stream stopCaptureWithCompletionHandler:^(NSError* _Nullable) {}];
                state->scStream = nil;
            }
        }
        if (state->scOutput) {
            state->scOutput.captureOwner = nullptr;
            state->scOutput = nil;
        }
        state->audioQueue = nil;
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
