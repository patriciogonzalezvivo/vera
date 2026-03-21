#pragma once

#include "glm/glm.hpp"
#include <string>

// toVec4 — convert a color name or hex string to a normalised RGBA vec4.
// Accepts CSS named colors (e.g. "red", "darkslateblue") and hex strings in
// #RRGGBB or #RGB form.  Returns opaque white (1,1,1,1) for unknown inputs.
glm::vec4 toVec4(const std::string& color);


