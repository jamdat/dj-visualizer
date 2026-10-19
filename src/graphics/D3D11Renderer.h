#pragma once

#include "GL33Loader.h"
#include <memory>
#include <string>
#include "ConstantBuffers.h"
#include "ShaderCompiler.h"
#include "TextureManager.h"
#include "SpoutSender.h"
#include "WindowManager.h"

namespace Visualizer::Graphics {

class D3D11Renderer {
public:
    D3D11Renderer();
    ~D3D11Renderer();

    // Initialize OpenGL 3.3 Core pipeline for Control Window
    bool initialize(WindowManager* windowManager);

    // Setup or release secondary Stage Projector Window context state
    bool setupStageSwapChain(GLFWwindow* stageWindow, int width, int height);
    void releaseStageSwapChain();

    // Resize handlers
    void resizeControl(int width, int height);
    void resizeStage(int width, int height);

    // Render frame
    void beginFrame();
    void renderScene(const std::string& shaderName, const AudioVisualizerParams& params);
    void renderPostProcess(const AudioVisualizerParams& params);
    void bindControlRenderTarget();
    void renderFullscreenBlit();
    void endFrame(bool vsync = true);

    // Subsystems
    ShaderCompiler* getShaderCompiler() { return m_shaderCompiler.get(); }
    TextureManager* getTextureManager() { return m_textureManager.get(); }
    SpoutSender* getSpoutSender() { return &m_spoutSender; }

    void* getDevice() const { return nullptr; }
    void* getContext() const { return nullptr; }
    GLuint getSceneSRV() const { return m_sceneTexture; }
    GLuint getFinalSRV() const { return m_postTexture; }

private:
    void createOffscreenRenderTargets(int width, int height);
    void createConstantBuffer();

    WindowManager* m_windowManager = nullptr;

    GLuint m_controlVAO = 0;
    GLuint m_stageVAO = 0;

    // Offscreen render targets (Scene -> Post-Process -> Output)
    GLuint m_sceneFBO = 0;
    GLuint m_sceneTexture = 0;

    GLuint m_postFBO = 0;
    GLuint m_postTexture = 0;

    int m_offscreenWidth = 1920;
    int m_offscreenHeight = 1080;

    // Uniform Buffer Object for AudioVisualizerParams (binding 0)
    GLuint m_constantBuffer = 0;

    // Subsystems
    std::unique_ptr<ShaderCompiler> m_shaderCompiler;
    std::unique_ptr<TextureManager> m_textureManager;
    SpoutSender m_spoutSender;
};

} // namespace Visualizer::Graphics
