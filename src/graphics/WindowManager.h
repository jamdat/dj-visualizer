#pragma once

#include <string>
#include <vector>
#include <functional>

struct GLFWwindow;
struct GLFWmonitor;

namespace Visualizer::Graphics {

struct MonitorInfo {
    int index = 0;
    std::wstring name;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool isPrimary = false;
};

class WindowManager {
public:
    using ResizeCallback = std::function<void(int width, int height, bool isStage)>;
    using KeyCallback = std::function<void(int key, int action)>;

    WindowManager();
    ~WindowManager();

    // Enumerate connected monitors (Projectors / Secondary screens)
    std::vector<MonitorInfo> enumerateMonitors();

    // Create main DJ control panel window (Cross-platform GLFW + OpenGL 3.3 Core)
    bool createControlWindow(void* hInstance = nullptr, int width = 1600, int height = 900, const wchar_t* title = L"DJ Audio-Reactive Visualizer Console");
    void showControlWindow(bool maximized = true);

    // Create or toggle dedicated Stage Output window on target monitor (shares OpenGL textures with Control window)
    bool createStageWindow(void* hInstance = nullptr, int monitorIndex = 1);
    void closeStageWindow();
    bool isStageWindowOpen() const;

    // Toggle single-screen fullscreen presentation mode (F11)
    void toggleSingleScreenFullscreen();
    bool isSingleScreenFullscreen() const { return m_isSingleScreenFullscreen; }

    GLFWwindow* getControlWindow() const { return m_controlWindow; }
    GLFWwindow* getStageWindow() const { return m_stageWindow; }

    // Legacy compatibility accessors
    GLFWwindow* getControlHWND() const { return m_controlWindow; }
    GLFWwindow* getStageHWND() const { return m_stageWindow; }

    int getControlWidth() const { return m_controlWidth; }
    int getControlHeight() const { return m_controlHeight; }

    int getStageWidth() const { return m_stageWidth; }
    int getStageHeight() const { return m_stageHeight; }

    bool processMessages();

    void setResizeCallback(ResizeCallback cb) { m_resizeCallback = std::move(cb); }
    void setKeyCallback(KeyCallback cb) { m_keyCallback = std::move(cb); }

    void saveWindowConfig();
    void loadWindowConfig();

    float getUiScale() const { return m_uiScale; }
    void setUiScale(float scale) { m_uiScale = scale; saveWindowConfig(); }
    bool wasMaximizedSaved() const { return m_isMaximized; }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    static void WindowPosCallback(GLFWwindow* window, int x, int y);
    static void WindowMaximizeCallback(GLFWwindow* window, int maximized);
    static void WindowCloseCallback(GLFWwindow* window);
    static void WindowKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_controlWindow = nullptr;
    GLFWwindow* m_stageWindow = nullptr;

    int m_controlWidth = 1600;
    int m_controlHeight = 900;
    int m_controlPosX = 100;
    int m_controlPosY = 100;
    bool m_isMaximized = false;
    float m_uiScale = 1.0f;

    int m_stageWidth = 1920;
    int m_stageHeight = 1080;

    bool m_isSingleScreenFullscreen = false;
    int m_savedX = 100;
    int m_savedY = 100;
    int m_savedW = 1600;
    int m_savedH = 900;

    ResizeCallback m_resizeCallback;
    KeyCallback m_keyCallback;
    static WindowManager* s_instance;
};

} // namespace Visualizer::Graphics
