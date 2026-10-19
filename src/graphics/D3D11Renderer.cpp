#include "D3D11Renderer.h"
#include "../core/Logger.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace Visualizer::Graphics {

D3D11Renderer::D3D11Renderer() = default;

D3D11Renderer::~D3D11Renderer() {
    releaseStageSwapChain();

    if (m_constantBuffer) { glDeleteBuffers(1, &m_constantBuffer); m_constantBuffer = 0; }
    if (m_sceneFBO) { glDeleteFramebuffers(1, &m_sceneFBO); m_sceneFBO = 0; }
    if (m_sceneTexture) { glDeleteTextures(1, &m_sceneTexture); m_sceneTexture = 0; }
    if (m_postFBO) { glDeleteFramebuffers(1, &m_postFBO); m_postFBO = 0; }
    if (m_postTexture) { glDeleteTextures(1, &m_postTexture); m_postTexture = 0; }
    if (m_controlVAO) { glDeleteVertexArrays(1, &m_controlVAO); m_controlVAO = 0; }
}

bool D3D11Renderer::initialize(WindowManager* windowManager) {
    Log("Renderer::initialize started (OpenGL 3.3 Core)");
    m_windowManager = windowManager;
    if (!m_windowManager || !m_windowManager->getControlWindow()) {
        return false;
    }

    glfwMakeContextCurrent(m_windowManager->getControlWindow());

    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* rendererStr = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    Log(std::string("OpenGL Device: ") + (vendor ? vendor : "Unknown") + " | " +
        (rendererStr ? rendererStr : "Unknown") + " | " + (version ? version : "Unknown"));

    glGenVertexArrays(1, &m_controlVAO);
    glBindVertexArray(m_controlVAO);

    createConstantBuffer();
    createOffscreenRenderTargets(1920, 1080);

    m_shaderCompiler = std::make_unique<ShaderCompiler>(nullptr);
    m_shaderCompiler->createFullscreenVertexShader();
    m_shaderCompiler->createPassthroughPixelShader();

    m_textureManager = std::make_unique<TextureManager>(nullptr, nullptr);
    m_textureManager->initialize();

    m_spoutSender.initialize(nullptr, "DJVisualizer", 1920, 1080);

    return true;
}

void D3D11Renderer::createConstantBuffer() {
    glGenBuffers(1, &m_constantBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, m_constantBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(AudioVisualizerParams), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_constantBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

static void CreateFBOAttachment(int width, int height, GLuint& outFBO, GLuint& outTex) {
    if (outFBO) { glDeleteFramebuffers(1, &outFBO); outFBO = 0; }
    if (outTex) { glDeleteTextures(1, &outTex); outTex = 0; }

    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_2D, outTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenFramebuffers(1, &outFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, outFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void D3D11Renderer::createOffscreenRenderTargets(int width, int height) {
    m_offscreenWidth = width;
    m_offscreenHeight = height;

    CreateFBOAttachment(width, height, m_sceneFBO, m_sceneTexture);
    CreateFBOAttachment(width, height, m_postFBO, m_postTexture);
}

bool D3D11Renderer::setupStageSwapChain(GLFWwindow* stageWindow, int /*width*/, int /*height*/) {
    if (!stageWindow) return false;
    glfwMakeContextCurrent(stageWindow);
    if (!m_stageVAO) {
        glGenVertexArrays(1, &m_stageVAO);
    }
    glfwMakeContextCurrent(m_windowManager->getControlWindow());
    return true;
}

void D3D11Renderer::releaseStageSwapChain() {
    if (m_stageVAO && m_windowManager && m_windowManager->getStageWindow()) {
        glfwMakeContextCurrent(m_windowManager->getStageWindow());
        glDeleteVertexArrays(1, &m_stageVAO);
        m_stageVAO = 0;
        if (m_windowManager->getControlWindow()) {
            glfwMakeContextCurrent(m_windowManager->getControlWindow());
        }
    }
    m_stageVAO = 0;
}

void D3D11Renderer::resizeControl(int /*width*/, int /*height*/) {
    // OpenGL default framebuffer automatically tracks GLFW window framebuffer size
}

void D3D11Renderer::resizeStage(int /*width*/, int /*height*/) {
    // OpenGL default framebuffer automatically tracks GLFW stage framebuffer size
}

void D3D11Renderer::beginFrame() {
    if (!m_windowManager || !m_windowManager->getControlWindow()) return;
    glfwMakeContextCurrent(m_windowManager->getControlWindow());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, m_postFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void D3D11Renderer::renderScene(const std::string& shaderName, const AudioVisualizerParams& params) {
    if (!m_sceneFBO || !m_constantBuffer) return;

    // 1. Update Constant Buffer (std140 UBO binding 0)
    glBindBuffer(GL_UNIFORM_BUFFER, m_constantBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AudioVisualizerParams), &params);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_constantBuffer);

    // 2. Bind Scene FBO & Viewport
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
    glViewport(0, 0, m_offscreenWidth, m_offscreenHeight);

    // 3. Bind Shader Program & Textures
    GLuint prog = m_shaderCompiler->getPixelShader(shaderName);
    if (prog) {
        glUseProgram(prog);
        m_textureManager->bindAll();
        glBindVertexArray(m_controlVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void D3D11Renderer::renderPostProcess(const AudioVisualizerParams& /*params*/) {
    if (!m_postFBO || !m_sceneTexture) return;

    glBindFramebuffer(GL_FRAMEBUFFER, m_postFBO);
    glViewport(0, 0, m_offscreenWidth, m_offscreenHeight);

    GLuint postProg = m_shaderCompiler->getPixelShader("PostProcessGlitch");
    if (!postProg) {
        postProg = m_shaderCompiler->createPassthroughPixelShader();
    }

    if (postProg) {
        glUseProgram(postProg);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_constantBuffer);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, m_sceneTexture);

        glBindVertexArray(m_controlVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_spoutSender.updateTexture(nullptr, m_postTexture);

    // If Stage Output window is open, render final frame onto Stage Window
    if (m_windowManager && m_windowManager->isStageWindowOpen()) {
        GLFWwindow* stageWin = m_windowManager->getStageWindow();
        glfwMakeContextCurrent(stageWin);

        if (!m_stageVAO) {
            glGenVertexArrays(1, &m_stageVAO);
        }

        int sw = m_windowManager->getStageWidth();
        int sh = m_windowManager->getStageHeight();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, sw, sh);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLuint passProg = m_shaderCompiler->createPassthroughPixelShader();
        if (passProg) {
            glUseProgram(passProg);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, m_postTexture);
            glBindVertexArray(m_stageVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(0);
        }

        glfwSwapInterval(0);
        glfwSwapBuffers(stageWin);

        glfwMakeContextCurrent(m_windowManager->getControlWindow());
    }
}

void D3D11Renderer::bindControlRenderTarget() {
    if (!m_windowManager || !m_windowManager->getControlWindow()) return;
    glfwMakeContextCurrent(m_windowManager->getControlWindow());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_windowManager->getControlWidth(), m_windowManager->getControlHeight());
}

void D3D11Renderer::renderFullscreenBlit() {
    if (!m_postTexture || !m_windowManager) return;

    bindControlRenderTarget();

    GLuint passProg = m_shaderCompiler->createPassthroughPixelShader();
    if (passProg) {
        glUseProgram(passProg);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, m_postTexture);
        glBindVertexArray(m_controlVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }
}

void D3D11Renderer::endFrame(bool vsync) {
    if (!m_windowManager || !m_windowManager->getControlWindow()) return;
    GLFWwindow* ctrlWin = m_windowManager->getControlWindow();
    glfwMakeContextCurrent(ctrlWin);
    glfwSwapInterval(vsync ? 1 : 0);
    glfwSwapBuffers(ctrlWin);
}

} // namespace Visualizer::Graphics
