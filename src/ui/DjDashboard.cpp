#include "DjDashboard.h"
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_glfw.h"
#include "../../third_party/imgui/backends/imgui_impl_opengl3.h"
#include "../core/Logger.h"
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <cstdint>
#include <cstdlib>

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
    Control::MidiController* midiController,
    Control::SpaceVoyage* spaceVoyage
) : m_windowManager(windowManager),
    m_renderer(renderer),
    m_audioCapture(audioCapture),
    m_audioAnalyzer(audioAnalyzer),
    m_beatDetector(beatDetector),
    m_presetManager(presetManager),
    m_autoPilot(autoPilot),
    m_midiController(midiController),
    m_spaceVoyage(spaceVoyage) {
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
#if defined(__APPLE__)
        // GLFW makes Contents/Resources the working directory, and rewriting imgui.ini there breaks the
        // bundle's code signature (and with it the macOS audio permissions), so keep it in Application Support
        if (const char* home = std::getenv("HOME")) {
            std::filesystem::path iniDir = std::filesystem::path(home) / "Library/Application Support/DJVisualizer";
            std::error_code iniEc;
            std::filesystem::create_directories(iniDir, iniEc);
            static std::string s_iniPath = (iniDir / "imgui.ini").string();
            io.IniFilename = s_iniPath.c_str();
        }
#endif

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

        if (m_windowManager) {
            m_uiScale = m_windowManager->getUiScale();
            m_targetFps = m_windowManager->getTargetFps();
            if (m_targetFps == 60) m_fpsLimitIndex = 0;
            else if (m_targetFps == 90) m_fpsLimitIndex = 1;
            else if (m_targetFps == 120) m_fpsLimitIndex = 2;
            else if (m_targetFps == 144) m_fpsLimitIndex = 3;
            else { m_fpsLimitIndex = 4; m_targetFps = 0; }

            // Automatic Coordination: if an FPS limit is active, disable VSync
            if (m_targetFps > 0) {
                m_vsync = false;
            } else {
                m_vsync = true;
            }
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

// Loop Visuals order: Main screen -> Space Voyage -> Chemistry -> Quantum -> Main screen ...
static StageLayout NextLoopLayout(StageLayout layout) {
    if (layout == StageLayout::Default) return StageLayout::SpaceVoyage;
    if (layout == StageLayout::SpaceVoyage) return StageLayout::Chemistry;
    if (layout == StageLayout::Chemistry) return StageLayout::Quantum;
    return StageLayout::Default;
}

static const char* StageLayoutName(StageLayout layout) {
    if (layout == StageLayout::SpaceVoyage) return "Space Voyage";
    if (layout == StageLayout::Chemistry) return "Chemistry";
    if (layout == StageLayout::Quantum) return "Quantum";
    return "Main Screen";
}

void DjDashboard::setStageLayout(StageLayout layout) {
    m_stageLayout = layout;
    if (m_stageLayout == StageLayout::SpaceVoyage && m_spaceVoyage) {
        m_spaceVoyage->restart();
    }
}

void DjDashboard::updateStageLoop(float deltaTime) {
    m_loopFadeIn = (std::min)(m_loopFadeIn + deltaTime, kLoopFadeSeconds);
    if (!m_loopVisuals) return;

    m_loopTimer += deltaTime;
    if (m_loopTimer >= m_loopIntervalSec) {
        m_loopTimer = 0.0f;
        m_loopFadeIn = 0.0f;
        setStageLayout(NextLoopLayout(m_stageLayout));
    }
}

float DjDashboard::getStageFade() const {
    // Fade out over the last kLoopFadeSeconds before a loop switch, then back in after it
    float fadeIn = m_loopFadeIn / kLoopFadeSeconds;
    float fadeOut = m_loopVisuals ? std::clamp((m_loopIntervalSec - m_loopTimer) / kLoopFadeSeconds, 0.0f, 1.0f) : 1.0f;
    return (std::min)(fadeIn, fadeOut);
}

void DjDashboard::triggerSpacebarDrop() {
    // In the Space Voyage layout a drop jumps to the next destination instead
    if (m_stageLayout == StageLayout::SpaceVoyage && m_spaceVoyage) {
        m_spaceVoyage->warpToNext();
        return;
    }
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

    // LEFT COLUMN: Visual Ingredients Mixer & Consolidated Details Panel
    ImGui::BeginChild("LeftControlsPanel", ImVec2(leftColWidth, contentSize.y), false);
    renderVisualMixerPanel();
    ImGui::Spacing();
    renderConsolidatedDetailsPanel();
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
            if (m_autoUpdater.isDevBuild()) {
                m_showUpdateConfirmModal = true;
            } else {
                m_autoUpdater.downloadAndInstallUpdateAsync();
            }
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

    if (m_showUpdateConfirmModal) {
        ImGui::OpenPopup("Overwrite Warning##DevUpdate");
    }

    if (ImGui::BeginPopupModal("Overwrite Warning##DevUpdate", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.0f, 1.0f), "CAUTION: Local Development Build Detected!");
        ImGui::Spacing();
        ImGui::Text("Installing this GitHub release will download and overwrite files in:");
        ImGui::TextDisabled("build/bin/ (DJVisualizer.exe, shaders, and presets)");
        ImGui::Text("with the pre-built release package (%s).", m_autoUpdater.getLatestVersion().c_str());
        ImGui::Spacing();
        ImGui::Text("Any uncommitted or local shader edits may be overwritten.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("⚠️ Yes, Overwrite & Install", ImVec2(200.0f * m_uiScale, 0))) {
            m_showUpdateConfirmModal = false;
            ImGui::CloseCurrentPopup();
            m_autoUpdater.downloadAndInstallUpdateAsync();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f * m_uiScale, 0))) {
            m_showUpdateConfirmModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    float rightAreaWidth = 640.0f * m_uiScale;
    float winWidth = ImGui::GetWindowWidth();
    if (winWidth > rightAreaWidth + 300.0f) {
        ImGui::SameLine(winWidth - rightAreaWidth);
    } else {
        ImGui::SameLine();
    }

    // Quick Zoom Controls: UI: [-] 100% [+]
    static const float kScaleSteps[] = { 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f };
    ImGui::Text("UI:");
    ImGui::SameLine();
    if (ImGui::SmallButton("[-]")) {
        float nextScale = m_uiScale;
        for (int i = 5; i >= 0; --i) {
            if (kScaleSteps[i] < m_uiScale - 0.05f) {
                nextScale = kScaleSteps[i];
                break;
            }
        }
        if (nextScale != m_uiScale) {
            m_uiScale = nextScale;
            if (m_windowManager) m_windowManager->setUiScale(m_uiScale);
        }
    }
    ImGui::SameLine();
    ImGui::Text("%.0f%%", m_uiScale * 100.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("[+]")) {
        float nextScale = m_uiScale;
        for (int i = 0; i < 6; ++i) {
            if (kScaleSteps[i] > m_uiScale + 0.05f) {
                nextScale = kScaleSteps[i];
                break;
            }
        }
        if (nextScale != m_uiScale) {
            m_uiScale = nextScale;
            if (m_windowManager) m_windowManager->setUiScale(m_uiScale);
        }
    }
    ImGui::SameLine();

#if defined(__APPLE__)
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[Apple Silicon]");
#else
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[Win64 GL3.3]");
#endif
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "%.0f FPS", m_fps);
    ImGui::SameLine();

    // FPS Limiter Dropdown: 60, 90, 120, 144, Unlimited
    static const int kFpsLimits[] = { 60, 90, 120, 144, 0 };
    static const char* kFpsLimitLabels[] = { "60 FPS", "90 FPS", "120 FPS", "144 FPS", "Unlimited" };

    ImGui::SetNextItemWidth(108.0f * m_uiScale);
    if (ImGui::Combo("##FpsLimiter", &m_fpsLimitIndex, kFpsLimitLabels, 5)) {
        m_targetFps = kFpsLimits[m_fpsLimitIndex];
        if (m_windowManager) {
            m_windowManager->setTargetFps(m_targetFps);
        }
        // Automatic Coordination: selecting an FPS limit disables VSync
        if (m_targetFps > 0) {
            m_vsync = false;
        } else {
            m_vsync = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Frame Rate Limiter: 60, 90, 120, 144 FPS, or Unlimited (Persisted to config)");
    }

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

    // 3. Stage Layout Toggles: each swaps the stage to a standalone scene shader (one at a time)
    auto RenderLayoutButton = [&](StageLayout layout, const char* onLabel, const char* offLabel,
                                  const ImVec4& onColor, const ImVec4& offHover, float width, const char* tooltip) {
        bool active = (m_stageLayout == layout);
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, onColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(onColor.x * 0.7f + 0.3f, onColor.y * 0.7f + 0.3f, onColor.z * 0.7f + 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, offHover);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        }
        if (ImGui::Button(active ? onLabel : offLabel, ImVec2(width * m_uiScale, 40.0f * m_uiScale))) {
            setStageLayout(active ? StageLayout::Default : layout);
            m_loopTimer = 0.0f; // a manual pick restarts the loop countdown from this visual
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
    };

    RenderLayoutButton(StageLayout::SpaceVoyage, "SPACE VOYAGE: ON", "SPACE VOYAGE: OFF",
        ImVec4(0.75f, 0.45f, 1.0f, 1.0f), ImVec4(0.55f, 0.3f, 0.85f, 1.0f), 185.0f,
        "Space Voyage layout: a tour of seven destinations with a hyperspace jump every 15 s:\n"
        "Ringed Giant, Asteroid Belt, Nebula Drift, Pulsar, Aurora World, Spiral Galaxy, Event Horizon.\n"
        "DROP [SPACE] jumps to the next destination. Click again for the default visuals.");
    ImGui::SameLine();
    RenderLayoutButton(StageLayout::Chemistry, "CHEMISTRY: ON", "CHEMISTRY: OFF",
        ImVec4(0.3f, 0.95f, 0.45f, 1.0f), ImVec4(0.2f, 0.7f, 0.35f, 1.0f), 165.0f,
        "Chemistry layout: a molecular world dancing to the music. The centerpiece changes every 24 s:\n"
        "DNA Double Helix, Benzene Ring, Salt Crystal, Carbon Atom, surrounded by a swarm of\n"
        "water, CO2, methane and nitrogen molecules that pulse to the spectrum and burst on drops.");
    ImGui::SameLine();
    RenderLayoutButton(StageLayout::Quantum, "QUANTUM: ON", "QUANTUM: OFF",
        ImVec4(1.0f, 0.4f, 0.8f, 1.0f), ImVec4(0.75f, 0.25f, 0.6f, 1.0f), 150.0f,
        "Quantum layout: four quantum phenomena crossfading through a wavefunction collapse (2-minute cycle):\n"
        "Electron Orbitals, Bubble Chamber (a collision every beat) and Double Slit (the mids set the wavelength)\n"
        "for 20 s each, then the Quantum Field (vacuum fluctuations and particle pairs) for 60 s.");
    ImGui::SameLine();

    // Loop Visuals: steps Main -> Space Voyage -> Chemistry -> Quantum on a timer (advanced in updateStageLoop)
    if (m_loopVisuals) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.75f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.85f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.6f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    }
    if (ImGui::Button(m_loopVisuals ? "LOOP VISUALS: ON" : "LOOP VISUALS: OFF", ImVec2(175.0f * m_uiScale, 40.0f * m_uiScale))) {
        m_loopVisuals = !m_loopVisuals;
        m_loopTimer = 0.0f;
    }
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically cycles the stage through the Main screen, Space Voyage, Chemistry and Quantum,\n"
                          "fading through black between them. Keeps running in fullscreen and on the stage window.");
    }
    if (m_loopVisuals) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.0f * m_uiScale);
        ImGui::SliderFloat("##LoopSeconds", &m_loopIntervalSec, 15.0f, 300.0f, "%.0fs each");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Time on each visual before looping to the next");
        }
    }

    ImGui::SameLine();

    // 4. Auto-Pilot Mode / Space Voyage Destination & Live Audio Sync Status
    ImGui::BeginGroup();
    if (m_stageLayout == StageLayout::SpaceVoyage && m_spaceVoyage) {
        if (m_spaceVoyage->isWarping()) {
            ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), "Space Voyage: leaving %s... HYPERSPACE JUMP", m_spaceVoyage->getDestinationName());
        } else {
            ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), "Space Voyage: %s | next jump in %.0fs (DROP = jump now)",
                m_spaceVoyage->getDestinationName(), m_spaceVoyage->getSecondsToNextJump());
        }
    } else if (autoOn) {
        ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "🤖 Auto-Pilot Active (Dynamic Audio-Reactive Director)");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "🎛️ Manual Live Mode (Direct Slider & Toggle Control)");
    }

    // Audio status & BPM
    float bpm = m_beatDetector ? m_beatDetector->getBpm() : 128.0f;
    float pulse = m_beatDetector ? m_beatDetector->getBeatPulse() : 0.0f;
    if (m_loopVisuals) {
        ImGui::TextDisabled("Loop: %s next in %.0fs | Live Audio Sync: %.0f BPM | Beat Pulse: %.2f",
            StageLayoutName(NextLoopLayout(m_stageLayout)), (std::max)(0.0f, m_loopIntervalSec - m_loopTimer), bpm, pulse);
    } else {
        ImGui::TextDisabled("Live Audio Sync: %.0f BPM | Beat Pulse: %.2f", bpm, pulse);
    }
    ImGui::EndGroup();

    // 5. Audio silence warning if no sound detected
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

void DjDashboard::renderVisualMixerPanel() {
    if (ImGui::CollapsingHeader("🎛️ 1. VISUAL INGREDIENTS MIXER", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& user = m_autoPilot->getUserSettings();

        ImGui::TextDisabled("Click any ingredient to inspect its details below. Click [ON]/[OFF] to toggle:");

        auto RenderMixerRow = [&](
            int ingredientIdx,
            const char* depthBadge,
            const char* label,
            const char* id,
            bool& enabled,
            float& level,
            bool isSubItem,
            bool hasSolo
        ) {
            ImGui::PushID(id);

            // 1. Master Illuminated [ ON ] / [ OFF ] Toggle Pill
            if (enabled) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.78f, 0.40f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.90f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.24f, 0.29f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.32f, 0.35f, 0.42f, 0.95f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.63f, 0.70f, 1.0f));
            }

            if (ImGui::Button(enabled ? "ON" : "OFF", ImVec2(44.0f * m_uiScale, 24.0f * m_uiScale))) {
                enabled = !enabled;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();

            // 2. Depth Badge + Hierarchy Indentation
            if (isSubItem) {
                ImGui::TextDisabled(" └");
                ImGui::SameLine();
            }

            bool isSelected = (m_selectedIngredient == ingredientIdx);
            ImVec4 badgeCol = isSelected ? ImVec4(1.0f, 0.75f, 0.0f, 1.0f) : (enabled ? ImVec4(0.2f, 0.85f, 1.0f, 1.0f) : ImVec4(0.45f, 0.48f, 0.55f, 0.7f));
            ImGui::TextColored(badgeCol, "[%-8s]", depthBadge);
            ImGui::SameLine();

            // 3. Selectable Name Label (Clicking selects for details inspector)
            char selectableLabel[128];
            std::snprintf(selectableLabel, sizeof(selectableLabel), "%s%s", isSelected ? "▶ " : "", label);

            ImVec4 labelCol = isSelected ? ImVec4(1.0f, 0.85f, 0.2f, 1.0f) : (enabled ? ImVec4(0.95f, 0.96f, 0.98f, 1.0f) : ImVec4(0.48f, 0.50f, 0.56f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_Text, labelCol);

            float labelWidth = ImGui::GetWindowWidth() - (hasSolo ? 250.0f : 200.0f) * m_uiScale - ImGui::GetCursorPosX();
            if (labelWidth < 80.0f * m_uiScale) labelWidth = 80.0f * m_uiScale;

            if (ImGui::Selectable(selectableLabel, isSelected, 0, ImVec2(labelWidth, 0.0f))) {
                m_selectedIngredient = ingredientIdx;
            }
            ImGui::PopStyleColor();

            // 4. Level Gain Slider (Right-aligned)
            float sliderPos = ImGui::GetWindowWidth() - (hasSolo ? 175.0f : 125.0f) * m_uiScale;
            if (sliderPos > ImGui::GetCursorPosX() + 10.0f) {
                ImGui::SameLine(sliderPos);
            } else {
                ImGui::SameLine();
            }

            ImGui::SetNextItemWidth(85.0f * m_uiScale);
            ImGui::SliderFloat("##Lvl", &level, 0.0f, 1.5f, "%.2fx");

            // 5. Quick Solo Button
            if (hasSolo) {
                ImGui::SameLine();
                if (ImGui::SmallButton("S")) {
                    user.heroTextEnabled = (&enabled == &user.heroTextEnabled);
                    user.triangleEnabled = (&enabled == &user.triangleEnabled);
                    user.sunEnabled = (&enabled == &user.sunEnabled);
                    user.gridEnabled = (&enabled == &user.gridEnabled);
                    user.lasersEnabled = (&enabled == &user.lasersEnabled);
                    user.particlesEnabled = (&enabled == &user.particlesEnabled);
                    enabled = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Solo: isolate this layer exclusively");
                }
            }

            ImGui::PopID();
        };

        // 1. Foremost Depth: Hero Text (Index 0)
        RenderMixerRow(0, "FOREMOST", "🔤 Hero Text Centerpiece", "hero", user.heroTextEnabled, user.heroTextLevel, false, true);

        // 2. Screen Overlay: Vignette & Drop Flash Pulse (Index 1)
        RenderMixerRow(1, "OVERLAY",  "🔲 Vignette & Drop Pulse", "vig",  user.vignetteEnabled, user.vignetteLevel, false, false);

        // 3. Stage Mid: Alive Triangle / Pyramid & Core (Index 2)
        RenderMixerRow(2, "STAGE",    "🔺 Triangle (Alive Pyramid)", "tri",  user.triangleEnabled, user.triangleLevel, false, true);

        // 4. Stage FX: Volumetric Lasers (Index 3)
        RenderMixerRow(3, "STAGE FX", "🔦 Volumetric Apex Lasers", "las", user.lasersEnabled, user.lasersLevel, false, true);

        // 5. Stage FX: Electric Sparks Particles (Index 4)
        RenderMixerRow(4, "STAGE FX", "✨ Electric Stage Sparks", "prt", user.particlesEnabled, user.particlesLevel, false, true);

        // 6. Ground: TRON Grid (Index 5)
        RenderMixerRow(5, "GROUND",   "🌐 TRON Perspective Grid",  "grd", user.gridEnabled, user.gridLevel, false, true);

        // Sub-elements under Grid:
        RenderMixerRow(6, "OBJECTS",  "⛰️ Edge Crystals / Waves",  "cry", user.gridObjectsEnabled, user.gridObjectsLevel, true, false);
        RenderMixerRow(7, "TRAILS",   "⚡ Speed Lines (Trails)",    "spd", user.speedLinesEnabled, user.speedLinesLevel, true, false);

        // 7. Furthest Horizon: Sun / Celestial Centerpiece (Behind grid) (Index 8)
        RenderMixerRow(8, "HORIZON",  "☀️ Sun / Celestial Horizon", "sun", user.sunEnabled, user.sunLevel, false, true);
    }
}

void DjDashboard::renderConsolidatedDetailsPanel() {
    auto& user = m_autoPilot->getUserSettings();

    static const char* kIngredientTitles[] = {
        "🔤 HERO TEXT CENTERPIECE",
        "🔲 VIGNETTE & DROP PULSE",
        "🔺 TRIANGLE (ALIVE PYRAMID)",
        "🔦 VOLUMETRIC APEX LASERS",
        "✨ ELECTRIC STAGE SPARKS",
        "🌐 TRON PERSPECTIVE GRID",
        "⛰️ EDGE CRYSTALS / MOUNTAIN WAVES",
        "⚡ SPEED LINES (LIGHT-CYCLE TRAILS)",
        "☀️ SUN / CELESTIAL HORIZON",
        "🎨 CONCERT PALETTES & SAFETY"
    };

    const char* activeTitle = (m_selectedIngredient >= 0 && m_selectedIngredient < 10)
        ? kIngredientTitles[m_selectedIngredient] : "DETAILS";

    std::string headerStr = std::string("⚙️ 2. DETAILS: ") + activeTitle;
    if (ImGui::CollapsingHeader(headerStr.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

        // Ingredient 0: Hero Text
        if (m_selectedIngredient == 0) {
            ImGui::Checkbox("Authentic 3D Mesh (References 1-3)", &user.useAuthenticLogo);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Authentic 1994-2007 extruded 3D polygonal mesh rendered in foremost depth pass");
            }

            if (user.useAuthenticLogo) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[Foremost 3D Mesh Active]");
            } else {
                ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Custom DJ Vector Font Text (16 chars):");
                ImGui::SetNextItemWidth(240.0f * m_uiScale);
                ImGui::InputText("##CustomText", user.customText, 17);

                ImGui::SameLine();
                if (ImGui::Button("Reset 'DADT PUNK'")) {
                    std::snprintf(user.customText, sizeof(user.customText), "%s", "DADT PUNK");
                }

                const char* fontFaceNames[] = {
                    "0: Neon Sans (Double Glow Laser)",
                    "1: Ryga (Triple-Line Cyber Electro)",
                    "2: Raster Forge (90s Retro Arcade)",
                    "3: Atari Classic (8-Bit Chunky ATASCII)"
                };
                ImGui::SetNextItemWidth(300.0f * m_uiScale);
                ImGui::Combo("##HeroFontFace", &user.fontFace, fontFaceNames, 4);
            }

            ImGui::SliderFloat("Text Size##HeroScale", &user.logoScale, 0.5f, 3.5f, "%.2fx (Scale)");
            ImGui::SliderFloat("Vertical Y##HeroY", &user.logoPosY, -0.6f, 0.8f, "%.2f (Stage Position)");

            const char* logoStyleNames[] = {
                "0: Discovery 3D Liquid Chrome (Reference Image 1)",
                "1: Faceted 3D Crystal Mesh (Reference Image 2)",
                "2: Neon Laser Outline / Cyber Vector",
                "3: Digital Cyber Slices & Scanlines",
                "4: Solid Daft Red / Gold Punk Finish"
            };
            ImGui::SetNextItemWidth(300.0f * m_uiScale);
            ImGui::Combo("##HeroTextStyle", &user.logoStyle, logoStyleNames, 5);
        }
        // Ingredient 1: Vignette
        else if (m_selectedIngredient == 1) {
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "Screen-Border Vignette & Pulse:");
            ImGui::TextDisabled("Darkens perimeter and flashes dynamic glow during musical drops and kicks.");
            ImGui::SliderFloat("Vignette Intensity", &user.vignetteLevel, 0.0f, 1.5f, "%.2fx");
        }
        // Ingredient 2: Triangle (Alive Pyramid)
        else if (m_selectedIngredient == 2) {
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Pyramid Face LED Pattern:");
            const char* patternNames[] = { "0: VU Strips", "1: Chevrons", "2: Scanner Sweep", "3: Solid Glow", "4: Rainbow" };
            ImGui::SetNextItemWidth(260.0f * m_uiScale);
            ImGui::Combo("##PyramidPattern", &user.pyramidPattern, patternNames, 5);

            ImGui::Spacing();
            ImGui::Checkbox("Internal Sub-Bass Octahedron Core", &user.octahedronCoreEnabled);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Pulsing geometric sub-bass diamond glowing inside pyramid frame");
            }
        }
        // Ingredient 3: Lasers
        else if (m_selectedIngredient == 3) {
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "8-Beam Volumetric Apex Lasers:");
            ImGui::TextDisabled("Volumetric fan sweeping forward over the crowd, occluded by the pyramid.");
            ImGui::SliderFloat("Laser Output Intensity", &user.lasersLevel, 0.0f, 1.5f, "%.2fx");
        }
        // Ingredient 4: Particles
        else if (m_selectedIngredient == 4) {
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "Electric Stage Embers:");
            ImGui::TextDisabled("Beat-reactive 3D sparks rising up from the ground plane towards the camera.");
            ImGui::SliderFloat("Particle Density & Glow", &user.particlesLevel, 0.0f, 1.5f, "%.2fx");
        }
        // Ingredient 5 & 6: Grid & Mountain Waves
        else if (m_selectedIngredient == 5 || m_selectedIngredient == 6) {
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Mountain Waves & Edge Crystals:");
            ImGui::SliderFloat("Mountain Wave Height", &user.mountainWaveHeight, 0.0f, 1.5f, user.mountainWaveHeight < 0.02f ? "Flat TRON Grid" : "%.2f (Wave Mountains)");
            ImGui::TextDisabled("Motion direction: Forward back-to-front down the road in lockstep.");
        }
        // Ingredient 7: Speed Lines
        else if (m_selectedIngredient == 7) {
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "TRON Speed Lines (Light-Cycle Trails):");
            ImGui::TextDisabled("High-speed light ribbons shooting down 4 floor lanes toward the horizon.");
            ImGui::SliderFloat("Trail Intensity", &user.speedLinesLevel, 0.0f, 1.5f, "%.2fx");
        }
        // Ingredient 8: Sun / Celestial Horizon
        else if (m_selectedIngredient == 8) {
            ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Celestial Horizon Centerpiece:");
            const char* celestialNames[] = {
                "0: Black Hole Singularity Eclipse",
                "1: Glowing Neon Portal Ring",
                "2: Digital Wireframe Cyber Moon"
            };
            ImGui::SetNextItemWidth(280.0f * m_uiScale);
            ImGui::Combo("##CelestialCenterpiece", &user.celestialMode, celestialNames, 3);
            ImGui::TextDisabled("Sun sits on the horizon line and is naturally culled by the TRON floor.");
        }
    }

    // Always-accessible Concert Palettes & Photosafety
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("🎨 3. CONCERT COLOR PALETTES & SAFETY", ImGuiTreeNodeFlags_DefaultOpen)) {
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

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "🛡️ Elementary-School Safe (Zero Strobe):");
        ImGui::SliderFloat("Master Brightness", &user.masterBrightness, 0.1f, 1.0f, "%.2f (Kid-Safe Cap)");
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

void DjDashboard::openStageWindowOn(int monitorIndex) {
    // Reset the renderer's stage state first: Esc closes the stage window directly, without telling the renderer
    m_renderer->releaseStageSwapChain();
    m_windowManager->closeStageWindow();
    if (m_windowManager->createStageWindow(nullptr, monitorIndex)) {
        m_renderer->setupStageSwapChain(m_windowManager->getStageWindow(), 1920, 1080);
    }
}

void DjDashboard::renderProjectorOutputPanel() {
    if (ImGui::CollapsingHeader("STAGE PROJECTOR & PRESET MANAGER", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Stage Display Picker: displays are re-listed every second so a TV or projector plugged in
        // mid-set shows up; picking one moves an open stage window there right away
        m_monitorRefreshTimer -= ImGui::GetIO().DeltaTime;
        if (m_monitorRefreshTimer <= 0.0f || m_monitors.empty()) {
            m_monitors = m_windowManager->enumerateMonitors();
            m_monitorRefreshTimer = 1.0f;
        }
        int monitorCount = static_cast<int>(m_monitors.size());
        int targetMonitor = std::clamp(m_selectedMonitor, 0, (std::max)(monitorCount - 1, 0));
        auto MonitorLabel = [&](int i) {
            const auto& mon = m_monitors[i];
            char buf[192];
            std::snprintf(buf, sizeof(buf), "%d: %s  %dx%d%s", i + 1, WStringToString(mon.name).c_str(),
                mon.width, mon.height, mon.isPrimary ? "  (main)" : "");
            return std::string(buf);
        };

        bool stageOpen = m_windowManager->isStageWindowOpen();
        if (monitorCount > 0) {
            ImGui::Text("Stage Display:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##StageDisplay", MonitorLabel(targetMonitor).c_str())) {
                for (int i = 0; i < monitorCount; ++i) {
                    ImGui::PushID(i);
                    bool isSel = (i == targetMonitor);
                    if (ImGui::Selectable(MonitorLabel(i).c_str(), isSel)) {
                        m_selectedMonitor = i;
                        targetMonitor = i;
                        if (stageOpen) {
                            openStageWindowOn(i);
                        }
                    }
                    if (isSel) ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Display for the stage output. Changing it while the stage window is open moves it immediately.");
            }
        }

        // Fullscreen Blit / Stage Toggle
        stageOpen = m_windowManager->isStageWindowOpen();
        if (stageOpen) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Close Stage Window", ImVec2(-1, 35.0f * m_uiScale))) {
                m_renderer->releaseStageSwapChain();
                m_windowManager->closeStageWindow();
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
            if (ImGui::Button("Open Stage Window on Selected Display", ImVec2(-1, 35.0f * m_uiScale))) {
                openStageWindowOn(targetMonitor);
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Fills the selected display with the visuals only. Press Esc on it to close.");
            }
        }

        // Single-screen fullscreen (same as F11, which macOS usually reserves for Show Desktop)
        if (ImGui::Button("Fullscreen on Main Display (Esc to exit)", ImVec2(-1, 30.0f * m_uiScale))) {
            m_windowManager->toggleSingleScreenFullscreen();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Hides the dashboard and shows the visuals full screen on the main display. Esc or F11 exits.");
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
                            user.masterBrightness = loaded.masterBrightness;
                            std::snprintf(user.customText, sizeof(user.customText), "%s", loaded.customText.c_str());
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
            saveP.masterBrightness = user.masterBrightness;
            saveP.customText = user.customText;
            m_presetManager->savePreset(saveP);
        }
    }
}

} // namespace Visualizer::UI
