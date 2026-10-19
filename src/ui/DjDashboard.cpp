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

        // Load crisp system font on Windows or macOS, fallback to default font (20px base)
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

        // Check GitHub Releases for updates in the background on startup
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
    style = ImGuiStyle(); // Reset to defaults before applying scaling

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
    colors[ImGuiCol_WindowBg]              = ImVec4(0.04f, 0.05f, 0.07f, 0.35f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.06f, 0.07f, 0.10f, 0.72f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.09f, 0.11f, 0.95f);
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
    colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(1.00f, 0.40f, 0.00f, 0.80f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.85f, 0.30f, 0.00f, 1.00f);
}

void DjDashboard::triggerSpacebarDrop(Graphics::AudioVisualizerParams& params) {
    if (m_autoPilot) {
        m_autoPilot->triggerDropTransition(params);
    }
}

void DjDashboard::render(
    float deltaTime,
    Graphics::AudioVisualizerParams& params
) {
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && !ImGui::GetIO().WantTextInput) {
        triggerSpacebarDrop(params);
    }

    if (std::abs(m_uiScale - m_appliedUiScale) > 0.001f) {
        m_appliedUiScale = m_uiScale;
        ImGui::GetIO().FontGlobalScale = m_uiScale;
        applyCustomTheme(m_uiScale);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_fps = ImGui::GetIO().Framerate;
    m_frameTimeMs = 1000.0f / (m_fps > 0.0f ? m_fps : 60.0f);
    if (m_reloadBannerTimer > 0.0f) m_reloadBannerTimer -= deltaTime;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("DJ Control Console", nullptr, winFlags);

    renderHeaderBar();

    ImGui::Separator();

    renderNextDropQueueBar(params);

    ImGui::Separator();

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    float leftColWidth = contentSize.x * 0.54f;
    float rightColWidth = contentSize.x - leftColWidth - ImGui::GetStyle().ItemSpacing.x;

    // LEFT COLUMN: Visual Ingredient Mixer (Scrollable)
    ImGui::BeginChild("LeftControlsPanel", ImVec2(leftColWidth, contentSize.y), false);
    renderHeroShapeIngredient(params);
    ImGui::Spacing();
    renderBackgroundIngredient(params);
    ImGui::Spacing();
    renderStageFxIngredients(params);
    ImGui::Spacing();
    renderFluidityMotionPanel(params);
    ImGui::Spacing();
    renderVibeRecipesAndColors(params);
    ImGui::EndChild();

    ImGui::SameLine();

    // RIGHT COLUMN: Live Stage Preview, Audio Meters & Projector Output (Scrollable)
    ImGui::BeginChild("RightPreviewPanel", ImVec2(rightColWidth, contentSize.y), false);
    renderLivePreview();
    ImGui::Spacing();
    renderAudioSpectrumPanel();
    ImGui::Spacing();
    renderProjectorOutputPanel();
    ImGui::Spacing();
    renderShaderCompilerStatus();
    ImGui::EndChild();

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DjDashboard::renderHeaderBar() {
    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.0f, 1.0f), "LIVE DJ INGREDIENT MIXER");
    ImGui::SameLine();
    ImGui::TextDisabled("| %s", m_autoUpdater.getCurrentVersion().c_str());

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

    // Right-aligned status, VSync & UI Scale controls
    float rightAreaWidth = 420.0f * m_uiScale;
    float winWidth = ImGui::GetWindowWidth();
    if (winWidth > rightAreaWidth + 320.0f) {
        ImGui::SameLine(winWidth - rightAreaWidth);
    } else {
        ImGui::SameLine();
    }

#if defined(__APPLE__)
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[macOS ARM64]");
#else
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[WIN64 GL3.3]");
#endif
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "%.0f FPS", m_fps);
    ImGui::SameLine();
    ImGui::Checkbox("VSync", &m_vsync);
    ImGui::SameLine();

    const char* scaleLabels[] = { "100%", "125%", "150%", "175%", "200%", "225%", "250%", "300%" };
    const float scaleValues[] = { 1.00f,  1.25f,  1.50f,  1.75f,  2.00f,  2.25f,  2.50f,  3.00f  };
    constexpr int numScales = sizeof(scaleLabels) / sizeof(scaleLabels[0]);

    ImGui::SetNextItemWidth(90.0f * m_uiScale);
    if (ImGui::BeginCombo("Scale", scaleLabels[m_selectedScaleIndex])) {
        for (int i = 0; i < numScales; ++i) {
            bool isSelected = (m_selectedScaleIndex == i);
            if (ImGui::Selectable(scaleLabels[i], isSelected)) {
                m_selectedScaleIndex = i;
                m_uiScale = scaleValues[i];
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void DjDashboard::renderNextDropQueueBar(Graphics::AudioVisualizerParams& params) {
    bool isQuantized = m_autoPilot->isQuantizeToBeat();
    bool hasPending = m_autoPilot->hasPendingChanges();

    const char* bgNames[] = { "Starfield Space", "Warp Tunnel", "Soundwave Void", "Concert Stage", "Obsidian Ocean" };
    const char* shapeNames[] = { "Liquid Chrome", "Alive Pyramid", "TRON Gyroscope", "Obsidian Monolith", "Cyber Torus Knot", "4D Tesseract", "Geodesic Sphere", "Prism Spire", "No Shape" };

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.12f, 0.16f, 0.95f));
    ImGui::BeginChild("DropQueueBar", ImVec2(-1, 62.0f * m_uiScale), true);

    // 1. Drop Now (Spacebar) Action Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.35f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.55f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    if (ImGui::Button("⚡ DROP NOW [SPACEBAR]", ImVec2(210.0f * m_uiScale, 44.0f * m_uiScale))) {
        triggerSpacebarDrop(params);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // 2. Quantize to Beat Toggle
    if (isQuantized) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    std::string quantizeLabel = isQuantized ? "⏱️ QUANTIZE: ON (Next Beat)" : "⏱️ QUANTIZE: OFF (Instant)";
    if (ImGui::Button(quantizeLabel.c_str(), ImVec2(215.0f * m_uiScale, 44.0f * m_uiScale))) {
        m_autoPilot->setQuantizeToBeat(!isQuantized);
    }
    if (isQuantized) {
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();

    // 3. Status Display (Pending Changes vs Upcoming Drop)
    if (hasPending) {
        const auto& pending = m_autoPilot->getPendingRecipe();
        std::string bgStr = (pending.backgroundType >= 0 && pending.backgroundType < 5) ? bgNames[pending.backgroundType] : "Custom";
        std::string shapeStr = (pending.shapeType >= 0 && pending.shapeType < 9) ? shapeNames[pending.shapeType] : "None";

        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "⚡ STAGED CHANGE (Queued to Next Beat)");
        ImGui::TextDisabled("Mix: [%s] + [%s] (Hit Spacebar to drop immediately)", shapeStr.c_str(), bgStr.c_str());
        ImGui::EndGroup();
    } else {
        const auto& nextRecipe = m_autoPilot->getQueuedRecipe();
        std::string bgStr = (nextRecipe.backgroundType >= 0 && nextRecipe.backgroundType < 5) ? bgNames[nextRecipe.backgroundType] : "Custom";
        std::string shapeStr = (nextRecipe.shapeType >= 0 && nextRecipe.shapeType < 9) ? shapeNames[nextRecipe.shapeType] : "None";

        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "NEXT AUTO-DROP: \"%s\"", nextRecipe.name.c_str());
        ImGui::TextDisabled("Mix: [%s] + [%s] | Lasers: %.1fx", shapeStr.c_str(), bgStr.c_str(), nextRecipe.laserIntensity);
        ImGui::EndGroup();
    }

    ImGui::SameLine();
    if (ImGui::Button("🎲 Re-Roll Drop", ImVec2(120.0f * m_uiScale, 44.0f * m_uiScale))) {
        m_autoPilot->rollNextQueuedRecipe();
    }

    ImGui::SameLine();
    bool autoP = m_autoPilot->isEnabled();
    if (ImGui::Checkbox("Auto-Pilot Mode", &autoP)) {
        m_autoPilot->setEnabled(autoP);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void DjDashboard::renderHeroShapeIngredient(Graphics::AudioVisualizerParams& params) {
    if (ImGui::CollapsingHeader("1. HERO 3D SHAPE & DUPLICATION MODIFIERS", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* shapeLabels[] = {
            "🔺 Alive Pyramid",
            "⛩️ TRON Recognizer",
            "🪐 Identity Disc Gyro",
            "🎯 Holo HUD Shield",
            "🧊 4D Tesseract",
            "🏛️ Data Monoliths",
            "🪢 Light-Ribbon Knot",
            "💧 Liquid Mercury",
            "🚫 No Shape (Stage)"
        };

        int activeShape = params.ShapeType;
        if (m_autoPilot->isQuantizeToBeat() && m_autoPilot->hasPendingChanges()) {
            activeShape = m_autoPilot->getPendingRecipe().shapeType;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * m_uiScale, 6.0f * m_uiScale));
        
        // 3x3 Button Grid for Hero Shapes
        for (int i = 0; i < 9; ++i) {
            bool isCurrent = (activeShape == i);
            if (isCurrent) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.45f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            }

            if (ImGui::Button(shapeLabels[i])) {
                if (m_autoPilot->isQuantizeToBeat()) {
                    Control::VisualizerPreset staged;
                    if (m_autoPilot->hasPendingChanges()) staged = m_autoPilot->getPendingRecipe();
                    else staged.captureFromParams(params, "ModularMixer", m_audioAnalyzer->getGain(), m_autoPilot->isEnabled());
                    staged.shapeType = i;
                    m_autoPilot->stageRecipe(staged);
                } else {
                    params.ShapeType = i;
                }
            }

            if (isCurrent) {
                ImGui::PopStyleColor(2);
            }

            if ((i + 1) % 3 != 0 && i < 8) {
                ImGui::SameLine();
            }
        }

        // Surface Material Mode: 4-button row
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Surface Material Mode:");
        const char* matLabels[] = {
            "🖤 Smoked Obsidian (TRON)",
            "🎛️ Alive LED Matrix",
            "📟 Sci-Fi Holo HUD",
            "🪞 Liquid Chrome"
        };
        int activeMat = params.MaterialMode;
        if (m_autoPilot->isQuantizeToBeat() && m_autoPilot->hasPendingChanges()) {
            activeMat = m_autoPilot->getPendingRecipe().materialMode;
        }
        for (int m = 0; m < 4; ++m) {
            bool isCurrentMat = (activeMat == m);
            if (isCurrentMat) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.45f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            }

            if (ImGui::Button(matLabels[m])) {
                if (m_autoPilot->isQuantizeToBeat()) {
                    Control::VisualizerPreset staged;
                    if (m_autoPilot->hasPendingChanges()) staged = m_autoPilot->getPendingRecipe();
                    else staged.captureFromParams(params, "ModularMixer", m_audioAnalyzer->getGain(), m_autoPilot->isEnabled());
                    staged.materialMode = m;
                    m_autoPilot->stageRecipe(staged);
                } else {
                    params.MaterialMode = m;
                }
            }

            if (isCurrentMat) {
                ImGui::PopStyleColor(2);
            }

            if (m < 3) {
                ImGui::SameLine();
            }
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "Shape Duplication / Multi-Array Modifier:");
        
        const char* dupLabels[] = {
            "Single Shape",
            "🪐 Orbital Ring",
            "↔️ Triad Array",
            "❄️ Kaleidoscope",
            "⏩ Depth Tunnel"
        };

        int activeDup = params.DuplicationMode;
        for (int d = 0; d < 5; ++d) {
            bool isCurrentDup = (activeDup == d);
            if (isCurrentDup) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.75f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            }

            if (ImGui::Button(dupLabels[d])) {
                params.DuplicationMode = d;
            }

            if (isCurrentDup) {
                ImGui::PopStyleColor(2);
            }

            if (d < 4) {
                ImGui::SameLine();
            }
        }

        if (params.DuplicationMode > 0) {
            ImGui::SetNextItemWidth(150.0f * m_uiScale);
            ImGui::SliderFloat("Orbit / Spacing", &params.DuplicationParam0, 0.3f, 2.5f, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150.0f * m_uiScale);
            ImGui::SliderFloat("Satellite Scale", &params.DuplicationParam1, 0.2f, 1.0f, "%.2f");
        }

        ImGui::PopStyleVar();
    }
}

void DjDashboard::renderBackgroundIngredient(Graphics::AudioVisualizerParams& params) {
    if (ImGui::CollapsingHeader("2. COMPOSABLE 3D ENVIRONMENT STACK (MULTI-LAYER)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * m_uiScale, 6.0f * m_uiScale));

        auto renderEnvLayer = [this](const char* checkId, const char* label, float& weight) {
            bool active = weight > 0.01f;
            if (ImGui::Checkbox(checkId, &active)) {
                weight = active ? 1.0f : 0.0f;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(260.0f * m_uiScale);
            ImGui::SliderFloat(label, &weight, 0.0f, 1.5f, "%.2f");
        };

        renderEnvLayer("##SkyCheck", "🌌 Deep Space & Nebula", params.SkyLayerWeight);
        renderEnvLayer("##WarpCheck", "🌀 Hyperdrive Warp Streaks", params.WarpLayerWeight);
        renderEnvLayer("##FloorCheck", "🏁 3D TRON Grid & Obsidian Mirror Floor", params.FloorLayerWeight);
        renderEnvLayer("##SoundwaveCheck", "📊 3D Audio Soundwave & FFT Horizon", params.SoundwaveLayerWeight);
        renderEnvLayer("##TrussCheck", "🏟️ Overhead Concert Truss Rig", params.StageTrussWeight);

        ImGui::PopStyleVar();
    }
}

void DjDashboard::renderStageFxIngredients(Graphics::AudioVisualizerParams& params) {
    if (ImGui::CollapsingHeader("3. STAGE FX & ATMOSPHERE STACK", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Volumetric Lasers Toggle & Power (Always unlocked and visible across environments)
        bool lasersOn = params.LaserIntensity > 0.05f;
        if (ImGui::Checkbox("⚡ Volumetric Concert Lasers", &lasersOn)) {
            params.LaserIntensity = lasersOn ? 1.0f : 0.0f;
        }

        if (lasersOn) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(130.0f * m_uiScale);
            ImGui::SliderFloat("Power##Lasers", &params.LaserIntensity, 0.1f, 2.0f, "%.1fx");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(110.0f * m_uiScale);
            ImGui::SliderFloat("Fan##Lasers", &params.CustomParam6, -1.0f, 1.0f, "%.1f");
        }

        // Sci-Fi HUD Telemetry Overlay
        bool hudOn = params.HudLayerWeight > 0.01f;
        if (ImGui::Checkbox("##HudCheck", &hudOn)) {
            params.HudLayerWeight = hudOn ? 0.6f : 0.0f;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(260.0f * m_uiScale);
        ImGui::SliderFloat("🎯 Sci-Fi HUD Telemetry Overlay", &params.HudLayerWeight, 0.0f, 1.5f, "%.2f");

        // Particle Swarm
        bool particlesOn = params.ParticleIntensity > 0.05f;
        if (ImGui::Checkbox("Treble Spark Dust", &particlesOn)) {
            params.ParticleIntensity = particlesOn ? 1.0f : 0.0f;
        }
        if (particlesOn) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(140.0f * m_uiScale);
            ImGui::SliderFloat("Spark Density", &params.ParticleIntensity, 0.1f, 2.0f, "%.1fx");
        }

        ImGui::SliderFloat("Bloom Glow", &params.BloomIntensity, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Glitch Burst Rate", &params.GlitchRate, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Wireframe / LED Density", &params.WireframeDensity, 0.1f, 3.0f, "%.2f");
    }
}

void DjDashboard::renderFluidityMotionPanel(Graphics::AudioVisualizerParams& params) {
    if (ImGui::CollapsingHeader("4. MACRO DYNAMICS & MOTION ENGINE", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * m_uiScale, 8.0f * m_uiScale));

        // Macro Button 1: Bass Vortex
        bool vortexActive = (std::abs(params.VortexTwist) > 0.1f);
        if (vortexActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.35f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        if (ImGui::Button(vortexActive ? "🌀 BASS VORTEX: ON" : "🌀 BASS VORTEX: OFF")) {
            params.VortexTwist = vortexActive ? 0.0f : 1.2f;
        }
        if (vortexActive) ImGui::PopStyleColor(2);

        ImGui::SameLine();

        // Macro Button 2: Fluid Morph
        bool fluidActive = (params.FluidityAmount > 0.1f);
        if (fluidActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        if (ImGui::Button(fluidActive ? "💧 FLUID MORPH: ON" : "💧 FLUID MORPH: OFF")) {
            params.FluidityAmount = fluidActive ? 0.0f : 1.2f;
        }
        if (fluidActive) ImGui::PopStyleColor(2);

        ImGui::SameLine();

        // Macro Button 3: Camera Drift
        bool driftActive = (params.CameraDrift > 0.5f);
        if (driftActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.85f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        if (ImGui::Button(driftActive ? "🎥 CAMERA DRIFT: ON" : "🎥 CAMERA DRIFT: OFF")) {
            params.CameraDrift = driftActive ? 0.0f : 1.0f;
            if (!driftActive) params.CustomParam4 = 0.0f;
        }
        if (driftActive) ImGui::PopStyleColor(2);

        ImGui::PopStyleVar();

        // Compact 1-row Speed & Camera Distance
        ImGui::Spacing();
        ImGui::SetNextItemWidth(170.0f * m_uiScale);
        ImGui::SliderFloat("Speed", &params.CustomParam1, 0.2f, 3.5f, "%.2fx");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f * m_uiScale);
        ImGui::SliderFloat("Zoom", &params.CustomParam0, 0.3f, 3.0f, "%.2fx");
    }
}

void DjDashboard::renderVibeRecipesAndColors(Graphics::AudioVisualizerParams& params) {
    if (ImGui::CollapsingHeader("5. DAFT PUNK VIBES & PALETTE BRANDING", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("1-Click Daft & TRON Concert Vibes:");

        auto applyVibe = [&](const Control::VisualizerPreset& p) {
            if (m_autoPilot->isQuantizeToBeat()) {
                m_autoPilot->stageRecipe(p);
            } else {
                p.applyToParams(params);
            }
        };

        if (ImGui::Button("🔺 Daft Alive (2007)")) {
            applyVibe(Control::PresetManager::createDaftAlivePreset());
        }
        ImGui::SameLine();
        if (ImGui::Button("💠 TRON Light Ribbon")) {
            applyVibe(Control::PresetManager::createLiquidCyberPreset());
        }
        ImGui::SameLine();
        if (ImGui::Button("🌐 Cyber Horizon")) {
            applyVibe(Control::PresetManager::createSynthwaveHorizonPreset());
        }

        if (ImGui::Button("🪐 Gyro Laser Arena")) {
            applyVibe(Control::PresetManager::createGyroLaserArenaPreset());
        }
        ImGui::SameLine();
        if (ImGui::Button("💧 Liquid Mercury")) {
            applyVibe(Control::PresetManager::createAcidPlasmaPreset());
        }
        ImGui::SameLine();
        if (ImGui::Button("🎲 Random Recipe")) {
            applyVibe(Control::PresetManager::createRandomDropRecipe());
        }

        ImGui::Separator();
        float colP[3] = { params.ColorPrimary.x, params.ColorPrimary.y, params.ColorPrimary.z };
        if (ImGui::ColorEdit3("Primary Color", colP)) {
            params.ColorPrimary = { colP[0], colP[1], colP[2], 1.0f };
        }

        float colS[3] = { params.ColorSecondary.x, params.ColorSecondary.y, params.ColorSecondary.z };
        if (ImGui::ColorEdit3("Secondary Color", colS)) {
            params.ColorSecondary = { colS[0], colS[1], colS[2], 1.0f };
        }

        float colA[3] = { params.ColorAccent.x, params.ColorAccent.y, params.ColorAccent.z };
        if (ImGui::ColorEdit3("Accent / Highlight", colA)) {
            params.ColorAccent = { colA[0], colA[1], colA[2], 1.0f };
        }

        // Preset Save/Load
        ImGui::Separator();
        if (!m_presetList.empty()) {
            if (m_currentPresetIndex >= static_cast<int>(m_presetList.size())) m_currentPresetIndex = 0;
            const char* currentName = m_presetList[m_currentPresetIndex].c_str();

            if (ImGui::BeginCombo("Load Saved Recipe", currentName)) {
                for (int i = 0; i < static_cast<int>(m_presetList.size()); ++i) {
                    bool isSelected = (m_currentPresetIndex == i);
                    if (ImGui::Selectable(m_presetList[i].c_str(), isSelected)) {
                        m_currentPresetIndex = i;
                        Control::VisualizerPreset p;
                        if (m_presetManager->loadPreset(m_presetList[i], p)) {
                            applyVibe(p);
                            m_audioAnalyzer->setGain(p.audioGain);
                            m_autoPilot->setEnabled(p.autoPilot);
                        }
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        ImGui::InputText("Save Recipe As", m_newPresetName, sizeof(m_newPresetName));
        if (ImGui::Button("Save Current Mix as Recipe", ImVec2(260 * m_uiScale, 26 * m_uiScale))) {
            Control::VisualizerPreset p;
            p.name = m_newPresetName;
            p.captureFromParams(params, "ModularMixer", m_audioAnalyzer->getGain(), m_autoPilot->isEnabled());
            m_presetManager->savePreset(p);
            m_presetList = m_presetManager->getPresetNames();
        }
    }
}

void DjDashboard::renderLivePreview() {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "LIVE STAGE PREVIEW (Full Res 1080p)");

    GLuint srv = m_renderer->getFinalSRV();
    if (srv) {
        float availW = ImGui::GetContentRegionAvail().x;
        float previewH = availW * (9.0f / 16.0f);
        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(srv)), ImVec2(availW, previewH), ImVec2(0, 1), ImVec2(1, 0));
    }
}

void DjDashboard::renderAudioSpectrumPanel() {
    if (ImGui::CollapsingHeader("AUDIO MONITOR & EQUALIZER", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Audio Device Dropdown
        if (!m_audioDevices.empty()) {
            if (m_selectedAudioDevice >= static_cast<int>(m_audioDevices.size())) m_selectedAudioDevice = 0;
            std::string curDevName = WStringToString(m_audioDevices[m_selectedAudioDevice].name);

            if (ImGui::BeginCombo("Audio Source", curDevName.c_str())) {
                for (int i = 0; i < static_cast<int>(m_audioDevices.size()); ++i) {
                    std::string devName = WStringToString(m_audioDevices[i].name);
                    if (ImGui::Selectable(devName.c_str(), m_selectedAudioDevice == i)) {
                        m_selectedAudioDevice = i;
                        m_audioCapture->start(m_audioDevices[i].id, m_audioDevices[i].isLoopback);
                    }
                }
                ImGui::EndCombo();
            }
        }

        const auto& bands = m_audioAnalyzer->getBands();

        // 5-Band Audio Meter Bars
        auto renderMeter = [this](const char* name, float val, const ImVec4& col) {
            ImGui::Text("%-8s", name);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
            ImGui::ProgressBar((std::min)(val, 1.0f), ImVec2(-1, 16.0f * m_uiScale), "");
            ImGui::PopStyleColor();
        };

        renderMeter("SUB BASS", bands.subBass, ImVec4(1.0f, 0.1f, 0.2f, 1.0f));
        renderMeter("BASS/KICK", bands.bass, ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
        renderMeter("LOW MID", bands.lowMid, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
        renderMeter("HIGH MID", bands.highMid, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
        renderMeter("TREBLE", bands.treble, ImVec4(0.8f, 0.2f, 1.0f, 1.0f));

        // Beat pulse & BPM display
        float pulse = m_beatDetector->getBeatPulse();
        ImGui::Text("BEAT PULSE: ");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
        ImGui::ProgressBar(pulse, ImVec2(140.0f * m_uiScale, 16.0f * m_uiScale), "");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "BPM: ~%.0f", m_beatDetector->getBpm());

        ImGui::SameLine();
        if (ImGui::SmallButton("Tap Tempo")) {
            m_beatDetector->tapTempo();
        }
    }
}

void DjDashboard::renderProjectorOutputPanel() {
    if (ImGui::CollapsingHeader("PROJECTOR / STAGE OUTPUT (DISPLAY 2)", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!m_windowManager) return;

        if (m_monitors.size() > 1) {
            std::string curMonLabel = "Display " + std::to_string(m_selectedMonitor + 1);
            if (m_selectedMonitor < static_cast<int>(m_monitors.size())) {
                curMonLabel += " (" + std::to_string(m_monitors[m_selectedMonitor].width) + "x" + std::to_string(m_monitors[m_selectedMonitor].height) + ")";
            }

            if (ImGui::BeginCombo("Target Screen", curMonLabel.c_str())) {
                for (int i = 0; i < static_cast<int>(m_monitors.size()); ++i) {
                    std::string label = "Display " + std::to_string(i + 1) + (m_monitors[i].isPrimary ? " [Primary]" : " [Stage/Projector]");
                    label += " (" + std::to_string(m_monitors[i].width) + "x" + std::to_string(m_monitors[i].height) + ")";
                    if (ImGui::Selectable(label.c_str(), m_selectedMonitor == i)) {
                        m_selectedMonitor = i;
                    }
                }
                ImGui::EndCombo();
            }
        } else {
            ImGui::TextDisabled("Single monitor detected. Connecting a projector/display 2 will auto-list here.");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            m_monitors = m_windowManager->enumerateMonitors();
        }

        bool isStageOpen = m_windowManager->isStageWindowOpen();
        if (isStageOpen) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button("❌ CLOSE STAGE OUTPUT", ImVec2(-1, 36.0f * m_uiScale))) {
                m_renderer->releaseStageSwapChain();
                m_windowManager->closeStageWindow();
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("🖥️ LAUNCH FULLSCREEN STAGE OUTPUT", ImVec2(-1, 36.0f * m_uiScale))) {
                if (m_windowManager->createStageWindow(nullptr, m_selectedMonitor)) {
                    m_renderer->setupStageSwapChain(
                        m_windowManager->getStageWindow(),
                        m_windowManager->getStageWidth(),
                        m_windowManager->getStageHeight()
                    );
                }
            }
            ImGui::PopStyleColor();
        }

        ImGui::TextDisabled("Tip: Press [F11] to toggle fullscreen presentation mode on primary display.");
    }
}

void DjDashboard::renderShaderCompilerStatus() {
    if (ImGui::CollapsingHeader("HLSL LIVE SHADER HOT-RELOAD")) {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "✓ Live Hot-Reload Active (<50ms reload on Ctrl+S)");
        ImGui::TextDisabled("Shaders path: src/shaders/*.hlsl");

        auto names = m_renderer->getShaderCompiler()->getShaderNames();
        for (const auto& name : names) {
            std::string err = m_renderer->getShaderCompiler()->getLastError(name);
            if (!err.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "[Error] %s:", name.c_str());
                ImGui::TextWrapped("%s", err.c_str());
            }
        }
    }
}

} // namespace Visualizer::UI
