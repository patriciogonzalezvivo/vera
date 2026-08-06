#pragma once

#include "vera/types/scene.h"

namespace vera {

bool loadOBJ(const std::string& _filename, Scene* _scene, bool _verbose, const std::string& _prefix = "");

bool loadOBJ( const std::string& _filename, Mesh& _mesh );
inline Mesh loadOBJ( const std::string& _filename) {
    Mesh mesh;
    loadOBJ(_filename, mesh);
    return mesh;
}


}