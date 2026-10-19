#pragma once

#include "GL33Loader.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace Visualizer::Graphics {

struct TextureResource {
    GLuint texture = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    std::string filePath;
    std::filesystem::file_time_type lastModified;
};

class TextureManager {
public:
    TextureManager(void* device = nullptr, void* context = nullptr);
    ~TextureManager();

    // Initialize samplers and procedural default textures
    bool initialize();

    // Update dynamic audio data texture (512x2: Row 0 = Spectrum, Row 1 = Waveform)
    void updateAudioTexture(const std::vector<float>& spectrum512, const std::vector<float>& waveform512);

    // Load or reload custom texture from file path
    bool loadTexture(const std::string& slotName, const std::string& filePath);

    // Check for texture file modifications and hot-reload
    bool checkForHotReload();

    // Bind standard shader resources (t0..t4)
    void bindAll(void* context = nullptr);

    // Get specific texture handle
    GLuint getAudioSRV() const { return m_audioTexture; }
    GLuint getTextureSRV(const std::string& slotName);

    // List loaded texture slots
    std::vector<std::string> getLoadedTextureNames() const;

private:
    void createAudioTexture();
    void generateDefaultProceduralTextures();
    GLuint createRGBA8Texture(int w, int h, const void* data, bool wrapRepeat = false);

    // Dynamic audio texture (t0)
    GLuint m_audioTexture = 0;
    std::vector<float> m_audioBuffer;

    // User / Procedural textures (t1..t4)
    std::unordered_map<std::string, TextureResource> m_textures;
};

} // namespace Visualizer::Graphics
