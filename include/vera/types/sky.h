#pragma once

#include "glm/glm.hpp"

namespace vera {

// SKY BOX GENERATOR
// -------------------------------------------------------------- 
struct SkyData {
    glm::vec3   groundAlbedo    = glm::vec3(0.25f);
    float       elevation       = 0.3f;
    float       azimuth         = 0.0f;
    float       turbidity       = 4.0f;
    bool        change          = false;
};

}