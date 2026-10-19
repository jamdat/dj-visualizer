#pragma once

#include "GL33Loader.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <chrono>

namespace Visualizer::Graphics {

struct CompiledShader {
    GLuint program = 0;
    std::filesystem::file_time_type lastModifiedTime;
    std::string filePath;
    std::string lastErrorLog;
    bool isValid = false;
};

class ShaderCompiler {
public:
    ShaderCompiler(void* device = nullptr);
    ~ShaderCompiler();

    // Compile standard fullscreen quad vertex shader
    GLuint createFullscreenVertexShader();

    // Compile lightweight passthrough pixel shader Program for fast 4K swapchain blitting
    GLuint createPassthroughPixelShader();

    // Load and compile a pixel shader from .hlsl file into an OpenGL 3.3 Program
    bool loadPixelShader(const std::string& shaderName, const std::string& filePath, GLuint* outProgram = nullptr);

    // Check all tracked shaders for file modifications and hot-reload them
    bool checkForHotReload(std::string& outReloadedName, std::string& outError);

    // Force recompile a specific shader
    bool recompileShader(const std::string& shaderName);

    // Get compiled shader program by key
    GLuint getPixelShader(const std::string& shaderName);

    // Get compiler error message for a shader (empty if compiled successfully)
    std::string getLastError(const std::string& shaderName) const;

    // Get all tracked shader names
    std::vector<std::string> getShaderNames() const;

private:
    bool compilePixelShaderInternal(const std::string& filePath, GLuint* outProgram, std::string& outError);
    void configureProgramBindings(GLuint program);

    GLuint m_fullscreenVS = 0;
    GLuint m_passthroughProgram = 0;
    std::unordered_map<std::string, CompiledShader> m_shaders;
};

} // namespace Visualizer::Graphics
