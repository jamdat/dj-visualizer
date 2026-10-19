#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <chrono>
#include <string>

#include "graphics/WindowManager.h"
#include "graphics/D3D11Renderer.h"
#include "audio/WasapiAudioCapture.h"
#include "audio/AudioAnalyzer.h"
#include "audio/BeatDetector.h"
#include "control/PresetManager.h"
#include "control/AutoPilot.h"
#include "control/MidiController.h"
#include "ui/DjDashboard.h"
#include "core/Logger.h"

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep) {
    DWORD code = ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0;
    void* addr = ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionAddress : nullptr;
    char buf[256];
    snprintf(buf, sizeof(buf), "[CRASH] ExceptionCode=0x%08lX at Address=%p", code, addr);
    Visualizer::Log(buf);
    MessageBoxA(nullptr, buf, "DJ Visualizer Crash Handler", MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    using namespace Visualizer;

    SetUnhandledExceptionFilter(CrashHandler);
    timeBeginPeriod(1);

    ClearLog();
    Log("[INFO] WinMain started");

    HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Log("[INFO] COM initialized (hr=" + std::to_string(hrCom) + ")");

    try {
        // 1. Initialize Windowing System
        Log("[INFO] Step 1: Initializing WindowManager...");
        Graphics::WindowManager windowManager;
        if (!windowManager.createControlWindow(hInstance, 1600, 900, L"DJ Audio-Reactive Visualizer Console | Daft Alive Edition")) {
            Log("[ERROR] createControlWindow failed");
            MessageBoxW(nullptr, L"Failed to create main DJ console window.", L"Fatal Error", MB_ICONERROR);
            return -1;
        }
        Log("[INFO] Window created successfully");

        // 2. Initialize Direct3D 11 Renderer & Pipeline
        Log("[INFO] Step 2: Initializing D3D11Renderer...");
        Graphics::D3D11Renderer renderer;
        if (!renderer.initialize(&windowManager)) {
            Log("[ERROR] D3D11Renderer::initialize failed");
            MessageBoxW(nullptr, L"Failed to initialize Direct3D 11 graphics device.", L"Fatal Error", MB_ICONERROR);
            return -1;
        }
        Log("[INFO] D3D11Renderer initialized successfully");

        // Immediately hook resize callback so any WM_SIZE messages resize the swap chain
        windowManager.setMessageCallback([&](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            if (msg == WM_SIZE) {
                int w = LOWORD(lParam);
                int h = HIWORD(lParam);
                if (hwnd == windowManager.getControlHWND()) {
                    renderer.resizeControl(w, h);
                } else if (hwnd == windowManager.getStageHWND()) {
                    renderer.resizeStage(w, h);
                }
            }
            return 0;
        });

        // 3. Initialize Audio Engine & DSP
        Log("[INFO] Step 3: Initializing Audio Engine & DSP...");
        Audio::WasapiAudioCapture audioCapture;
        Audio::AudioAnalyzer audioAnalyzer(1024, 48000);
        Audio::BeatDetector beatDetector;

        // 4. Initialize Control Subsystems
        Log("[INFO] Step 4: Initializing Presets & Control...");
        Control::PresetManager presetManager("presets");
        Log("[INFO] Calling presetManager.initialize()...");
        presetManager.initialize();
        Log("[INFO] presetManager.initialize() done");

        Control::AutoPilot autoPilot;
        Control::MidiController midiController;
        Log("[INFO] AutoPilot and MidiController constructed");

        // 5. Initialize ImGui DJ Dashboard UI
        Log("[INFO] Step 5: Initializing ImGui Dashboard...");
        UI::DjDashboard dashboard(
            &windowManager,
            &renderer,
            &audioCapture,
            &audioAnalyzer,
            &beatDetector,
            &presetManager,
            &autoPilot,
            &midiController
        );
        Log("[INFO] DjDashboard constructed, calling initialize...");

        if (!dashboard.initialize(windowManager.getControlHWND(), renderer.getDevice(), renderer.getContext())) {
            Log("[ERROR] dashboard.initialize failed");
            MessageBoxW(nullptr, L"Failed to initialize Dear ImGui dashboard.", L"Fatal Error", MB_ICONERROR);
            return -1;
        }
        Log("[INFO] ImGui Dashboard initialized successfully");

        // Start default system loopback capture
        audioCapture.start(L"", true);
        Log("[INFO] Audio capture started");

        // 6. Compile Initial Shaders
        Log("[INFO] Step 6: Compiling Shaders...");
        auto* compiler = renderer.getShaderCompiler();
        bool bMaster = compiler->loadPixelShader("ModularMixer", "src/shaders/ModularMixer.hlsl", nullptr);
        Log(std::string("[INFO] ModularMixer: ") + (bMaster ? "OK" : compiler->getLastError("ModularMixer")));
        bool b1 = compiler->loadPixelShader("AlivePyramid", "src/shaders/AlivePyramid.hlsl", nullptr);
        Log(std::string("[INFO] AlivePyramid: ") + (b1 ? "OK" : compiler->getLastError("AlivePyramid")));
        bool b2 = compiler->loadPixelShader("SynthwaveGrid", "src/shaders/SynthwaveGrid.hlsl", nullptr);
        Log(std::string("[INFO] SynthwaveGrid: ") + (b2 ? "OK" : compiler->getLastError("SynthwaveGrid")));
        bool b3 = compiler->loadPixelShader("RaymarchedCrystals", "src/shaders/RaymarchedCrystals.hlsl", nullptr);
        Log(std::string("[INFO] RaymarchedCrystals: ") + (b3 ? "OK" : compiler->getLastError("RaymarchedCrystals")));
        bool b4 = compiler->loadPixelShader("ConcertLasers", "src/shaders/ConcertLasers.hlsl", nullptr);
        Log(std::string("[INFO] ConcertLasers: ") + (b4 ? "OK" : compiler->getLastError("ConcertLasers")));
        bool b5 = compiler->loadPixelShader("PostProcessGlitch", "src/shaders/PostProcessGlitch.hlsl", nullptr);
        Log(std::string("[INFO] PostProcessGlitch: ") + (b5 ? "OK" : compiler->getLastError("PostProcessGlitch")));

        // 7. Setup Visualizer Parameters & Defaults
        Graphics::AudioVisualizerParams params;
        std::string currentSceneShaderName = "ModularMixer";

        // Apply default Daft Alive Preset
        Control::VisualizerPreset defaultPreset = Control::PresetManager::createDaftAlivePreset();
        defaultPreset.applyToParams(params);
        Log("[INFO] Default recipe applied, starting main loop...");

    // Setup window resize callbacks
    windowManager.setMessageCallback([&](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        if (msg == WM_SIZE) {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            if (hwnd == windowManager.getControlHWND()) {
                renderer.resizeControl(w, h);
            } else if (hwnd == windowManager.getStageHWND()) {
                renderer.resizeStage(w, h);
            }
        } else if (msg == WM_KEYDOWN && wParam == VK_SPACE) {
            // Spacebar drop trigger for fullscreen presentation mode
            if (windowManager.isSingleScreenFullscreen()) {
                autoPilot.triggerDropTransition(params);
            }
        }
        return 0;
    });

    // Show main console window maximized now that everything is initialized
    windowManager.showControlWindow(true);

    // Explicitly sync swap chain size to the actual maximized client rectangle
    RECT clientRect = {};
    GetClientRect(windowManager.getControlHWND(), &clientRect);
    int clientW = clientRect.right - clientRect.left;
    int clientH = clientRect.bottom - clientRect.top;
    if (clientW > 0 && clientH > 0) {
        renderer.resizeControl(clientW, clientH);
        Log("[INFO] Window shown & resized swapchain to: " + std::to_string(clientW) + "x" + std::to_string(clientH));
    }

    // 8. High-Precision Timing
    LARGE_INTEGER perfFreq, lastTime, currTime;
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&lastTime);

    std::vector<float> audioSamples;
    float accumulatedTime = 0.0f;
    float hotReloadTimer = 0.0f;

    int frameCount = 0;
    // 9. Main Visualizer Loop
    while (windowManager.processMessages()) {
        frameCount++;
        if (frameCount <= 5 || frameCount % 60 == 0) {
            Log("[FRAME " + std::to_string(frameCount) + "] FPS: " + std::to_string(dashboard.getFPS()));
        }

        QueryPerformanceCounter(&currTime);
        float deltaTime = static_cast<float>(currTime.QuadPart - lastTime.QuadPart) / static_cast<float>(perfFreq.QuadPart);
        lastTime = currTime;

        // Clamp delta time to avoid physics/time explosion on window pause
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        if (deltaTime < 0.0001f) deltaTime = 0.0001f;

        accumulatedTime += deltaTime;
        params.Time = accumulatedTime;
        params.DeltaTime = deltaTime;
        params.Resolution = { 1920.0f, 1080.0f };
        params.AspectRatio = 1920.0f / 1080.0f;

        // A. Ingest WASAPI Audio Data & DSP
        audioCapture.getLatestSamples(audioSamples, 1024);
        audioAnalyzer.process(audioSamples, deltaTime);
        const auto& bands = audioAnalyzer.getBands();

        // B. Update Beat & Onset Detector
        beatDetector.update(bands, deltaTime);

        // C. Update Shader Audio Uniforms
        params.AudioSubBass = bands.subBass;
        params.AudioKick = bands.bass;
        params.AudioLowMid = bands.lowMid;
        params.AudioHighMid = bands.highMid;
        params.AudioTreble = bands.treble;
        params.BeatPulse = beatDetector.getBeatPulse();
        params.DropIntensity = beatDetector.getDropIntensity();
        params.EnergyAccumulator = audioAnalyzer.getEnergyAccumulator();
        params.Bpm = beatDetector.getBpm();

        // D. Update Auto-Pilot State Machine
        autoPilot.update(deltaTime, beatDetector, params);
        params.StrobeFlash = 0.0f;

        // E. Live Shader & Texture Hot-Reloading Check (Every 100ms)
        hotReloadTimer += deltaTime;
        if (hotReloadTimer >= 0.1f) {
            hotReloadTimer = 0.0f;
            std::string reloadedName, reloadErr;
            compiler->checkForHotReload(reloadedName, reloadErr);
            renderer.getTextureManager()->checkForHotReload();
        }

        // F. Upload Dynamic Audio Texture (Row 0: Spectrum, Row 1: Waveform)
        renderer.getTextureManager()->updateAudioTexture(audioAnalyzer.getSpectrum(), audioAnalyzer.getWaveform());

        // G. Render Graphics Pipeline
        renderer.beginFrame();

        // 1. Render Active Scene Shader to 1080p Target
        renderer.renderScene(currentSceneShaderName, params);

        // 2. Render Post-Process (Glitch, Chromatic Aberration, Tone Map) & Spout 2 Shared Output
        renderer.renderPostProcess(params);

        // 3. Bind Control Window Render Target and Render UI / Fullscreen
        renderer.bindControlRenderTarget();
        if (windowManager.isSingleScreenFullscreen()) {
            renderer.renderFullscreenBlit();
        } else {
            // Render the live shader visual in the background so the control window always has active visuals
            renderer.renderFullscreenBlit();
            dashboard.render(deltaTime, params);
        }

        // 4. Present Frame (VSync)
        renderer.endFrame(true);

        // High-precision 60 FPS frame pacing (with timeBeginPeriod(1) active)
        LARGE_INTEGER frameEndTime;
        QueryPerformanceCounter(&frameEndTime);
        float elapsedFrameSec = static_cast<float>(frameEndTime.QuadPart - currTime.QuadPart) / static_cast<float>(perfFreq.QuadPart);
        if (elapsedFrameSec < (1.0f / 60.0f)) {
            DWORD sleepMs = static_cast<DWORD>(((1.0f / 60.0f) - elapsedFrameSec) * 1000.0f);
            if (sleepMs > 1) {
                Sleep(sleepMs - 1);
            }
        }
    }

    // Cleanup
    audioCapture.stop();
    dashboard.shutdown();
    timeEndPeriod(1);
    Log("[INFO] Visualizer shutting down cleanly");

    } catch (const std::exception& e) {
        Log(std::string("[FATAL EXCEPTION] ") + e.what());
        MessageBoxA(nullptr, e.what(), "Fatal Exception", MB_ICONERROR);
        return -1;
    } catch (...) {
        Log("[FATAL UNKNOWN EXCEPTION]");
        MessageBoxW(nullptr, L"An unknown fatal exception occurred.", L"Fatal Exception", MB_ICONERROR);
        return -1;
    }

    return 0;
}
