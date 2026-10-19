#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb/stb_image.h"
#include "TextureManager.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

namespace Visualizer::Graphics {

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

    // 2. Daft Punk Alive Logo Mask (256x256 RGBA)
    {
        int w = 256, h = 256;
        std::vector<uint32_t> pixels(w * h, 0);

        for (int y = 0; y < h; ++y) {
            float ny = 1.0f - static_cast<float>(y) / static_cast<float>(h);
            for (int x = 0; x < w; ++x) {
                float nx = (static_cast<float>(x) / static_cast<float>(w)) * 2.0f - 1.0f;
                float halfWidth = (1.0f - ny) * 0.85f;
                bool insidePyramid = (std::abs(nx) <= halfWidth) && (ny >= 0.1f && ny <= 0.9f);
                bool edgePyramid = insidePyramid && (std::abs(std::abs(nx) - halfWidth) < 0.035f || ny < 0.13f);

                uint8_t val = edgePyramid ? 255 : (insidePyramid ? 60 : 0);
                uint8_t a = insidePyramid ? 255 : 0;
                pixels[y * w + x] = (a << 24) | (val << 16) | (val << 8) | val;
            }
        }
        GLuint tex = createRGBA8Texture(w, h, pixels.data(), false);
        m_textures["logo"] = { tex, w, h, 4, "", {} };
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
}

bool TextureManager::loadTexture(const std::string& slotName, const std::string& filePath) {
    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec)) return false;

    int w = 0, h = 0, channels = 0;
    stbi_uc* data = stbi_load(filePath.c_str(), &w, &h, &channels, 4);
    if (!data) {
        std::cerr << "Failed to load texture file: " << filePath << std::endl;
        return false;
    }

    GLuint tex = createRGBA8Texture(w, h, data, false);
    stbi_image_free(data);

    if (!tex) return false;

    auto it = m_textures.find(slotName);
    if (it != m_textures.end() && it->second.texture) {
        glDeleteTextures(1, &it->second.texture);
    }

    TextureResource res;
    res.texture = tex;
    res.width = w;
    res.height = h;
    res.channels = channels;
    res.filePath = filePath;
    res.lastModified = std::filesystem::last_write_time(filePath, ec);

    m_textures[slotName] = res;
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
    GLuint texs[5] = {
        m_audioTexture,
        getTextureSRV("noise"),
        getTextureSRV("logo"),
        getTextureSRV("lut"),
        getTextureSRV("dirt")
    };

    for (int i = 0; i < 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
    }
}

} // namespace Visualizer::Graphics
