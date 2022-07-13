#pragma once

#include "vera/types/scene.h"

namespace vera {

bool loadSTL(const std::string& _filename, Scene* _scene, bool _verbose);

}