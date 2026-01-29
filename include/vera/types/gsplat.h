#pragma once

#include <vector>
#include <string>
#include "glm/glm.hpp"

#include "vera/gl/texture.h"
#include "vera/gl/shader.h"
#include "camera.h"

namespace vera {

class Gsplat {
public:

    Gsplat();
    virtual ~Gsplat();

    bool    load(const std::string& _filepath);
    void    clear();
    size_t  count() const { return m_positions.size(); }
    
    void    draw(Camera& _camera);

private:
    void    pack();
    
    bool    loadPLY(const std::string& _filepath);
    bool    loadSPLAT(const std::string& _filepath);

    void    initGPUData();
    Texture* createTexture();
    void    sort(const glm::mat4& _viewProj);

    std::vector<glm::u8vec4> m_colors;
    std::vector<glm::quat>  m_rotations;
    std::vector<glm::vec3>  m_positions;
    std::vector<glm::vec3>  m_scales;
    
    // Sorting cache to avoid reallocation
    std::vector<std::pair<float, uint32_t>> m_sorter;
    std::vector<uint32_t>   m_depthIndex;
    std::vector<float>      m_indexFloats;

    std::vector<float>      m_packedData;       // Packed data for GPU
    std::vector<float>      m_worldPositions;   // Only needed for sorting

    Texture*                m_texture = nullptr;
    Shader*                 m_shader = nullptr;

    // Buffers
    GLuint                  m_vao = -1;
    GLuint                  m_positionVBO = -1;
    GLuint                  m_indexVBO = -1;

    // Cached attribute locations
    GLint                   m_position = -1;
    GLint                   m_index = -1;
};

}