#include "Mesh3D.h"
#include "../core/Logger.h"
#include <fstream>
#include <iostream>
#include <filesystem>

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

static std::filesystem::path ResolveMeshPath(const std::string& relPath) {
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

Mesh3D::Mesh3D() {}

Mesh3D::~Mesh3D() {
    cleanup();
}

void Mesh3D::cleanup() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    m_indexCount = 0;
}

bool Mesh3D::loadBinary(const std::string& path) {
    std::filesystem::path resolved = ResolveMeshPath(path);
    if (resolved.empty()) {
        Visualizer::Log("[Mesh3D] Mesh file not found: " + path);
        return false;
    }

    FILE* f = nullptr;
#if defined(_MSC_VER)
    fopen_s(&f, resolved.string().c_str(), "rb");
#else
    f = fopen(resolved.string().c_str(), "rb");
#endif
    if (!f) {
        Visualizer::Log("[Mesh3D] Failed to open: " + resolved.string());
        return false;
    }

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    if (fread(&vertexCount, sizeof(uint32_t), 1, f) != 1 ||
        fread(&indexCount, sizeof(uint32_t), 1, f) != 1) {
        fclose(f);
        Visualizer::Log("[Mesh3D] Failed to read header: " + resolved.string());
        return false;
    }

    std::vector<MeshVertex> vertices(vertexCount);
    std::vector<uint32_t> indices(indexCount);

    if (fread(vertices.data(), sizeof(MeshVertex), vertexCount, f) != vertexCount ||
        fread(indices.data(), sizeof(uint32_t), indexCount, f) != indexCount) {
        fclose(f);
        Visualizer::Log("[Mesh3D] Failed to read vertex/index payload: " + resolved.string());
        return false;
    }
    fclose(f);

    cleanup();

    // Create and bind VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create and upload VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MeshVertex), vertices.data(), GL_STATIC_DRAW);

    // Create and upload EBO
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Vertex Attributes:
    // Location 0: Position (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, x));

    // Location 1: Normal (3 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, nx));

    // Location 2: UV (2 floats)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, u));

    // Location 3: Barycentric (3 floats)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, bx));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_indexCount = indexCount;
    Visualizer::Log("[Mesh3D] Loaded '" + resolved.filename().string() + "' (" +
                    std::to_string(vertexCount) + " vertices, " +
                    std::to_string(indexCount / 3) + " triangles)");
    return true;
}

void Mesh3D::draw() const {
    if (m_vao != 0 && m_indexCount > 0) {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}

} // namespace Visualizer::Graphics
