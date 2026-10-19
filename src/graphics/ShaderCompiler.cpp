#include "ShaderCompiler.h"
#include "../core/Logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

namespace Visualizer::Graphics {

static std::filesystem::path GetExecutableDir() {
#if defined(_WIN32)
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    return std::filesystem::path(exePathBuf).parent_path();
#elif defined(__APPLE__)
    char pathBuf[4096];
    uint32_t size = sizeof(pathBuf);
    if (_NSGetExecutablePath(pathBuf, &size) == 0) {
        return std::filesystem::weakly_canonical(std::filesystem::path(pathBuf)).parent_path();
    }
    return std::filesystem::current_path();
#else
    return std::filesystem::current_path();
#endif
}

static std::string FindShaderFile(const std::string& inputPath) {
    std::error_code ec;
    if (std::filesystem::exists(inputPath, ec)) {
        return std::filesystem::absolute(inputPath, ec).string();
    }

    std::filesystem::path exeDir = GetExecutableDir();
    std::filesystem::path filename = std::filesystem::path(inputPath).filename();

    std::vector<std::filesystem::path> candidates = {
        exeDir / inputPath,
        exeDir / "src" / "shaders" / filename,
        exeDir / "../Resources/src/shaders" / filename, // macOS .app bundle Resources
        exeDir / "../Resources/shaders" / filename,
        exeDir / ".." / "src" / "shaders" / filename,
        exeDir / ".." / ".." / "src" / "shaders" / filename,
        exeDir / ".." / ".." / ".." / "src" / "shaders" / filename
    };

    for (const auto& cand : candidates) {
        if (std::filesystem::exists(cand, ec)) {
            return std::filesystem::weakly_canonical(cand, ec).string();
        }
    }

    return inputPath;
}

static const char* s_fullscreenVS_GLSL = R"(#version 330 core
out vec2 vUV;
void main() {
    float u = float((gl_VertexID << 1) & 2);
    float v = float(gl_VertexID & 2);
    vUV = vec2(u, v);
    gl_Position = vec4(u * 2.0 - 1.0, v * 2.0 - 1.0, 0.0, 1.0);
}
)";

static const char* s_passthroughFS_GLSL = R"(#version 330 core
in vec2 vUV;
out vec4 outFragColor;
uniform sampler2D SceneTexture;
void main() {
    outFragColor = texture(SceneTexture, vUV);
}
)";

static std::string PreprocessShaderIncludes(const std::string& source) {
    std::string result = source;
    size_t pos = 0;
    while ((pos = result.find("#include", pos)) != std::string::npos) {
        size_t quoteStart = result.find('"', pos);
        if (quoteStart == std::string::npos) break;
        size_t quoteEnd = result.find('"', quoteStart + 1);
        if (quoteEnd == std::string::npos) break;

        std::string includeFile = result.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        std::string fullPath = FindShaderFile(includeFile);

        std::ifstream inc(fullPath, std::ios::in | std::ios::binary);
        std::string includeContent;
        if (inc.is_open()) {
            std::ostringstream ss;
            ss << inc.rdbuf();
            includeContent = ss.str();
        }

        size_t lineEnd = result.find('\n', quoteEnd);
        if (lineEnd == std::string::npos) lineEnd = result.length();
        else lineEnd += 1;

        result.replace(pos, lineEnd - pos, includeContent + "\n");
        pos += includeContent.length();
    }
    return result;
}

// Transpiles our D3D11 HLSL shaders into OpenGL 3.3 Core GLSL so the exact same .hlsl files
// run and hot-reload natively on both Windows and macOS (Apple Silicon).
static std::string TranspileHLSLToGLSL330(const std::string& hlslSource) {
    std::string s = hlslSource;

    // 1. Replace cbuffer header with std140 uniform block
    s = std::regex_replace(s, std::regex(R"(cbuffer\s+AudioVisualizerParams\s*:\s*register\s*\(\s*b0\s*\))"),
                           "layout(std140) uniform AudioVisualizerParams");

    // 2. Replace Texture2D declarations with uniform sampler2D
    s = std::regex_replace(s, std::regex(R"(Texture2D(?:<float4>)?\s+(\w+)\s*:\s*register\s*\(\s*t\d+\s*\)\s*;)"),
                           "uniform sampler2D $1;");

    // 3. Replace SamplerState declarations with dummy constants
    s = std::regex_replace(s, std::regex(R"(SamplerState\s+(\w+)\s*:\s*register\s*\(\s*s(\d+)\s*\)\s*;)"),
                           "const int $1 = $2;");

    // 4. Replace .SampleLevel(sampler, uv, lod) -> textureLod(tex, uv, float(lod))
    s = std::regex_replace(s, std::regex(R"((\w+)\.SampleLevel\s*\(\s*\w+\s*,\s*([^,]+)\s*,\s*([^)]+)\))"),
                           "textureLod($1, $2, float($3))");

    // 5. Replace .Sample(sampler, uv) -> texture(tex, uv)
    s = std::regex_replace(s, std::regex(R"((\w+)\.Sample\s*\(\s*\w+\s*,\s*([^)]+)\))"),
                           "texture($1, $2)");

    // 6. Strip HLSL semantics (: SV_Position, : TEXCOORD0, : SV_Target)
    s = std::regex_replace(s, std::regex(R"(\s*:\s*SV_Position\b)"), "");
    s = std::regex_replace(s, std::regex(R"(\s*:\s*TEXCOORD0\b)"), "");
    s = std::regex_replace(s, std::regex(R"(\s*:\s*SV_Target\b)"), "");

    // 7. Strip HLSL loop/branch attributes ([unroll], [loop], [branch])
    s = std::regex_replace(s, std::regex(R"(\[\s*(unroll|loop|branch)\s*\])"), "");

    // 8. Rename reserved GLSL identifier 'input' -> 'hlsl_input'
    s = std::regex_replace(s, std::regex(R"(\binput\b)"), "hlsl_input");

    // 9. Strip trailing 'f' from float literals (e.g. 2.51f -> 2.51) for strict macOS GLSL 330 compliance
    s = std::regex_replace(s, std::regex(R"((\d+\.\d+)f\b)"), "$1");

    std::string preamble = R"(#version 330 core
in vec2 vUV;
out vec4 outFragColor;

#define float2 vec2
#define float3 vec3
#define float4 vec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4
#define lerp mix
#define frac fract
#define atan2 atan
#define ddx dFdx
#define ddy dFdy
#define rsqrt inversesqrt

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec2 saturate(vec2 x)   { return clamp(x, vec2(0.0), vec2(1.0)); }
vec3 saturate(vec3 x)   { return clamp(x, vec3(0.0), vec3(1.0)); }
vec4 saturate(vec4 x)   { return clamp(x, vec4(0.0), vec4(1.0)); }

float fmod(float x, float y) { return x - y * trunc(x / y); }
vec2 fmod(vec2 x, vec2 y)   { return x - y * trunc(x / y); }
vec3 fmod(vec3 x, vec3 y)   { return x - y * trunc(x / y); }
)";

    std::string epilogue = R"(
void main() {
    VSOutput hlsl_input;
    hlsl_input.Position = gl_FragCoord;
    hlsl_input.UV = vUV;
    outFragColor = PSMain(hlsl_input);
}
)";

    return preamble + "\n" + s + "\n" + epilogue;
}

static GLuint CompileGLShaderStage(GLenum type, const std::string& src, std::string& outError) {
    GLuint shader = glCreateShader(type);
    const char* ptr = src.c_str();
    GLint len = static_cast<GLint>(src.size());
    glShaderSource(shader, 1, &ptr, &len);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(logLen > 0 ? logLen : 1, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, infoLog.data());
        outError = infoLog;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

ShaderCompiler::ShaderCompiler(void* /*device*/) {
}

ShaderCompiler::~ShaderCompiler() {
    if (m_passthroughProgram) {
        glDeleteProgram(m_passthroughProgram);
        m_passthroughProgram = 0;
    }
    if (m_fullscreenVS) {
        glDeleteShader(m_fullscreenVS);
        m_fullscreenVS = 0;
    }
    for (auto& [name, shader] : m_shaders) {
        if (shader.program) {
            glDeleteProgram(shader.program);
        }
    }
    m_shaders.clear();
}

GLuint ShaderCompiler::createFullscreenVertexShader() {
    if (m_fullscreenVS) return m_fullscreenVS;

    std::string err;
    m_fullscreenVS = CompileGLShaderStage(GL_VERTEX_SHADER, s_fullscreenVS_GLSL, err);
    if (!m_fullscreenVS) {
        Log("[ERROR] Fullscreen Vertex Shader compile error: " + err);
    }
    return m_fullscreenVS;
}

void ShaderCompiler::configureProgramBindings(GLuint program) {
    if (!program) return;

    GLuint blockIdx = glGetUniformBlockIndex(program, "AudioVisualizerParams");
    if (blockIdx != GL_INVALID_INDEX) {
        glUniformBlockBinding(program, blockIdx, 0);
    }

    glUseProgram(program);
    const char* texNames[7] = {
        "AudioDataTexture",
        "NoiseTexture",
        "LogoTexture",
        "ColorLUT",
        "LensDirtTexture",
        "FontAtlasTexture",
        "SceneTexture"
    };
    for (int i = 0; i < 7; ++i) {
        GLint loc = glGetUniformLocation(program, texNames[i]);
        if (loc >= 0) {
            glUniform1i(loc, i);
        }
    }
    glUseProgram(0);
}

GLuint ShaderCompiler::createPassthroughPixelShader() {
    if (m_passthroughProgram) return m_passthroughProgram;

    GLuint vs = createFullscreenVertexShader();
    std::string err;
    GLuint fs = CompileGLShaderStage(GL_FRAGMENT_SHADER, s_passthroughFS_GLSL, err);
    if (!vs || !fs) {
        Log("[ERROR] Passthrough FS compile error: " + err);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        glDeleteProgram(prog);
        return 0;
    }

    configureProgramBindings(prog);
    m_passthroughProgram = prog;
    return m_passthroughProgram;
}

GLuint ShaderCompiler::loadMeshShaderProgram(const std::string& vsPath, const std::string& fsPath) {
    std::string actualVS = FindShaderFile(vsPath);
    std::string actualFS = FindShaderFile(fsPath);

    std::ifstream fVS(actualVS);
    if (!fVS.is_open()) {
        Log("[ERROR] Failed to open mesh vertex shader: " + actualVS);
        return 0;
    }
    std::string vsSource((std::istreambuf_iterator<char>(fVS)), std::istreambuf_iterator<char>());

    std::ifstream fFS(actualFS);
    if (!fFS.is_open()) {
        Log("[ERROR] Failed to open mesh fragment shader: " + actualFS);
        return 0;
    }
    std::string fsSource((std::istreambuf_iterator<char>(fFS)), std::istreambuf_iterator<char>());

    std::string err;
    GLuint vs = CompileGLShaderStage(GL_VERTEX_SHADER, vsSource, err);
    if (!vs) {
        Log("[ERROR] Mesh Vertex Shader compilation failed: " + err);
        return 0;
    }

    GLuint fs = CompileGLShaderStage(GL_FRAGMENT_SHADER, fsSource, err);
    if (!fs) {
        Log("[ERROR] Mesh Fragment Shader compilation failed: " + err);
        glDeleteShader(vs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(logLen > 0 ? logLen : 1, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, infoLog.data());
        Log("[ERROR] Mesh shader program link failed: " + infoLog);
        glDeleteProgram(prog);
        return 0;
    }

    configureProgramBindings(prog);
    Log("[ShaderCompiler] Successfully compiled & linked 3D Mesh Shader Program");
    return prog;
}

bool ShaderCompiler::compilePixelShaderInternal(
    const std::string& filePath,
    GLuint* outProgram,
    std::string& outError
) {
    std::string actualPath = FindShaderFile(filePath);
    std::error_code ec;
    if (!std::filesystem::exists(actualPath, ec)) {
        outError = "Shader file not found: " + filePath + " (searched " + actualPath + ")";
        Log("Error: " + outError);
        return false;
    }

    std::ifstream file(actualPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        outError = "Failed to open shader file: " + actualPath;
        Log("Error: " + outError);
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string rawSource = ss.str();
    file.close();

    std::string inlinedHLSL = PreprocessShaderIncludes(rawSource);
    std::string glslSource = TranspileHLSLToGLSL330(inlinedHLSL);

    GLuint vs = createFullscreenVertexShader();
    GLuint fs = CompileGLShaderStage(GL_FRAGMENT_SHADER, glslSource, outError);
    if (!fs) {
        Log("GLSL compilation error in " + filePath + ":\n" + outError);
        return false;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(logLen > 0 ? logLen : 1, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, infoLog.data());
        outError = infoLog;
        glDeleteProgram(prog);
        Log("GLSL link error in " + filePath + ":\n" + outError);
        return false;
    }

    configureProgramBindings(prog);
    if (outProgram) *outProgram = prog;
    outError = "";
    return true;
}

bool ShaderCompiler::loadPixelShader(const std::string& shaderName, const std::string& filePath, GLuint* outProgram) {
    std::string actualPath = FindShaderFile(filePath);
    std::string errorLog;
    GLuint prog = 0;
    bool success = compilePixelShaderInternal(actualPath, &prog, errorLog);

    CompiledShader cs;
    cs.filePath = actualPath;
    cs.lastErrorLog = errorLog;
    cs.isValid = success;

    std::error_code ec;
    if (std::filesystem::exists(actualPath, ec)) {
        cs.lastModifiedTime = std::filesystem::last_write_time(actualPath, ec);
    }

    if (success) {
        cs.program = prog;
        if (outProgram) *outProgram = prog;
    }

    auto it = m_shaders.find(shaderName);
    if (it != m_shaders.end() && it->second.program) {
        glDeleteProgram(it->second.program);
    }

    m_shaders[shaderName] = cs;
    return success;
}

bool ShaderCompiler::checkForHotReload(std::string& outReloadedName, std::string& outError) {
    bool anyReloaded = false;

    for (auto& [name, shader] : m_shaders) {
        std::error_code ec;
        if (shader.filePath.empty() || !std::filesystem::exists(shader.filePath, ec)) continue;

        auto currentWriteTime = std::filesystem::last_write_time(shader.filePath, ec);
        if (ec) continue;

        if (currentWriteTime > shader.lastModifiedTime) {
            shader.lastModifiedTime = currentWriteTime;

            GLuint newProg = 0;
            std::string err;
            if (compilePixelShaderInternal(shader.filePath, &newProg, err)) {
                if (shader.program) glDeleteProgram(shader.program);
                shader.program = newProg;
                shader.lastErrorLog = "";
                shader.isValid = true;

                outReloadedName = name;
                outError = "";
                anyReloaded = true;
            } else {
                shader.lastErrorLog = err;
                outReloadedName = name;
                outError = err;
                anyReloaded = true;
            }
        }
    }

    return anyReloaded;
}

bool ShaderCompiler::recompileShader(const std::string& shaderName) {
    auto it = m_shaders.find(shaderName);
    if (it == m_shaders.end()) return false;

    GLuint newProg = 0;
    std::string err;
    if (compilePixelShaderInternal(it->second.filePath, &newProg, err)) {
        if (it->second.program) glDeleteProgram(it->second.program);
        it->second.program = newProg;
        it->second.lastErrorLog = "";
        it->second.isValid = true;
        std::error_code ec;
        if (std::filesystem::exists(it->second.filePath, ec)) {
            it->second.lastModifiedTime = std::filesystem::last_write_time(it->second.filePath, ec);
        }
        return true;
    } else {
        it->second.lastErrorLog = err;
        return false;
    }
}

GLuint ShaderCompiler::getPixelShader(const std::string& shaderName) {
    auto it = m_shaders.find(shaderName);
    if (it != m_shaders.end() && it->second.isValid) {
        return it->second.program;
    }
    return 0;
}

std::string ShaderCompiler::getLastError(const std::string& shaderName) const {
    auto it = m_shaders.find(shaderName);
    if (it != m_shaders.end()) {
        return it->second.lastErrorLog;
    }
    return "";
}

std::vector<std::string> ShaderCompiler::getShaderNames() const {
    std::vector<std::string> names;
    for (const auto& [k, v] : m_shaders) {
        names.push_back(k);
    }
    return names;
}

} // namespace Visualizer::Graphics
