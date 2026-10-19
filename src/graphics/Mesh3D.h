#pragma once

#include "GL33Loader.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Visualizer::Graphics {

#pragma pack(push, 1)
struct MeshVertex {
    float x, y, z;       // Position
    float nx, ny, nz;    // Normal
    float u, v;          // TexCoord
    float bx, by, bz;    // Barycentric coordinates for wireframe facet rendering
};
#pragma pack(pop)

class Mesh3D {
public:
    Mesh3D();
    ~Mesh3D();

    bool loadBinary(const std::string& path);

    void draw() const;
    bool isLoaded() const { return m_vao != 0 && m_indexCount > 0; }
    uint32_t getIndexCount() const { return m_indexCount; }

private:
    void cleanup();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    uint32_t m_indexCount = 0;
};

} // namespace Visualizer::Graphics
