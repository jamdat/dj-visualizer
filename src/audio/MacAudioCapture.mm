#if defined(__APPLE__)
#include "WasapiAudioCapture.h"
#include "../core/Logger.h"

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreAudio/AudioHardwareTapping.h>
#import <CoreAudio/CATapDescription.h>
#import <AudioToolbox/AudioToolbox.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <CoreMedia/CoreMedia.h>

#include <algorithm>
#include <atomic>
#include <cmath>

// =============================================================================
// MacAudioCapture.mm - Native Apple Silicon macOS Audio Engine
// 1. System Audio Loopback via Core Audio process tap on macOS 14.2+
//    (needs only "System Audio Recording Only" permission, lowest latency)
// 2. System Audio Loopback via ScreenCaptureKit (SCStream) on macOS 13.x
// 3. External DJ Mixer / Audio Interface / Microphone Line-In via AVAudioEngine & CoreAudio
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

// Shared with the Core Audio tap IOProc (realtime thread); everything it needs is allocated up front
struct MacSystemTapContext {
    WasapiAudioCapture* owner = nullptr;
    uint32_t sampleRate = 48000;
    UInt32 firstTapBuffer = 0; // aggregate input buffers before this index come from the clock (output) device
    std::vector<float> mono;
    std::atomic<bool> loggedFirstAudio{false};
    std::atomic<bool> loggedSilence{false};
    uint64_t silentFrames = 0;
};

struct MacAudioInternalState {
    AVAudioEngine* audioEngine = nil;
    id scStream = nil; // SCStream*
    DJSystemAudioStreamOutput* scOutput = nil;
    dispatch_queue_t audioQueue = nil;

    // Core Audio process tap (macOS 14.2+)
    AudioObjectID tapId = kAudioObjectUnknown;
    AudioObjectID aggregateId = kAudioObjectUnknown;
    AudioDeviceIOProcID tapProcId = nullptr;
    MacSystemTapContext* tapContext = nullptr;
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

    // 1. Native macOS System Audio Loopback (Core Audio process tap, ScreenCaptureKit on macOS 13.x)
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
        // AVAudioNode raises NSException (not an NSError) on format mismatches or missing mic access;
        // uncaught it aborts the app, especially when this runs inside a ScreenCaptureKit completion handler
        @try {
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
        } @catch (NSException* ex) {
            Log(std::string("[ERROR] macOS input tap install failed: ") + (ex.name.UTF8String ?: "NSException") +
                " - " + (ex.reason.UTF8String ?: "no reason"));
            state->audioEngine = nil;
            return false;
        }

        NSError* err = nil;
        if (![state->audioEngine startAndReturnError:&err]) {
            Log(std::string("[ERROR] Failed to start AVAudioEngine input capture: ") +
                (err.localizedDescription.UTF8String ?: "unknown error"));
            [inputNode removeTapOnBus:0];
            state->audioEngine = nil;
            return false;
        }
        Log("[INFO] macOS CoreAudio Input capture started (" + std::to_string(sr) + " Hz)");
        return true;
    }
}

// Runs on the HAL realtime IO thread: no allocation, no locks besides the ring buffer mutex
static OSStatus SystemTapIOProc(AudioObjectID /*device*/, const AudioTimeStamp* /*now*/, const AudioBufferList* inInputData,
                                const AudioTimeStamp* /*inputTime*/, AudioBufferList* /*outOutputData*/,
                                const AudioTimeStamp* /*outputTime*/, void* clientData) {
    auto* ctx = static_cast<MacSystemTapContext*>(clientData);
    if (!ctx || !ctx->owner || !inInputData || inInputData->mNumberBuffers == 0) return noErr;

    UInt32 first = ctx->firstTapBuffer < inInputData->mNumberBuffers ? ctx->firstTapBuffer : 0;
    const AudioBuffer& lead = inInputData->mBuffers[first];
    UInt32 leadChannels = lead.mNumberChannels > 0 ? lead.mNumberChannels : 1;
    size_t frames = std::min<size_t>(lead.mDataByteSize / (sizeof(float) * leadChannels), ctx->mono.size());
    if (frames == 0) return noErr;

    float* mono = ctx->mono.data();
    std::fill_n(mono, frames, 0.0f);
    UInt32 totalChannels = 0;
    for (UInt32 b = first; b < inInputData->mNumberBuffers; ++b) {
        const AudioBuffer& buf = inInputData->mBuffers[b];
        const float* data = static_cast<const float*>(buf.mData);
        UInt32 chs = buf.mNumberChannels;
        if (!data || chs == 0) continue;
        size_t bufFrames = std::min<size_t>(frames, buf.mDataByteSize / (sizeof(float) * chs));
        for (size_t f = 0; f < bufFrames; ++f) {
            for (UInt32 c = 0; c < chs; ++c) {
                mono[f] += data[f * chs + c];
            }
        }
        totalChannels += chs;
    }
    if (totalChannels == 0) return noErr;

    const float scale = 1.0f / static_cast<float>(totalChannels);
    float peak = 0.0f;
    for (size_t f = 0; f < frames; ++f) {
        mono[f] *= scale;
        peak = std::max(peak, std::fabs(mono[f]));
    }
    ctx->owner->pushMonoSamples(mono, frames, ctx->sampleRate);

    // One-shot diagnostics, logged off the realtime thread. A denied "System Audio Recording Only"
    // permission fails no API call: the tap just delivers silence
    if (!ctx->loggedFirstAudio.load(std::memory_order_relaxed)) {
        if (peak > 1e-5f) {
            ctx->loggedFirstAudio.store(true);
            dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
                Log("[INFO] macOS system audio tap is receiving audio");
            });
        } else if (!ctx->loggedSilence.load(std::memory_order_relaxed)) {
            ctx->silentFrames += frames;
            if (ctx->silentFrames > static_cast<uint64_t>(ctx->sampleRate) * 10) {
                ctx->loggedSilence.store(true);
                dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
                    Log("[WARN] macOS system audio tap silent for 10 s: nothing is playing, or System Audio Recording Only "
                        "is off for DJVisualizer (System Settings > Privacy & Security > Screen & System Audio Recording)");
                });
            }
        }
    }
    return noErr;
}

static NSString* CopyDeviceUID(AudioObjectID deviceId) {
    CFStringRef uidRef = nullptr;
    UInt32 size = sizeof(uidRef);
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    if (AudioObjectGetPropertyData(deviceId, &addr, 0, nullptr, &size, &uidRef) != noErr || !uidRef) return nil;
    return (__bridge_transfer NSString*)uidRef;
}

// Number of input buffers a device contributes to an IOProc's AudioBufferList (one per input stream)
static UInt32 InputBufferCount(AudioObjectID deviceId) {
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyStreamConfiguration,
        kAudioObjectPropertyScopeInput,
        kAudioObjectPropertyElementMain
    };
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(deviceId, &addr, 0, nullptr, &size) != noErr || size < sizeof(UInt32)) return 0;
    std::vector<uint8_t> storage(size);
    auto* list = reinterpret_cast<AudioBufferList*>(storage.data());
    if (AudioObjectGetPropertyData(deviceId, &addr, 0, nullptr, &size, list) != noErr) return 0;
    return list->mNumberBuffers;
}

static void StopSystemTapOnMac(MacAudioInternalState* state) {
    if (state->aggregateId != kAudioObjectUnknown) {
        if (state->tapProcId) {
            AudioDeviceStop(state->aggregateId, state->tapProcId);
            AudioDeviceDestroyIOProcID(state->aggregateId, state->tapProcId);
            state->tapProcId = nullptr;
        }
        AudioHardwareDestroyAggregateDevice(state->aggregateId);
        state->aggregateId = kAudioObjectUnknown;
    }
    if (state->tapId != kAudioObjectUnknown) {
        if (@available(macOS 14.2, *)) {
            AudioHardwareDestroyProcessTap(state->tapId);
        }
        state->tapId = kAudioObjectUnknown;
    }
    delete state->tapContext;
    state->tapContext = nullptr;
}

// System audio via a Core Audio process tap: a global stereo mixdown of every app's output,
// read through a private aggregate device. Needs only the "System Audio Recording Only" permission
// (NSAudioCaptureUsageDescription); on failure the caller runs StopSystemTapOnMac to release partial state.
API_AVAILABLE(macos(14.2))
static bool StartSystemTapOnMac(WasapiAudioCapture* owner, MacAudioInternalState* state) {
    @autoreleasepool {
        CATapDescription* tapDesc = [[CATapDescription alloc] initStereoGlobalTapButExcludeProcesses:@[]];
        tapDesc.name = @"DJVisualizer System Audio";
        tapDesc.privateTap = YES;
        tapDesc.muteBehavior = CATapUnmuted;

        OSStatus status = AudioHardwareCreateProcessTap(tapDesc, &state->tapId);
        if (status != noErr || state->tapId == kAudioObjectUnknown) {
            state->tapId = kAudioObjectUnknown;
            Log("[WARN] AudioHardwareCreateProcessTap failed (OSStatus " + std::to_string(status) + ")");
            return false;
        }

        AudioStreamBasicDescription tapFormat = {};
        UInt32 size = sizeof(tapFormat);
        AudioObjectPropertyAddress fmtAddr = {
            kAudioTapPropertyFormat,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        status = AudioObjectGetPropertyData(state->tapId, &fmtAddr, 0, nullptr, &size, &tapFormat);
        if (status != noErr || !(tapFormat.mFormatFlags & kAudioFormatFlagIsFloat) || tapFormat.mBitsPerChannel != 32) {
            Log("[WARN] System audio tap format is not 32-bit float (OSStatus " + std::to_string(status) + ")");
            return false;
        }

        // The default output device clocks the aggregate, as in Apple's process tap sample
        AudioObjectID outputId = kAudioObjectUnknown;
        size = sizeof(outputId);
        AudioObjectPropertyAddress outAddr = {
            kAudioHardwarePropertyDefaultOutputDevice,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        AudioObjectGetPropertyData(kAudioObjectSystemObject, &outAddr, 0, nullptr, &size, &outputId);
        NSString* outputUID = CopyDeviceUID(outputId);
        if (!outputUID) {
            Log("[WARN] No default macOS output device to clock the system audio tap");
            return false;
        }

        NSString* tapUID = tapDesc.UUID.UUIDString;
        CFStringRef tapUIDRef = nullptr;
        size = sizeof(tapUIDRef);
        AudioObjectPropertyAddress tapUIDAddr = {
            kAudioTapPropertyUID,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        if (AudioObjectGetPropertyData(state->tapId, &tapUIDAddr, 0, nullptr, &size, &tapUIDRef) == noErr && tapUIDRef) {
            tapUID = (__bridge_transfer NSString*)tapUIDRef;
        }

        NSDictionary* aggregateDesc = @{
            @kAudioAggregateDeviceNameKey: @"DJVisualizer System Tap",
            @kAudioAggregateDeviceUIDKey: [@"com.djvisualizer.systemtap." stringByAppendingString:[NSUUID UUID].UUIDString],
            @kAudioAggregateDeviceMainSubDeviceKey: outputUID,
            @kAudioAggregateDeviceIsPrivateKey: @YES,
            @kAudioAggregateDeviceIsStackedKey: @NO,
            @kAudioAggregateDeviceTapAutoStartKey: @NO,
            @kAudioAggregateDeviceSubDeviceListKey: @[ @{ @kAudioSubDeviceUIDKey: outputUID } ],
            @kAudioAggregateDeviceTapListKey: @[ @{
                @kAudioSubTapDriftCompensationKey: @YES,
                @kAudioSubTapUIDKey: tapUID
            } ],
        };
        status = AudioHardwareCreateAggregateDevice((__bridge CFDictionaryRef)aggregateDesc, &state->aggregateId);
        if (status != noErr || state->aggregateId == kAudioObjectUnknown) {
            state->aggregateId = kAudioObjectUnknown;
            Log("[WARN] Failed to create system audio tap aggregate device (OSStatus " + std::to_string(status) + ")");
            return false;
        }

        auto* ctx = new MacSystemTapContext();
        ctx->owner = owner;
        ctx->sampleRate = static_cast<uint32_t>(tapFormat.mSampleRate > 0 ? tapFormat.mSampleRate : 48000);
        // The output device's own input streams (e.g. a DJ controller's line-in) come before the tap's
        ctx->firstTapBuffer = InputBufferCount(outputId);
        ctx->mono.assign(16384, 0.0f);
        state->tapContext = ctx;

        status = AudioDeviceCreateIOProcID(state->aggregateId, SystemTapIOProc, ctx, &state->tapProcId);
        if (status != noErr || !state->tapProcId) {
            state->tapProcId = nullptr;
            Log("[WARN] Failed to create system audio tap IOProc (OSStatus " + std::to_string(status) + ")");
            return false;
        }

        // The first start shows the "System Audio Recording Only" prompt
        status = AudioDeviceStart(state->aggregateId, state->tapProcId);
        if (status != noErr) {
            Log("[WARN] Failed to start system audio tap (OSStatus " + std::to_string(status) + ")");
            return false;
        }

        Log("[INFO] macOS Core Audio system tap started (" + std::to_string(ctx->sampleRate) + " Hz, " +
            std::to_string(tapFormat.mChannelsPerFrame) + " ch, tap buffers from index " +
            std::to_string(ctx->firstTapBuffer) + " of " + std::to_string(InputBufferCount(state->aggregateId)) + ")");
        return true;
    }
}

bool WasapiAudioCapture::start(const std::wstring& deviceId, bool isLoopback) {
    stop();
    auto* state = static_cast<MacAudioInternalState*>(m_macAudioState);
    if (!state) return false;

    m_isRunning = true;

    if (isLoopback) {
        // 1. macOS 14.2+: Core Audio process tap. Falls back to the default input rather than
        //    ScreenCaptureKit, which would bring back the Screen Recording prompts
        if (@available(macOS 14.2, *)) {
            if (StartSystemTapOnMac(this, state)) {
                return true;
            }
            StopSystemTapOnMac(state);
            Log("[WARN] Core Audio system tap unavailable, falling back to default audio input");
            return StartInputCaptureOnMac(this, state, L"");
        }

        // 2. macOS 13.x: ScreenCaptureKit System Audio Loopback
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

    // 3. CoreAudio Input / DJ Mixer Line-In Capture
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
        StopSystemTapOnMac(state);
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
