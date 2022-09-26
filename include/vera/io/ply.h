#pragma once

#include "vera/types/scene.h"

namespace vera {

bool loadPLY(const std::string& _filename, Scene* _scene, bool _verbose);

bool loadPLY( const std::string& _filename, Mesh& _mesh );
inline Mesh loadPLY( const std::string& _filename) {
    Mesh mesh;
    loadPLY(_filename, mesh);
    return mesh;
}

}