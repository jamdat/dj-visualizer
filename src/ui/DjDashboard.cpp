#include "DjDashboard.h"
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_glfw.h"
#include "../../third_party/imgui/backends/imgui_impl_opengl3.h"
#include "../core/Logger.h"
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <cstdint>

namespace Visualizer::UI {

static std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    std::string out;
    out.reserve(wstr.size());
    for (wchar_t wc : wstr) {
        if (wc < 0x80) {
            out.push_back(static_cast<char>(wc));
        } else if (wc < 0x800) {
            out.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
    }
    return out;
}

DjDashboard::DjDashboard(
    Graphics::WindowManager* windowManager,
    Graphics::D3D11Renderer* renderer,
    Audio::WasapiAudioCapture* audioCapture,
    Audio::AudioAnalyzer* audioAnalyzer,
    Audio::BeatDetector* beatDetector,
    Control::PresetManager* presetManager,
    Control::AutoPilot* autoPilot,
    Control::MidiController* midiController
) : m_windowManager(windowManager),
    m_renderer(renderer),
    m_audioCapture(audioCapture),
    m_audioAnalyzer(audioAnalyzer),
    m_beatDetector(beatDetector),
    m_presetManager(presetManager),
    m_autoPilot(autoPilot),
    m_midiController(midiController) {
}

DjDashboard::~DjDashboard() {
    shutdown();
}

bool DjDashboard::initialize(GLFWwindow* window, void* /*device*/, void* /*context*/) {
    try {
        Log("DjDashboard::initialize started");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Load crisp system font on Windows or macOS
        ImFont* font = nullptr;
        std::error_code ec;
#if defined(_WIN32)
        if (std::filesystem::exists("C:\\Windows\\Fonts\\segoeui.ttf", ec)) {
            font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
        } else if (std::filesystem::exists("C:\\Windows\\Fonts\\arial.ttf", ec)) {
            font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 20.0f);
        }
#elif defined(__APPLE__)
        if (std::filesystem::exists("/System/Library/Fonts/Supplemental/Arial.ttf", ec)) {
            font = io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Supplemental/Arial.ttf", 20.0f);
        } else if (std::filesystem::exists("/System/Library/Fonts/SFNS.ttf", ec)) {
            font = io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/SFNS.ttf", 20.0f);
        }
#endif
        if (!font) {
            io.Fonts->AddFontDefault();
        }

        m_appliedUiScale = m_uiScale;
        io.FontGlobalScale = m_uiScale;
        applyCustomTheme(m_uiScale);

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");

        m_audioDevices = m_audioCapture->enumerateDevices();
        m_presetList = m_presetManager->getPresetNames();
        if (m_windowManager) {
            m_monitors = m_windowManager->enumerateMonitors();
        }

        m_autoUpdater.checkForUpdatesAsync();

        m_isInitialized = true;
        return true;
    } catch (const std::exception& e) {
        Log(std::string("Exception in DjDashboard::initialize: ") + e.what());
        return false;
    } catch (...) {
        Log("Unknown exception in DjDashboard::initialize");
        return false;
    }
}

void DjDashboard::shutdown() {
    if (m_isInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_isInitialized = false;
    }
}

void DjDashboard::applyCustomTheme(float scale) {
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();

    style.WindowRounding    = 6.0f * scale;
    style.FrameRounding     = 4.0f * scale;
    style.PopupRounding     = 6.0f * scale;
    style.ScrollbarRounding = 6.0f * scale;
    style.GrabRounding      = 4.0f * scale;
    style.TabRounding       = 4.0f * scale;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(10.0f * scale, 8.0f * scale);
    style.ItemInnerSpacing  = ImVec2(6.0f * scale, 6.0f * scale);
    style.FramePadding      = ImVec2(8.0f * scale, 6.0f * scale);
    style.WindowPadding     = ImVec2(10.0f * scale, 10.0f * scale);
    style.ScrollbarSize     = 16.0f * scale;
    style.GrabMinSize       = 12.0f * scale;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.52f, 0.58f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.04f, 0.05f, 0.07f, 0.95f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.06f, 0.07f, 0.10f, 0.85f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.09f, 0.11f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.22f, 0.25f, 0.32f, 0.60f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.12f, 0.14f, 0.18f, 0.85f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.23f, 0.29f, 0.95f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.25f, 0.29f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.09f, 0.11f, 0.80f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.17f, 0.22f, 0.90f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.08f, 0.09f, 0.11f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24f, 0.27f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.32f, 0.36f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.45f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(1.00f, 0.35f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 0.60f, 0.10f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.16f, 0.19f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(1.00f, 0.35f, 0.00f, 0.85f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(1.00f, 0.50f, 0.10f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.22f, 0.29f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(1.00f, 0.40f, 0.00f, 0.80f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(1.00f, 0.55f, 0.10f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.23f, 0.30f, 1.00f);
}

void DjDashboard::triggerSpacebarDrop() {
    if (m_autoPilot) {
        m_autoPilot->triggerDropTransition();
    }
}

void DjDashboard::render(
    float deltaTime,
    Graphics::AudioVisualizerParams& /*params*/
) {
    ImGuiIO& io = ImGui::GetIO();

    // Hotkeys: Spacebar (Drop) and 1..6 (Direct Mood Selection) when not typing in text box
    if (!io.WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            triggerSpacebarDrop();
        }
        for (int k = 0; k < 6; ++k) {
            ImGuiKey key = static_cast<ImGuiKey>(ImGuiKey_1 + k);
            if (ImGui::IsKeyPressed(key, false)) {
                m_autoPilot->selectScene(k, 30.0f);
            }
        }
    }

    if (std::abs(m_uiScale - m_appliedUiScale) > 0.001f) {
        m_appliedUiScale = m_uiScale;
        io.FontGlobalScale = m_uiScale;
        applyCustomTheme(m_uiScale);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_fps = io.Framerate;
    m_frameTimeMs = 1000.0f / (m_fps > 0.0f ? m_fps : 60.0f);

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Dad Punk DJ Console", nullptr, winFlags);

    renderHeaderBar();
    ImGui::Separator();

    renderTopStatusBar(deltaTime);
    ImGui::Separator();

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    float leftColWidth = contentSize.x * 0.52f;
    float rightColWidth = contentSize.x - leftColWidth - ImGui::GetStyle().ItemSpacing.x;

    // LEFT COLUMN: Mood scenes, Logo text, Palettes, and Kid-Safe controls
    ImGui::BeginChild("LeftControlsPanel", ImVec2(leftColWidth, contentSize.y), false);
    renderMoodButtons();
    ImGui::Spacing();
    renderTextAndPalettePanel();
    ImGui::Spacing();
    renderKidSafeAndLayerControls();
    ImGui::EndChild();

    ImGui::SameLine();

    // RIGHT COLUMN: Live Stage Preview, Audio Meters, and Output Controls
    ImGui::BeginChild("RightPreviewPanel", ImVec2(rightColWidth, contentSize.y), false);
    renderLivePreview();
    ImGui::Spacing();
    renderAudioSpectrumPanel();
    ImGui::Spacing();
    renderProjectorOutputPanel();
    ImGui::EndChild();

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DjDashboard::renderHeaderBar() {
    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.0f, 1.0f), "🎃🔺 DAD PUNK LIVE DJ VISUALIZER");
    ImGui::SameLine();
    ImGui::TextDisabled("| 1080p Stage TV Edition | %s", m_autoUpdater.getCurrentVersion().c_str());

    ImGui::SameLine();
    auto upState = m_autoUpdater.getState();
    if (upState == Control::UpdateState::UpdateAvailable) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.75f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        std::string btnLabel = "⬇ Install Update (" + m_autoUpdater.getLatestVersion() + ") & Restart";
        if (ImGui::SmallButton(btnLabel.c_str())) {
            m_autoUpdater.downloadAndInstallUpdateAsync();
        }
        ImGui::PopStyleColor(2);
    } else if (upState == Control::UpdateState::DownloadingAndInstalling) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "⏳ %s", m_autoUpdater.getStatusMessage().c_str());
    } else if (upState == Control::UpdateState::Checking) {
        ImGui::TextDisabled("Checking updates...");
    } else {
        if (ImGui::SmallButton("Check Updates")) {
            m_autoUpdater.checkForUpdatesAsync();
        }
    }

    float rightAreaWidth = 380.0f * m_uiScale;
    float winWidth = ImGui::GetWindowWidth();
    if (winWidth > rightAreaWidth + 300.0f) {
        ImGui::SameLine(winWidth - rightAreaWidth);
    } else {
        ImGui::SameLine();
    }

#if defined(__APPLE__)
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[Apple Silicon arm64]");
#else
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[Win64 GL3.3 Core]");
#endif
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "%.0f FPS", m_fps);
    ImGui::SameLine();
    ImGui::Checkbox("VSync", &m_vsync);
}

void DjDashboard::renderTopStatusBar(float deltaTime) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 0.95f));
    ImGui::BeginChild("TopStatusBar", ImVec2(-1, 58.0f * m_uiScale), true);

    // 1. AUTO: ON / OFF Button
    bool autoOn = m_autoPilot->isEnabled();
    if (autoOn) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.75f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    }

    std::string autoLabel = autoOn ? "🤖 AUTO: ON" : "⏸️ AUTO: OFF";
    if (ImGui::Button(autoLabel.c_str(), ImVec2(130.0f * m_uiScale, 40.0f * m_uiScale))) {
        m_autoPilot->setEnabled(!autoOn);
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    // 2. DROP NOW [SPACEBAR]
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.35f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.55f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    if (ImGui::Button("⚡ DROP [SPACE]", ImVec2(140.0f * m_uiScale, 40.0f * m_uiScale))) {
        triggerSpacebarDrop();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // 3. Current Scene Display & Countdown
    int curSceneIdx = m_autoPilot->getCurrentSceneIndex();
    std::string curName = m_autoPilot->getCurrentSceneName();
    bool isHold = m_autoPilot->isManualHold();
    float timeLeft = m_autoPilot->getTimeRemainingInScene();

    ImGui::BeginGroup();
    if (isHold) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "MANUAL SCENE: [%s] (Hold: %.0fs)", curName.c_str(), timeLeft);
        ImGui::SameLine();
        if (ImGui::SmallButton("▶ Resume Auto")) {
            m_autoPilot->cancelManualHold();
        }
    } else {
        ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "ACTIVE SCENE: [%s] (Next phrase in %.0fs)", curName.c_str(), timeLeft);
    }

    // Audio status & BPM
    float bpm = m_beatDetector ? m_beatDetector->getBpm() : 128.0f;
    float pulse = m_beatDetector ? m_beatDetector->getBeatPulse() : 0.0f;
    ImGui::TextDisabled("BPM: %.0f | Beat Pulse: %.2f", bpm, pulse);
    ImGui::EndGroup();

    // 4. Audio silence warning if no sound detected
    const auto& bands = m_audioAnalyzer->getBands();
    float totalAudio = bands.subBass + bands.bass + bands.lowMid + bands.highMid + bands.treble;
    if (totalAudio < 0.005f) {
        m_silentWarningTimer += deltaTime;
    } else {
        m_silentWarningTimer = 0.0f;
    }

    if (m_silentWarningTimer > 2.0f) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 320.0f * m_uiScale);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));
        ImGui::Text("⚠️ NO AUDIO (Loopback Silent)");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void DjDashboard::renderMoodButtons() {
    if (ImGui::CollapsingHeader("1. 6 CURATED MOOD SCENES (Keys [1] - [6])", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& scenes = m_autoPilot->getScenes();
        int activeIdx = m_autoPilot->getCurrentSceneIndex();

        for (size_t i = 0; i < scenes.size(); ++i) {
            bool isActive = (activeIdx == static_cast<int>(i));
            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.45f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            }

            std::string label = "[" + std::to_string(i + 1) + "] " + scenes[i].icon + " " + scenes[i].name;
            if (ImGui::Button(label.c_str(), ImVec2(175.0f * m_uiScale, 45.0f * m_uiScale))) {
                m_autoPilot->selectScene(static_cast<int>(i), 30.0f);
            }

            if (isActive) {
                ImGui::PopStyleColor(2);
            }

            if ((i + 1) % 3 != 0 && i < scenes.size() - 1) {
                ImGui::SameLine();
            }
        }
    }
}

void DjDashboard::renderTextAndPalettePanel() {
    if (ImGui::CollapsingHeader("2. CUSTOM LED LOGO & PALETTE", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& user = m_autoPilot->getUserSettings();

        // Custom Logo Text Input Box
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Custom LED Dot-Matrix Text (16 chars):");
        ImGui::SetNextItemWidth(260.0f * m_uiScale);
        ImGui::InputText("##CustomText", user.customText, 17);

        ImGui::SameLine();
        if (ImGui::Button("Reset to 'DAD PUNK'")) {
            strncpy_s(user.customText, sizeof(user.customText), "DAD PUNK", _TRUNCATE);
        }

        // 5 Palette Swatches
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Concert Color Palette Swatches:");
        const char* paletteNames[] = {
            "🔴 Daft Red/Gold",
            "🔵 TRON Cyan/Orange",
            "🎃 Spooky Purple/Orange",
            "⚪ Chrome Silver/Blue",
            "🌈 Rainbow Discovery"
        };

        for (int p = 0; p < 5; ++p) {
            bool isCurrent = (user.paletteIndex == p);
            if (isCurrent) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.45f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            }

            if (ImGui::Button(paletteNames[p])) {
                user.paletteIndex = p;
            }

            if (isCurrent) {
                ImGui::PopStyleColor(2);
            }

            if (p < 4 && (p + 1) % 3 != 0) {
                ImGui::SameLine();
            }
        }
    }
}

void DjDashboard::renderKidSafeAndLayerControls() {
    if (ImGui::CollapsingHeader("3. KID-SAFE & LAYER MULTIPLIERS", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& user = m_autoPilot->getUserSettings();

        // Kid-Safe Master Brightness (Default 85%)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "🛡️ Elementary-School Safe (Zero Strobe):");
        ImGui::SliderFloat("Master Brightness", &user.masterBrightness, 0.1f, 1.0f, "%.2f (Kid-Safe Cap)");
        ImGui::Checkbox("🎃 Halloween Spooky FX Override", &user.halloweenMode);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Pyramid Face LED Pattern:");
        const char* patternNames[] = { "0: VU Strips", "1: Chevrons", "2: Scanner Sweep", "3: Solid Glow", "4: Rainbow" };
        ImGui::Combo("##PyramidPattern", &user.pyramidPattern, patternNames, 5);

        ImGui::Spacing();
        ImGui::TextDisabled("Non-Destructive Layer Multipliers (Won't snap back):");
        ImGui::SliderFloat("Pyramid Weight", &user.pyramidMultiplier, 0.0f, 2.0f, "%.2fx");
        ImGui::SliderFloat("Logo Weight", &user.logoMultiplier, 0.0f, 2.0f, "%.2fx");
        ImGui::SliderFloat("Laser Intensity", &user.laserMultiplier, 0.0f, 2.0f, "%.2fx");
        ImGui::SliderFloat("TRON Grid Floor", &user.floorMultiplier, 0.0f, 2.0f, "%.2fx");
        ImGui::SliderFloat("Sky Dome / Stars", &user.skyMultiplier, 0.0f, 2.0f, "%.2fx");
        ImGui::SliderFloat("Particles (Bats/Sparks)", &user.particleMultiplier, 0.0f, 2.0f, "%.2fx");
    }
}

void DjDashboard::renderLivePreview() {
    ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "LIVE STAGE PREVIEW (1080p Mirror):");

    GLuint postTex = m_renderer->getFinalSRV();
    if (postTex) {
        float previewW = ImGui::GetContentRegionAvail().x;
        float previewH = previewW * (9.0f / 16.0f);
        ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(postTex)), ImVec2(previewW, previewH), ImVec2(0, 1), ImVec2(1, 0));
    }
}

void DjDashboard::renderAudioSpectrumPanel() {
    if (ImGui::CollapsingHeader("AUDIO DSP & FREQUENCY BANDS", ImGuiTreeNodeFlags_DefaultOpen)) {
        float gain = m_audioAnalyzer->getGain();
        if (ImGui::SliderFloat("Audio Gain", &gain, 0.1f, 5.0f, "%.1fx")) {
            m_audioAnalyzer->setGain(gain);
        }

        const auto& bands = m_audioAnalyzer->getBands();
        float bandValues[5] = { bands.subBass, bands.bass, bands.lowMid, bands.highMid, bands.treble };
        ImGui::PlotHistogram("##Bands", bandValues, 5, 0, "Sub | Kick | LowMid | HighMid | Treble", 0.0f, 1.5f, ImVec2(-1, 55.0f * m_uiScale));

        // Audio device selector
        if (!m_audioDevices.empty()) {
            std::string curDev = (m_selectedAudioDevice < static_cast<int>(m_audioDevices.size()))
                ? WStringToString(m_audioDevices[m_selectedAudioDevice].name) : "Default";

            if (ImGui::BeginCombo("Capture Device", curDev.c_str())) {
                for (size_t i = 0; i < m_audioDevices.size(); ++i) {
                    bool isSelected = (m_selectedAudioDevice == static_cast<int>(i));
                    std::string devName = WStringToString(m_audioDevices[i].name);
                    if (ImGui::Selectable(devName.c_str(), isSelected)) {
                        m_selectedAudioDevice = static_cast<int>(i);
                        m_audioCapture->start(m_audioDevices[i].id, m_audioDevices[i].isLoopback);
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
    }
}

void DjDashboard::renderProjectorOutputPanel() {
    if (ImGui::CollapsingHeader("STAGE PROJECTOR & PRESET MANAGER", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Fullscreen Blit / Stage Toggle
        bool stageOpen = m_windowManager->isStageWindowOpen();
        if (stageOpen) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Close Stage Window", ImVec2(-1, 35.0f * m_uiScale))) {
                m_renderer->releaseStageSwapChain();
                m_windowManager->closeStageWindow();
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
            if (ImGui::Button("Open Stage Window on Secondary Display (TV)", ImVec2(-1, 35.0f * m_uiScale))) {
                if (m_windowManager->createStageWindow(nullptr, m_selectedMonitor)) {
                    m_renderer->setupStageSwapChain(m_windowManager->getStageWindow(), 1920, 1080);
                }
            }
            ImGui::PopStyleColor();
        }

        // Preset Save & Load
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Preset Recipes:");
        m_presetList = m_presetManager->getPresetNames();
        if (!m_presetList.empty()) {
            if (ImGui::BeginCombo("Select Preset", m_presetList[m_currentPresetIndex].c_str())) {
                for (size_t i = 0; i < m_presetList.size(); ++i) {
                    bool isSel = (m_currentPresetIndex == static_cast<int>(i));
                    if (ImGui::Selectable(m_presetList[i].c_str(), isSel)) {
                        m_currentPresetIndex = static_cast<int>(i);
                        Control::VisualizerPreset loaded;
                        if (m_presetManager->loadPreset(m_presetList[i], loaded)) {
                            auto& user = m_autoPilot->getUserSettings();
                            user.pyramidMultiplier = loaded.pyramidMultiplier;
                            user.logoMultiplier = loaded.logoMultiplier;
                            user.laserMultiplier = loaded.laserMultiplier;
                            user.floorMultiplier = loaded.floorMultiplier;
                            user.skyMultiplier = loaded.skyMultiplier;
                            user.particleMultiplier = loaded.particleMultiplier;
                            user.pyramidPattern = loaded.pyramidPattern;
                            user.logoStyle = loaded.logoStyle;
                            user.paletteIndex = loaded.paletteIndex;
                            user.halloweenMode = loaded.halloweenMode;
                            user.masterBrightness = loaded.masterBrightness;
                            strncpy_s(user.customText, sizeof(user.customText), loaded.customText.c_str(), _TRUNCATE);
                            m_autoPilot->selectScene(loaded.sceneIndex, 30.0f);
                        }
                    }
                    if (isSel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        ImGui::SetNextItemWidth(180.0f * m_uiScale);
        ImGui::InputText("##NewPreset", m_newPresetName, 64);
        ImGui::SameLine();
        if (ImGui::Button("Save Current Vibe")) {
            Control::VisualizerPreset saveP;
            saveP.name = m_newPresetName;
            saveP.sceneIndex = m_autoPilot->getCurrentSceneIndex();
            const auto& user = m_autoPilot->getUserSettings();
            saveP.pyramidMultiplier = user.pyramidMultiplier;
            saveP.logoMultiplier = user.logoMultiplier;
            saveP.laserMultiplier = user.laserMultiplier;
            saveP.floorMultiplier = user.floorMultiplier;
            saveP.skyMultiplier = user.skyMultiplier;
            saveP.particleMultiplier = user.particleMultiplier;
            saveP.pyramidPattern = user.pyramidPattern;
            saveP.logoStyle = user.logoStyle;
            saveP.paletteIndex = user.paletteIndex;
            saveP.halloweenMode = user.halloweenMode;
            saveP.masterBrightness = user.masterBrightness;
            saveP.customText = user.customText;
            m_presetManager->savePreset(saveP);
        }
    }
}

} // namespace Visualizer::UI
