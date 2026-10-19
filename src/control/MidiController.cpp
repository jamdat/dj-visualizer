#include "MidiController.h"
#include <iostream>

namespace Visualizer::Control {

MidiController::MidiController() = default;

MidiController::~MidiController() {
    closeDevice();
}

std::vector<MidiDeviceInfo> MidiController::enumerateDevices() {
    std::vector<MidiDeviceInfo> devices;
#if defined(_WIN32)
    UINT numDevs = midiInGetNumDevs();
    for (UINT i = 0; i < numDevs; ++i) {
        MIDIINCAPSW caps = {};
        if (midiInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            MidiDeviceInfo info;
            info.id = i;
            info.name = caps.szPname;
            devices.push_back(info);
        }
    }
#endif
    return devices;
}

bool MidiController::openDevice(uint32_t deviceId) {
    closeDevice();
#if defined(_WIN32)
    UINT numDevs = midiInGetNumDevs();
    if (deviceId >= numDevs) return false;

    MIDIINCAPSW caps = {};
    if (midiInGetDevCapsW(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
        m_currentDeviceName = caps.szPname;
    }

    MMRESULT res = midiInOpen(&m_hMidiIn, deviceId, reinterpret_cast<DWORD_PTR>(MidiInProc), reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) {
        m_hMidiIn = nullptr;
        m_isConnected = false;
        return false;
    }

    midiInStart(m_hMidiIn);
    m_isConnected = true;
    return true;
#else
    (void)deviceId;
    return false;
#endif
}

void MidiController::closeDevice() {
#if defined(_WIN32)
    if (m_hMidiIn) {
        midiInStop(m_hMidiIn);
        midiInClose(m_hMidiIn);
        m_hMidiIn = nullptr;
    }
#endif
    m_isConnected = false;
    m_currentDeviceName.clear();
}

float MidiController::getCCValue(uint8_t controller, float defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ccValues.find(controller);
    if (it != m_ccValues.end()) {
        return it->second;
    }
    return defaultValue;
}

#if defined(_WIN32)
void CALLBACK MidiController::MidiInProc(HMIDIIN /*hMidiIn*/, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR /*dwParam2*/) {
    if (wMsg == MIM_DATA && dwInstance) {
        auto* self = reinterpret_cast<MidiController*>(dwInstance);
        self->handleMidiMessage(dwParam1);
    }
}

void MidiController::handleMidiMessage(DWORD_PTR dwParam1) {
    uint8_t status = static_cast<uint8_t>(dwParam1 & 0xFF);
    uint8_t data1  = static_cast<uint8_t>((dwParam1 >> 8) & 0xFF);
    uint8_t data2  = static_cast<uint8_t>((dwParam1 >> 16) & 0xFF);

    uint8_t messageType = status & 0xF0;
    uint8_t channel = status & 0x0F;

    if (messageType == 0xB0) {
        uint8_t ccNumber = data1;
        uint8_t ccVal = data2;
        float normVal = static_cast<float>(ccVal) / 127.0f;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_ccValues[ccNumber] = normVal;
        }

        if (m_ccCallback) {
            m_ccCallback(channel, ccNumber, ccVal);
        }
    } else if (messageType == 0x90) {
        if (m_noteCallback) {
            m_noteCallback(channel, data1, data2);
        }
    }
}
#endif

} // namespace Visualizer::Control
