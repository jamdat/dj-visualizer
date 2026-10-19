#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb/stb_image.h"
#include "TextureManager.h"
#include "../core/Logger.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

namespace Visualizer::Graphics {

static std::filesystem::path GetBaseAppDir() {
#if defined(_WIN32)
    wchar_t exePathBuf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    return std::filesystem::path(exePathBuf).parent_path();
#elif defined(__APPLE__)
    char pathBuf[4096];
    uint32_t size = sizeof(pathBuf);
    if (_NSGetExecutablePath(pathBuf, &size) == 0) {
        std::filesystem::path exeDir = std::filesystem::weakly_canonical(std::filesystem::path(pathBuf)).parent_path();
        std::error_code ec;
        if (std::filesystem::exists(exeDir / "../Resources/assets", ec)) {
            return std::filesystem::weakly_canonical(exeDir / "../Resources", ec);
        }
        return exeDir;
    }
    return std::filesystem::current_path();
#else
    return std::filesystem::current_path();
#endif
}

static std::filesystem::path ResolveAssetPath(const std::string& relPath) {
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path() / relPath);
    std::filesystem::path baseDir = GetBaseAppDir();
    candidates.push_back(baseDir / relPath);
    candidates.push_back(baseDir / "assets" / relPath);
    candidates.push_back(baseDir.parent_path() / relPath);
    candidates.push_back(baseDir.parent_path().parent_path() / relPath);

    std::error_code ec;
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p, ec)) {
            return std::filesystem::weakly_canonical(p, ec);
        }
    }
    return "";
}

TextureManager::TextureManager(void* /*device*/, void* /*context*/) {
    m_audioBuffer.resize(512 * 2, 0.0f);
}

TextureManager::~TextureManager() {
    if (m_audioTexture) {
        glDeleteTextures(1, &m_audioTexture);
        m_audioTexture = 0;
    }
    for (auto& [name, tex] : m_textures) {
        if (tex.texture) {
            glDeleteTextures(1, &tex.texture);
        }
    }
    m_textures.clear();
}

bool TextureManager::initialize() {
    createAudioTexture();
    generateDefaultProceduralTextures();
    return true;
}

void TextureManager::createAudioTexture() {
    glGenTextures(1, &m_audioTexture);
    glBindTexture(GL_TEXTURE_2D, m_audioTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_audioBuffer.assign(512 * 2, 0.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 512, 2, 0, GL_RED, GL_FLOAT, m_audioBuffer.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureManager::updateAudioTexture(const std::vector<float>& spectrum512, const std::vector<float>& waveform512) {
    if (!m_audioTexture) return;

    for (size_t i = 0; i < 512; ++i) {
        m_audioBuffer[i] = (i < spectrum512.size()) ? spectrum512[i] : 0.0f;
        m_audioBuffer[512 + i] = (i < waveform512.size()) ? waveform512[i] : 0.0f;
    }

    glBindTexture(GL_TEXTURE_2D, m_audioTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 2, GL_RED, GL_FLOAT, m_audioBuffer.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint TextureManager::createRGBA8Texture(int w, int h, const void* data, bool wrapRepeat) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapRepeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void TextureManager::generateDefaultProceduralTextures() {
    // 1. Noise Texture (256x256 RGBA)
    {
        int w = 256, h = 256;
        std::vector<uint32_t> pixels(w * h);
        std::mt19937 rng(1337);
        std::uniform_int_distribution<int> dist(0, 255);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                uint8_t r = static_cast<uint8_t>(dist(rng));
                uint8_t g = static_cast<uint8_t>(dist(rng));
                uint8_t b = static_cast<uint8_t>(dist(rng));
                uint8_t a = 255;
                pixels[y * w + x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
        }
        GLuint tex = createRGBA8Texture(w, h, pixels.data(), true);
        m_textures["noise"] = { tex, w, h, 4, "", {} };
    }

    // 2. Daft Punk Authentic Logo Mask & SDF (1024x256 RGBA)
    {
        bool loaded = loadTexture("logo", "assets/textures/dad_punk_logo.tga");
        if (!loaded) {
            loaded = loadTexture("logo", "assets/textures/dad_punk_logo.png");
        }
        if (!loaded) {
            loaded = loadTexture("logo", "assets/textures/daft_punk_brush.png");
        }

        if (!loaded) {
            Visualizer::Log("[TextureManager] Generating in-memory procedural DAD PUNK SDF fallback (1024x256)");
            int w = 1024, h = 256;
            std::vector<uint32_t> pixels(w * h, 0);

            // Bold, solid horizontal "DAD PUNK" blocks with rounded bevel SDF (never a triangle!)
            for (int y = 0; y < h; ++y) {
                float ny = static_cast<float>(y) / static_cast<float>(h);
                float centerY = std::abs(ny - 0.5f) * 2.0f; // 0 at center, 1 at top/bottom

                for (int x = 0; x < w; ++x) {
                    float nx = static_cast<float>(x) / static_cast<float>(w);
                    float seg = nx * 8.0f;
                    int charIdx = static_cast<int>(seg);
                    float charU = (seg - charIdx); // 0..1 in character box

                    bool isText = (charIdx != 3) && (centerY < 0.65f) && (charU > 0.15f && charU < 0.85f);
                    float dEdge = isText ? (0.65f - centerY) : -1.0f;

                    uint8_t a = isText ? 255 : 0;
                    uint8_t sdf = static_cast<uint8_t>(std::clamp(128.0f + dEdge * 180.0f, 0.0f, 255.0f));
                    uint8_t bevel = isText ? static_cast<uint8_t>(std::clamp(dEdge * 350.0f, 0.0f, 255.0f)) : 0;
                    uint8_t rim = (bevel > 30 && bevel < 220) ? 255 : 0;

                    pixels[y * w + x] = (a << 24) | (rim << 16) | (bevel << 8) | sdf;
                }
            }
            GLuint tex = createRGBA8Texture(w, h, pixels.data(), false);
            m_textures["logo"] = { tex, w, h, 4, "", {} };
        }
    }

    // 3. Neon Color Ramp LUT (256x1 RGBA)
    {
        int w = 256, h = 1;
        std::vector<uint32_t> pixels(w);

        for (int x = 0; x < w; ++x) {
            float t = static_cast<float>(x) / 255.0f;
            float r = std::clamp(std::sin(t * 3.14159f * 0.8f + 0.5f) * 1.5f, 0.0f, 1.0f);
            float g = std::clamp(std::sin((t - 0.35f) * 3.14159f * 1.2f) * 1.2f, 0.0f, 1.0f);
            float b = std::clamp(std::cos(t * 3.14159f * 0.9f) * 0.9f, 0.0f, 1.0f);

            uint8_t ur = static_cast<uint8_t>(r * 255.0f);
            uint8_t ug = static_cast<uint8_t>(g * 255.0f);
            uint8_t ub = static_cast<uint8_t>(b * 255.0f);
            pixels[x] = (255 << 24) | (ub << 16) | (ug << 8) | ur;
        }
        GLuint tex = createRGBA8Texture(w, h, pixels.data(), false);
        m_textures["lut"] = { tex, w, h, 4, "", {} };
    }

    // 4. Lens Dirt Texture (256x256 RGBA)
    {
        int w = 256, h = 256;
        std::vector<uint32_t> pixels(w * h, 0);
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> fdist(0.0f, 1.0f);

        struct Spot { float x, y, r, intensity; };
        std::vector<Spot> spots;
        for (int i = 0; i < 40; ++i) {
            spots.push_back({ fdist(rng) * w, fdist(rng) * h, 4.0f + fdist(rng) * 20.0f, 0.1f + fdist(rng) * 0.4f });
        }

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float dirt = 0.02f;
                for (const auto& s : spots) {
                    float d = std::hypot(x - s.x, y - s.y);
                    if (d < s.r) {
                        float ring = std::sin((d / s.r) * 3.14159f);
                        dirt += ring * s.intensity;
                    }
                }
                dirt = (std::min)(dirt, 1.0f);
                uint8_t uVal = static_cast<uint8_t>(dirt * 255.0f);
                pixels[y * w + x] = (255 << 24) | (uVal << 16) | (uVal << 8) | uVal;
            }
        }
        GLuint tex = createRGBA8Texture(w, h, pixels.data(), false);
        m_textures["dirt"] = { tex, w, h, 4, "", {} };
    }

    // 5. Multi-Font Concert Atlas (1024x1536 RGBA: Neon Sans, Ryga, Raster Forge, Atari Classic)
    {
        bool loaded = loadTexture("font_atlas", "assets/textures/font_atlas.tga");
        if (!loaded) {
            loaded = loadTexture("font_atlas", "assets/textures/font_atlas.png");
        }
        if (!loaded) {
            Visualizer::Log("[TextureManager] Font atlas texture not found: assets/textures/font_atlas.tga");
        }
    }
}

bool TextureManager::loadTexture(const std::string& slotName, const std::string& filePath) {
    std::filesystem::path resolved = ResolveAssetPath(filePath);
    if (resolved.empty()) {
        Visualizer::Log("[TextureManager] Asset not found in search paths: " + filePath);
        return false;
    }

    int w = 0, h = 0, channels = 0;
    stbi_uc* data = stbi_load(resolved.string().c_str(), &w, &h, &channels, 4);
    if (!data) {
        Visualizer::Log("[TextureManager] stbi_load failed for: " + resolved.string());
        return false;
    }

    GLuint tex = createRGBA8Texture(w, h, data, false);
    stbi_image_free(data);

    if (!tex) {
        Visualizer::Log("[TextureManager] createRGBA8Texture failed for: " + resolved.string());
        return false;
    }

    auto it = m_textures.find(slotName);
    if (it != m_textures.end() && it->second.texture) {
        glDeleteTextures(1, &it->second.texture);
    }

    std::error_code ec;
    TextureResource res;
    res.texture = tex;
    res.width = w;
    res.height = h;
    res.channels = channels;
    res.filePath = resolved.string();
    res.lastModified = std::filesystem::last_write_time(resolved, ec);

    m_textures[slotName] = res;
    Visualizer::Log("[TextureManager] Loaded '" + slotName + "' (" + std::to_string(w) + "x" + std::to_string(h) + ") from: " + resolved.string());
    return true;
}

bool TextureManager::checkForHotReload() {
    bool reloaded = false;
    for (auto& [name, res] : m_textures) {
        std::error_code ec;
        if (res.filePath.empty() || !std::filesystem::exists(res.filePath, ec)) continue;

        auto currentWriteTime = std::filesystem::last_write_time(res.filePath, ec);
        if (ec) continue;

        if (currentWriteTime > res.lastModified) {
            res.lastModified = currentWriteTime;
            if (loadTexture(name, res.filePath)) {
                reloaded = true;
            }
        }
    }
    return reloaded;
}

GLuint TextureManager::getTextureSRV(const std::string& slotName) {
    auto it = m_textures.find(slotName);
    if (it != m_textures.end()) return it->second.texture;
    return 0;
}

std::vector<std::string> TextureManager::getLoadedTextureNames() const {
    std::vector<std::string> names;
    for (const auto& [k, v] : m_textures) names.push_back(k);
    return names;
}

void TextureManager::bindAll(void* /*context*/) {
    GLuint texs[6] = {
        m_audioTexture,
        getTextureSRV("noise"),
        getTextureSRV("logo"),
        getTextureSRV("lut"),
        getTextureSRV("dirt"),
        getTextureSRV("font_atlas")
    };

    for (int i = 0; i < 6; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
    }
}

} // namespace Visualizer::Graphics
