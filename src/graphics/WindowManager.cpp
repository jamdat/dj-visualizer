#include "WindowManager.h"
#include "GL33Loader.h"
#include "../core/Logger.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace Visualizer::Graphics {

WindowManager* WindowManager::s_instance = nullptr;

#if !defined(__APPLE__)
PFNGLACTIVETEXTUREPROC        glActiveTexture = nullptr;
PFNGLCREATESHADERPROC         glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC         glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC        glCompileShader = nullptr;
PFNGLGETSHADERIVPROC          glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC     glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC         glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC        glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC         glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC          glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC         glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC    glGetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC           glUseProgram = nullptr;
PFNGLDELETEPROGRAMPROC        glDeleteProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC   glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC            glUniform1i = nullptr;
PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex = nullptr;
PFNGLUNIFORMBLOCKBINDINGPROC  glUniformBlockBinding = nullptr;
PFNGLGENBUFFERSPROC           glGenBuffers = nullptr;
PFNGLDELETEBUFFERSPROC        glDeleteBuffers = nullptr;
PFNGLBINDBUFFERPROC           glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC           glBufferData = nullptr;
PFNGLBUFFERSUBDATAPROC        glBufferSubData = nullptr;
PFNGLBINDBUFFERBASEPROC       glBindBufferBase = nullptr;
PFNGLGENVERTEXARRAYSPROC      glGenVertexArrays = nullptr;
PFNGLDELETEVERTEXARRAYSPROC   glDeleteVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC      glBindVertexArray = nullptr;
PFNGLGENFRAMEBUFFERSPROC      glGenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC   glDeleteFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC      glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
PFNGLGENSAMPLERSPROC          glGenSamplers = nullptr;
PFNGLDELETESAMPLERSPROC       glDeleteSamplers = nullptr;
PFNGLBINDSAMPLERPROC          glBindSampler = nullptr;
PFNGLSAMPLERPARAMETERIPROC    glSamplerParameteri = nullptr;

bool InitGL33Loader() {
    auto load = [](const char* name) -> void* {
        return reinterpret_cast<void*>(glfwGetProcAddress(name));
    };
    glActiveTexture        = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(load("glActiveTexture"));
    glCreateShader         = reinterpret_cast<PFNGLCREATESHADERPROC>(load("glCreateShader"));
    glShaderSource         = reinterpret_cast<PFNGLSHADERSOURCEPROC>(load("glShaderSource"));
    glCompileShader        = reinterpret_cast<PFNGLCOMPILESHADERPROC>(load("glCompileShader"));
    glGetShaderiv          = reinterpret_cast<PFNGLGETSHADERIVPROC>(load("glGetShaderiv"));
    glGetShaderInfoLog     = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(load("glGetShaderInfoLog"));
    glDeleteShader         = reinterpret_cast<PFNGLDELETESHADERPROC>(load("glDeleteShader"));
    glCreateProgram        = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(load("glCreateProgram"));
    glAttachShader         = reinterpret_cast<PFNGLATTACHSHADERPROC>(load("glAttachShader"));
    glLinkProgram          = reinterpret_cast<PFNGLLINKPROGRAMPROC>(load("glLinkProgram"));
    glGetProgramiv         = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(load("glGetProgramiv"));
    glGetProgramInfoLog    = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(load("glGetProgramInfoLog"));
    glUseProgram           = reinterpret_cast<PFNGLUSEPROGRAMPROC>(load("glUseProgram"));
    glDeleteProgram        = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(load("glDeleteProgram"));
    glGetUniformLocation   = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(load("glGetUniformLocation"));
    glUniform1i            = reinterpret_cast<PFNGLUNIFORM1IPROC>(load("glUniform1i"));
    glGetUniformBlockIndex = reinterpret_cast<PFNGLGETUNIFORMBLOCKINDEXPROC>(load("glGetUniformBlockIndex"));
    glUniformBlockBinding  = reinterpret_cast<PFNGLUNIFORMBLOCKBINDINGPROC>(load("glUniformBlockBinding"));
    glGenBuffers           = reinterpret_cast<PFNGLGENBUFFERSPROC>(load("glGenBuffers"));
    glDeleteBuffers        = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(load("glDeleteBuffers"));
    glBindBuffer           = reinterpret_cast<PFNGLBINDBUFFERPROC>(load("glBindBuffer"));
    glBufferData           = reinterpret_cast<PFNGLBUFFERDATAPROC>(load("glBufferData"));
    glBufferSubData        = reinterpret_cast<PFNGLBUFFERSUBDATAPROC>(load("glBufferSubData"));
    glBindBufferBase       = reinterpret_cast<PFNGLBINDBUFFERBASEPROC>(load("glBindBufferBase"));
    glGenVertexArrays      = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(load("glGenVertexArrays"));
    glDeleteVertexArrays   = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(load("glDeleteVertexArrays"));
    glBindVertexArray      = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(load("glBindVertexArray"));
    glGenFramebuffers      = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(load("glGenFramebuffers"));
    glDeleteFramebuffers   = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(load("glDeleteFramebuffers"));
    glBindFramebuffer      = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(load("glBindFramebuffer"));
    glFramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(load("glFramebufferTexture2D"));
    glCheckFramebufferStatus = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(load("glCheckFramebufferStatus"));
    glGenSamplers          = reinterpret_cast<PFNGLGENSAMPLERSPROC>(load("glGenSamplers"));
    glDeleteSamplers       = reinterpret_cast<PFNGLDELETESAMPLERSPROC>(load("glDeleteSamplers"));
    glBindSampler          = reinterpret_cast<PFNGLBINDSAMPLERPROC>(load("glBindSampler"));
    glSamplerParameteri    = reinterpret_cast<PFNGLSAMPLERPARAMETERIPROC>(load("glSamplerParameteri"));

    return glCreateProgram != nullptr && glGenFramebuffers != nullptr && glGenVertexArrays != nullptr;
}
#endif

static std::string WideToUtf8(const wchar_t* wstr) {
    if (!wstr) return "DJ Audio-Reactive Visualizer Console";
    std::string out;
    while (*wstr) {
        wchar_t wc = *wstr++;
        if (wc < 0x80) out.push_back(static_cast<char>(wc));
    }
    return out.empty() ? "DJ Audio-Reactive Visualizer Console" : out;
}

static std::wstring Utf8ToWide(const char* str) {
    if (!str) return L"Display";
    std::wstring out;
    while (*str) {
        out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*str++)));
    }
    return out;
}

WindowManager::WindowManager() {
    s_instance = this;
}

WindowManager::~WindowManager() {
    closeStageWindow();
    if (m_controlWindow) {
        glfwDestroyWindow(m_controlWindow);
        m_controlWindow = nullptr;
    }
    glfwTerminate();
    s_instance = nullptr;
}

std::vector<MonitorInfo> WindowManager::enumerateMonitors() {
    std::vector<MonitorInfo> monitors;
    int count = 0;
    GLFWmonitor** glfwMons = glfwGetMonitors(&count);
    GLFWmonitor* primary = glfwGetPrimaryMonitor();

    for (int i = 0; i < count; ++i) {
        GLFWmonitor* m = glfwMons[i];
        const GLFWvidmode* mode = glfwGetVideoMode(m);
        int mx = 0, my = 0;
        glfwGetMonitorPos(m, &mx, &my);
        const char* name = glfwGetMonitorName(m);

        MonitorInfo info;
        info.index = i;
        info.name = Utf8ToWide(name);
        info.x = mx;
        info.y = my;
        info.width = mode ? mode->width : 1920;
        info.height = mode ? mode->height : 1080;
        info.isPrimary = (m == primary);
        monitors.push_back(info);
    }

    if (monitors.empty()) {
        MonitorInfo fallback;
        fallback.index = 0;
        fallback.name = L"Primary Display";
        fallback.width = 1920;
        fallback.height = 1080;
        fallback.isPrimary = true;
        monitors.push_back(fallback);
    }

    return monitors;
}

bool WindowManager::createControlWindow(void* /*hInstance*/, int width, int height, const wchar_t* title) {
    if (!glfwInit()) {
        Log("[ERROR] glfwInit() failed");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    m_controlWidth = width;
    m_controlHeight = height;

    std::string utf8Title = WideToUtf8(title);
    m_controlWindow = glfwCreateWindow(width, height, utf8Title.c_str(), nullptr, nullptr);
    if (!m_controlWindow) {
        Log("[ERROR] glfwCreateWindow failed for Control Window");
        return false;
    }

    glfwMakeContextCurrent(m_controlWindow);
    if (!InitGL33Loader()) {
        Log("[ERROR] InitGL33Loader failed");
        return false;
    }

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(m_controlWindow, &fbW, &fbH);
    if (fbW > 0 && fbH > 0) {
        m_controlWidth = fbW;
        m_controlHeight = fbH;
    }

    glfwSetFramebufferSizeCallback(m_controlWindow, FramebufferSizeCallback);
    glfwSetKeyCallback(m_controlWindow, WindowKeyCallback);

    return true;
}

void WindowManager::showControlWindow(bool maximized) {
    if (!m_controlWindow) return;
    glfwShowWindow(m_controlWindow);
    if (maximized) {
        glfwMaximizeWindow(m_controlWindow);
    }
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(m_controlWindow, &fbW, &fbH);
    if (fbW > 0 && fbH > 0) {
        m_controlWidth = fbW;
        m_controlHeight = fbH;
    }
}

bool WindowManager::createStageWindow(void* /*hInstance*/, int monitorIndex) {
    closeStageWindow();

    auto monitors = enumerateMonitors();
    if (monitors.empty()) return false;

    MonitorInfo targetMon = monitors[0];
    if (monitorIndex >= 0 && monitorIndex < static_cast<int>(monitors.size())) {
        targetMon = monitors[monitorIndex];
    } else if (monitors.size() > 1) {
        targetMon = monitors[1];
    }

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    // Share OpenGL textures/buffers with m_controlWindow
    m_stageWindow = glfwCreateWindow(
        targetMon.width,
        targetMon.height,
        "DJ Stage Output",
        nullptr,
        m_controlWindow
    );

    // Restore default hints
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);

    if (!m_stageWindow) {
        Log("[ERROR] Failed to create Stage Output GLFW window");
        return false;
    }

    glfwSetWindowPos(m_stageWindow, targetMon.x, targetMon.y);
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(m_stageWindow, &fbW, &fbH);
    m_stageWidth = (fbW > 0) ? fbW : targetMon.width;
    m_stageHeight = (fbH > 0) ? fbH : targetMon.height;

    glfwSetFramebufferSizeCallback(m_stageWindow, FramebufferSizeCallback);
    glfwSetKeyCallback(m_stageWindow, WindowKeyCallback);

    return true;
}

void WindowManager::closeStageWindow() {
    if (m_stageWindow) {
        glfwDestroyWindow(m_stageWindow);
        m_stageWindow = nullptr;
    }
    if (m_controlWindow) {
        glfwMakeContextCurrent(m_controlWindow);
    }
}

bool WindowManager::isStageWindowOpen() const {
    return m_stageWindow != nullptr && !glfwWindowShouldClose(m_stageWindow);
}

void WindowManager::toggleSingleScreenFullscreen() {
    if (!m_controlWindow) return;

    if (!m_isSingleScreenFullscreen) {
        glfwGetWindowPos(m_controlWindow, &m_savedX, &m_savedY);
        glfwGetWindowSize(m_controlWindow, &m_savedW, &m_savedH);

        GLFWmonitor* mon = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(mon);
        if (mon && mode) {
            glfwSetWindowMonitor(m_controlWindow, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
            m_isSingleScreenFullscreen = true;
        }
    } else {
        glfwSetWindowMonitor(m_controlWindow, nullptr, m_savedX, m_savedY, m_savedW, m_savedH, 0);
        m_isSingleScreenFullscreen = false;
    }
}

bool WindowManager::processMessages() {
    if (!m_controlWindow) return false;
    glfwPollEvents();

    if (m_stageWindow && glfwWindowShouldClose(m_stageWindow)) {
        closeStageWindow();
    }

    return !glfwWindowShouldClose(m_controlWindow);
}

void WindowManager::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (!s_instance || width <= 0 || height <= 0) return;

    if (window == s_instance->m_controlWindow) {
        s_instance->m_controlWidth = width;
        s_instance->m_controlHeight = height;
        if (s_instance->m_resizeCallback) {
            s_instance->m_resizeCallback(width, height, false);
        }
    } else if (window == s_instance->m_stageWindow) {
        s_instance->m_stageWidth = width;
        s_instance->m_stageHeight = height;
        if (s_instance->m_resizeCallback) {
            s_instance->m_resizeCallback(width, height, true);
        }
    }
}

void WindowManager::WindowKeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (!s_instance || action != GLFW_PRESS) return;

    if (key == GLFW_KEY_F11) {
        s_instance->toggleSingleScreenFullscreen();
    } else if (key == GLFW_KEY_ESCAPE) {
        if (window == s_instance->m_stageWindow) {
            s_instance->closeStageWindow();
        } else if (s_instance->m_isSingleScreenFullscreen) {
            s_instance->toggleSingleScreenFullscreen();
        }
    }

    if (s_instance->m_keyCallback) {
        s_instance->m_keyCallback(key, action);
    }
}

} // namespace Visualizer::Graphics
