//
//  PolarPoint.h
//
//  Created by Patricio Gonzalez Vivo on 9/12/14.
//
//

#pragma once

#include "glm/glm.hpp"

namespace vera {

class PolarPoint {
public:
    
    PolarPoint();
    PolarPoint(const float &_angle, const float &_radius, const float &_z = 0.0);
    PolarPoint(const glm::vec3 &_org, const glm::vec3 &_dst);
    
    glm::vec3 getXY();
    
    float a,r,z;
};

}