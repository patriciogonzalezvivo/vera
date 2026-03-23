#pragma once

#include "glm/glm.hpp"
#include <string>

/// Convert color name or hex string to normalized RGBA vec4
/// 
/// Accepts CSS named colors (e.g., "red", "darkslateblue") and hex strings
/// in #RRGGBB or #RGB format. Returns opaque white (1,1,1,1) for unknown inputs.
///
/// @param color Color name (CSS) or hex string (#RGB or #RRGGBB)
/// @return Normalized RGBA color vector (0.0-1.0 per component)
glm::vec4 toVec4(const std::string& color);

