#pragma once

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <mmsystem.h>
    #pragma comment(lib, "winmm.lib")
#endif

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <cstdint>

namespace Visualizer::Control {

struct MidiDeviceInfo {
    uint32_t id = 0;
    std::wstring name;
};

class MidiController {
public:
    using MidiCCCallback = std::function<void(uint8_t channel, uint8_t controller, uint8_t value)>;
    using MidiNoteCallback = std::function<void(uint8_t channel, uint8_t note, uint8_t velocity)>;

    MidiController();
    ~MidiController();

    // Enumerate connected MIDI controllers (Pioneer, Akai, Novation, etc.)
    std::vector<MidiDeviceInfo> enumerateDevices();

    // Connect to MIDI device by ID
    bool openDevice(uint32_t deviceId);
    void closeDevice();

    bool isConnected() const { return m_isConnected; }
    std::wstring getConnectedDeviceName() const { return m_currentDeviceName; }

    // Get normalized CC value [0.0 .. 1.0]
    float getCCValue(uint8_t controller, float defaultValue = 0.0f) const;

    // Callbacks
    void setCCCallback(MidiCCCallback cb) { m_ccCallback = cb; }
    void setNoteCallback(MidiNoteCallback cb) { m_noteCallback = cb; }

private:
#if defined(_WIN32)
    static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void handleMidiMessage(DWORD_PTR dwParam1);
    HMIDIIN m_hMidiIn = nullptr;
#endif

    bool m_isConnected = false;
    std::wstring m_currentDeviceName;

    mutable std::mutex m_mutex;
    std::unordered_map<uint8_t, float> m_ccValues;

    MidiCCCallback m_ccCallback;
    MidiNoteCallback m_noteCallback;
};

} // namespace Visualizer::Control
