#pragma once

#include "vera/types/scene.h"

namespace vera {

bool loadSTL(const std::string& _filename, Scene* _scene, bool _verbose);
bool loadSTL( const std::string& _filename, Mesh& _mesh );
inline Mesh loadSTL( const std::string& _filename) {
    Mesh mesh;
    loadSTL(_filename, mesh);
    return mesh;
}

}