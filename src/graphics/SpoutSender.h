#pragma once

#include "GL33Loader.h"
#include <string>
#include <cstdint>

namespace Visualizer::Graphics {

class SpoutSender {
public:
    SpoutSender() = default;
    ~SpoutSender() { shutdown(); }

    bool initialize(void* /*device*/, const std::string& senderName, uint32_t width, uint32_t height) {
        m_senderName = senderName;
        m_width = (width > 0) ? width : 1920;
        m_height = (height > 0) ? height : 1080;
        m_isInitialized = true;
        return true;
    }

    void shutdown() {
        m_isInitialized = false;
    }

    void updateTexture(void* /*context*/, GLuint /*sourceTexture*/) {
        // Cross-platform shared texture output hook
    }

    bool isInitialized() const { return m_isInitialized; }
    const std::string& getSenderName() const { return m_senderName; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }

private:
    std::string m_senderName = "DJVisualizer";
    uint32_t m_width = 1920;
    uint32_t m_height = 1080;
    bool m_isInitialized = false;
};

} // namespace Visualizer::Graphics
