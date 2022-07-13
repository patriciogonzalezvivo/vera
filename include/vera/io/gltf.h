#pragma once

#include "vera/types/scene.h"

namespace vera {

bool loadGLTF( const std::string& _filename, Scene* _scene, bool _verbose);

}