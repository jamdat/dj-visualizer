#include "D3D11Renderer.h"
#include "Math3D.h"
#include "../core/Logger.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace Visualizer::Graphics {

D3D11Renderer::D3D11Renderer() = default;

D3D11Renderer::~D3D11Renderer() {
    releaseStageSwapChain();

    if (m_meshProgram) { glDeleteProgram(m_meshProgram); m_meshProgram = 0; }
    if (m_sceneDepthTexture) { glDeleteTextures(1, &m_sceneDepthTexture); m_sceneDepthTexture = 0; }
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

    // Compile and link 3D Mesh Shader Program
    m_meshProgram = m_shaderCompiler->loadMeshShaderProgram("src/shaders/Mesh3D.vert", "src/shaders/Mesh3D.frag");
    if (!m_meshProgram) {
        Log("[D3D11Renderer] Warning: Failed to load 3D mesh shader program");
    }

    // Load authentic 3D Daft Punk polygonal mesh asset
    if (!m_heroMesh.loadBinary("assets/models/dad_punk_3d.bin")) {
        Log("[D3D11Renderer] Warning: Failed to load 3D mesh: assets/models/dad_punk_3d.bin");
    }

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

    // Create and attach depth buffer to scene FBO for 3D polygonal mesh rasterization
    if (m_sceneDepthTexture) { glDeleteTextures(1, &m_sceneDepthTexture); m_sceneDepthTexture = 0; }
    glGenTextures(1, &m_sceneDepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_sceneDepthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_sceneDepthTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    // 3. Bind Shader Program & Textures for Background Stage Pass
    GLuint prog = m_shaderCompiler->getPixelShader(shaderName);
    if (prog) {
        glUseProgram(prog);
        m_textureManager->bindAll();
        glBindVertexArray(m_controlVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(0);
    }

    // 4. Render 3D Polygonal Hero Mesh Pass
    // Active when LogoWeight > 0.01 and CustomParam5 > 0.5 (Authentic 3D Polygonal Mesh)
    if (params.LogoWeight > 0.01f && params.CustomParam5 > 0.5f && m_heroMesh.isLoaded()) {
        if (!m_meshProgram) {
            m_meshProgram = m_shaderCompiler->loadMeshShaderProgram("src/shaders/Mesh3D.vert", "src/shaders/Mesh3D.frag");
        }

        if (m_meshProgram) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glClear(GL_DEPTH_BUFFER_BIT);

            glUseProgram(m_meshProgram);

            // Compute Camera View & Projection Matrices
            // Narrow FOV (24 deg) avoids edge stretching and exaggerated extrusion depth on large text.
            // Camera is dollied back so framing matches the old 45 deg / 4.0 unit setup:
            // dist = 4.0 * tan(22.5 deg) / tan(12 deg)
            const float kFovDeg = 24.0f;
            const float kPi = 3.14159265f;
            float aspect = static_cast<float>(m_offscreenWidth) / static_cast<float>(m_offscreenHeight);
            Mat4 proj = Mat4::perspective(kFovDeg * kPi / 180.0f, aspect, 0.1f, 100.0f);

            float camDist = 4.0f * std::tan(22.5f * kPi / 180.0f) / std::tan(0.5f * kFovDeg * kPi / 180.0f);
            // Keep the same slight downward look angle as before (0.2 units over 4.0 distance)
            float camX = 0.0f;
            float camY = 0.15f + 0.2f * (camDist / 4.0f);
            float camZ = camDist;
            Mat4 view = Mat4::lookAt(camX, camY, camZ, 0.0f, 0.15f, 0.0f, 0.0f, 1.0f, 0.0f);

            // Model Transform:
            // CustomParam2 = logo size slider (0.5 to 3.5, default 1.25) remapped to 0..1,
            //   then to an effective mesh multiplier of 0.6x..1.5x (1.5x is the largest sensible size)
            // CustomParam4 = logo Y position offset (-0.6 to +0.8, default 0.0)
            // CustomParam6 = yaw rotation angle (degrees)
            float baseScale = 0.85f;
            float sliderVal = (params.CustomParam2 > 0.01f ? params.CustomParam2 : 1.25f);
            float sizeT = std::clamp((sliderVal - 0.5f) / 3.0f, 0.0f, 1.0f);
            float scaleMultiplier = 0.6f + (1.5f - 0.6f) * sizeT;
            float scaleVal = scaleMultiplier * baseScale;
            float posY = 0.22f + params.CustomParam4;
            float rotYRad = params.CustomParam6 * 3.14159265f / 180.0f;

            // Subtle audio reactivity: small kick pulse scale bounce
            float kickBounce = 1.0f + params.AudioKick * 0.05f;
            Mat4 modelS = Mat4::scale(scaleVal * kickBounce, scaleVal * kickBounce, scaleVal * kickBounce);
            Mat4 modelR = Mat4::rotationY(rotYRad);
            Mat4 modelT = Mat4::translation(0.0f, posY, 0.0f);
            Mat4 model = modelT * (modelR * modelS);

            Mat4 mvp = proj * (view * model);

            GLint locMVP = glGetUniformLocation(m_meshProgram, "u_MVP");
            if (locMVP >= 0) glUniformMatrix4fv(locMVP, 1, GL_FALSE, mvp.m);

            GLint locModel = glGetUniformLocation(m_meshProgram, "u_Model");
            if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, model.m);

            GLint locStyle = glGetUniformLocation(m_meshProgram, "u_Style");
            if (locStyle >= 0) glUniform1i(locStyle, params.LogoStyle);

            GLint locCamPos = glGetUniformLocation(m_meshProgram, "u_CameraPos");
            if (locCamPos >= 0) glUniform3f(locCamPos, camX, camY, camZ);

            m_heroMesh.draw();

            glUseProgram(0);
            glDisable(GL_DEPTH_TEST);
        }
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

        glActiveTexture(GL_TEXTURE6);
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
