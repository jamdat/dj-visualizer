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
#include <algorithm>
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
#include "control/SpaceVoyage.h"
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
        Control::SpaceVoyage spaceVoyage;

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
            &midiController,
            &spaceVoyage
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
        bool bSpace = compiler->loadPixelShader("SpaceVoyage", "src/shaders/SpaceVoyage.hlsl", nullptr);
        Log(std::string("[INFO] SpaceVoyage: ") + (bSpace ? "OK" : compiler->getLastError("SpaceVoyage")));
        bool bChem = compiler->loadPixelShader("Chemistry", "src/shaders/Chemistry.hlsl", nullptr);
        Log(std::string("[INFO] Chemistry: ") + (bChem ? "OK" : compiler->getLastError("Chemistry")));
        bool bQuantum = compiler->loadPixelShader("Quantum", "src/shaders/Quantum.hlsl", nullptr);
        Log(std::string("[INFO] Quantum: ") + (bQuantum ? "OK" : compiler->getLastError("Quantum")));
        bool bDaft = compiler->loadPixelShader("DaftPunk", "src/shaders/DaftPunk.hlsl", nullptr);
        Log(std::string("[INFO] DaftPunk: ") + (bDaft ? "OK" : compiler->getLastError("DaftPunk")));
        bool bDance = compiler->loadPixelShader("DanceParty", "src/shaders/DanceParty.hlsl", nullptr);
        Log(std::string("[INFO] DanceParty: ") + (bDance ? "OK" : compiler->getLastError("DanceParty")));

        // 7. Setup Visualizer Parameters & Defaults
        Graphics::AudioVisualizerParams params;
        std::string currentSceneShaderName = "ModularMixer";
        const std::string spaceSceneShaderName = "SpaceVoyage";
        const std::string chemistrySceneShaderName = "Chemistry";
        const std::string quantumSceneShaderName = "Quantum";
        const std::string daftPunkSceneShaderName = "DaftPunk";
        const std::string dancePartySceneShaderName = "DanceParty";
        Log("[INFO] ModularMixer loaded, starting main loop...");

        windowManager.setResizeCallback([&](int w, int h, bool isStage) {
            if (isStage) renderer.resizeStage(w, h);
            else renderer.resizeControl(w, h);
        });

        windowManager.setKeyCallback([&](int key, int action) {
            if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && windowManager.isSingleScreenFullscreen()) {
                dashboard.triggerSpacebarDrop();
            }
        });

        // Show main console window (restoring maximized state if it was previously saved)
        windowManager.showControlWindow(windowManager.wasMaximizedSaved());

        // 8. High-Precision Cross-Platform Timing
        auto lastTime = std::chrono::steady_clock::now();

        std::vector<float> audioSamples;
        float accumulatedTime = 0.0f;
        float hotReloadTimer = 0.0f;
        float beatPhase = 0.0f; // beats, integrated from the detected BPM, wraps at 64
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

            // C. Pack Audio Frame Data
            Control::AudioFrameData audioData;
            audioData.subBass = bands.subBass;
            audioData.bass = bands.bass;
            audioData.lowMid = bands.lowMid;
            audioData.highMid = bands.highMid;
            audioData.treble = bands.treble;
            audioData.beatPulse = beatDetector.getBeatPulse();
            audioData.dropIntensity = beatDetector.getDropIntensity();
            audioData.energyAccumulator = audioAnalyzer.getEnergyAccumulator();
            audioData.bpm = beatDetector.getBpm();
            audioData.rms = bands.subBass + bands.bass + bands.lowMid + bands.highMid + bands.treble;
            audioData.isSilent = (audioData.rms < 0.005f);

            // D. Update Auto-Pilot State Machine
            autoPilot.update(deltaTime, beatDetector, audioData);

            // E. Resolve GPU Parameters Non-Destructively
            autoPilot.resolveGpuParams(audioData, params);

            // Loop Visuals: the dashboard steps through the layouts on a timer (even while hidden in fullscreen)
            // and dips the stage to black around each switch
            dashboard.updateStageLoop(deltaTime);
            params.MasterBrightness *= dashboard.getStageFade();

            // Dashboard layout buttons (SPACE VOYAGE / CHEMISTRY / QUANTUM / DAFT PUNK / DANCE PARTY) swap in a standalone scene shader, with
            // the hero logo shrunk to a bottom-left badge. Falls back to ModularMixer if that shader failed to compile.
            UI::StageLayout stageLayout = dashboard.getStageLayout();
            const std::string& layoutShaderName =
                (stageLayout == UI::StageLayout::SpaceVoyage) ? spaceSceneShaderName :
                (stageLayout == UI::StageLayout::Chemistry) ? chemistrySceneShaderName :
                (stageLayout == UI::StageLayout::Quantum) ? quantumSceneShaderName :
                (stageLayout == UI::StageLayout::DaftPunk) ? daftPunkSceneShaderName :
                (stageLayout == UI::StageLayout::DanceParty) ? dancePartySceneShaderName : currentSceneShaderName;
            bool standaloneLayout = (stageLayout != UI::StageLayout::Default) && compiler->getPixelShader(layoutShaderName) != 0;
            const std::string& sceneShaderName = standaloneLayout ? layoutShaderName : currentSceneShaderName;
            renderer.setHeroLogoBadge(standaloneLayout);

            // Standalone layouts reuse two ModularMixer-only params: CustomParam7 = beat phase (tempo-locked
            // pulsar beams, bubble-chamber collisions, quantum pairs, helmet cycle, dance moves), CustomParam6 = Space Voyage clock
            beatPhase += deltaTime * (std::max)(audioData.bpm, 1.0f) / 60.0f;
            if (beatPhase >= 64.0f) beatPhase -= 64.0f;
            if (standaloneLayout) params.CustomParam7 = beatPhase;
            if (standaloneLayout && stageLayout == UI::StageLayout::SpaceVoyage) {
                spaceVoyage.update(deltaTime);
                params.CustomParam6 = spaceVoyage.getClock();
            } else {
                params.CustomParam6 = 0.0f;
            }

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
            renderer.renderScene(sceneShaderName, params);

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

            // 5. High-Precision Frame Rate Limiter (60, 90, 120, 144 FPS)
            int targetFps = dashboard.getTargetFps();
            if (targetFps > 0 && !dashboard.isVsyncEnabled()) {
                double targetFrameSec = 1.0 / static_cast<double>(targetFps);
                auto elapsedSec = std::chrono::duration<double>(std::chrono::steady_clock::now() - currTime).count();
                double remainingSec = targetFrameSec - elapsedSec;
                if (remainingSec > 0.0015) {
                    std::this_thread::sleep_for(std::chrono::duration<double>(remainingSec - 0.0012));
                }
                while (std::chrono::duration<double>(std::chrono::steady_clock::now() - currTime).count() < targetFrameSec) {
#if defined(_WIN32)
                    YieldProcessor();
#endif
                }
            }
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
