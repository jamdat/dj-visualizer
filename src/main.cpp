#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <mmsystem.h>
    #include <objbase.h>
#endif

#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>

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

#if defined(_WIN32)
static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep) {
    DWORD code = ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0;
    void* addr = ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionAddress : nullptr;
    char buf[256];
    snprintf(buf, sizeof(buf), "[CRASH] ExceptionCode=0x%08lX at Address=%p", code, addr);
    Visualizer::Log(buf);
    MessageBoxA(nullptr, buf, "DJ Visualizer Crash Handler", MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static int RunVisualizerApp() {
    using namespace Visualizer;

#if defined(_WIN32)
    SetUnhandledExceptionFilter(CrashHandler);
    timeBeginPeriod(1);
    HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    Log("[INFO] COM initialized (hr=" + std::to_string(hrCom) + ")");
#endif

    ClearLog();
    Log("[INFO] DJ Visualizer starting...");

    try {
        // 1. Initialize Cross-Platform Windowing System (GLFW + OpenGL 3.3 Core)
        Log("[INFO] Step 1: Initializing WindowManager...");
        Graphics::WindowManager windowManager;
        if (!windowManager.createControlWindow(nullptr, 1600, 900, L"DJ Audio-Reactive Visualizer Console | Daft Alive Edition")) {
            Log("[ERROR] createControlWindow failed");
            return -1;
        }
        Log("[INFO] Window created successfully");

        // 2. Initialize Cross-Platform OpenGL 3.3 Core Renderer & Pipeline
        Log("[INFO] Step 2: Initializing Renderer...");
        Graphics::D3D11Renderer renderer;
        if (!renderer.initialize(&windowManager)) {
            Log("[ERROR] Renderer::initialize failed");
            return -1;
        }
        Log("[INFO] Renderer initialized successfully");

        // 3. Initialize Audio Engine & DSP
        Log("[INFO] Step 3: Initializing Audio Engine & DSP...");
        Audio::WasapiAudioCapture audioCapture;
        Audio::AudioAnalyzer audioAnalyzer(1024, 48000);
        Audio::BeatDetector beatDetector;

        // 4. Initialize Control Subsystems
        Log("[INFO] Step 4: Initializing Presets & Control...");
        Control::PresetManager presetManager("presets");
        presetManager.initialize();

        Control::AutoPilot autoPilot;
        Control::MidiController midiController;

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

        if (!dashboard.initialize(windowManager.getControlWindow(), renderer.getDevice(), renderer.getContext())) {
            Log("[ERROR] dashboard.initialize failed");
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

        Control::VisualizerPreset defaultPreset = Control::PresetManager::createDaftAlivePreset();
        defaultPreset.applyToParams(params);
        Log("[INFO] Default recipe applied, starting main loop...");

        windowManager.setResizeCallback([&](int w, int h, bool isStage) {
            if (isStage) renderer.resizeStage(w, h);
            else renderer.resizeControl(w, h);
        });

        windowManager.setKeyCallback([&](int key, int action) {
            if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && windowManager.isSingleScreenFullscreen()) {
                autoPilot.triggerDropTransition(params);
            }
        });

        // Show main console window maximized now that everything is initialized
        windowManager.showControlWindow(true);

        // 8. High-Precision Cross-Platform Timing
        auto lastTime = std::chrono::steady_clock::now();

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

            auto currTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(currTime - lastTime).count();
            lastTime = currTime;

            if (deltaTime > 0.1f) deltaTime = 0.1f;
            if (deltaTime < 0.0001f) deltaTime = 0.0001f;

            accumulatedTime += deltaTime;
            params.Time = accumulatedTime;
            params.DeltaTime = deltaTime;
            params.Resolution = { 1920.0f, 1080.0f };
            params.AspectRatio = 1920.0f / 1080.0f;

            // A. Ingest Audio Data & DSP
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

            // 2. Render Post-Process & Stage Output
            renderer.renderPostProcess(params);

            // 3. Bind Control Window Render Target and Render UI / Fullscreen
            renderer.bindControlRenderTarget();
            if (windowManager.isSingleScreenFullscreen()) {
                renderer.renderFullscreenBlit();
            } else {
                renderer.renderFullscreenBlit();
                dashboard.render(deltaTime, params);
            }

            // 4. Present Frame
            renderer.endFrame(dashboard.isVsyncEnabled());
        }

        // Cleanup
        audioCapture.stop();
        dashboard.shutdown();
#if defined(_WIN32)
        timeEndPeriod(1);
        CoUninitialize();
#endif
        Log("[INFO] Visualizer shutting down cleanly");
    } catch (const std::exception& e) {
        Log(std::string("[FATAL EXCEPTION] ") + e.what());
        return -1;
    } catch (...) {
        Log("[FATAL UNKNOWN EXCEPTION]");
        return -1;
    }

    return 0;
}

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    return RunVisualizerApp();
}
#endif

int main(int /*argc*/, char** /*argv*/) {
    return RunVisualizerApp();
}
