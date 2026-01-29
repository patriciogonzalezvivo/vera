#pragma once

#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "vera/gl/texture.h"

namespace vera {

class Gsplat {
public:

    Gsplat();
    virtual ~Gsplat();

    void    load(const std::string& _filepath);
    void    clear();
    void    sort(const glm::mat4& _viewProj, std::vector<uint32_t>& _depthIndex);
    size_t  count() const { return m_positions.size(); }
    
    Texture* getTexture();

private:
    void pack();

    std::vector<glm::u8vec4> m_colors;
    std::vector<glm::quat>  m_rotations;
    std::vector<glm::vec3>  m_positions;
    std::vector<glm::vec3>  m_scales;

    std::vector<float>      m_packedData;       // Packed data for GPU
    std::vector<float>      m_worldPositions;   // Only needed for sorting


};

}