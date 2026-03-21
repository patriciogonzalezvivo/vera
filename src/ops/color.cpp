#include "vera/ops/color.h"

#include <unordered_map>

// toVec4 \u2014 convert a CSS color name or hex string (#RRGGBB / #RGB) to a
// normalised RGBA vec4 (all components in [0,1]).  Color name lookup uses a
// static unordered_map so repeated calls are O(1) instead of the O(n) linear
// if-else chain previously used.  Returns opaque white for unrecognised input.

// The "grey" variants map to the same values as their "gray" counterparts so
// both spellings are listed explicitly.

static const std::unordered_map<std::string, glm::vec4>& getCSSColorTable() {
    static const std::unordered_map<std::string, glm::vec4> table = {
        // Basic colours
        { "black",          glm::vec4(0.00f, 0.00f, 0.00f, 1.0f) },
        { "white",          glm::vec4(1.00f, 1.00f, 1.00f, 1.0f) },
        { "red",            glm::vec4(1.00f, 0.00f, 0.00f, 1.0f) },
        { "green",          glm::vec4(0.00f, 1.00f, 0.00f, 1.0f) },
        { "blue",           glm::vec4(0.00f, 0.00f, 1.00f, 1.0f) },
        { "yellow",         glm::vec4(1.00f, 1.00f, 0.00f, 1.0f) },
        { "cyan",           glm::vec4(0.00f, 1.00f, 1.00f, 1.0f) },
        { "magenta",        glm::vec4(1.00f, 0.00f, 1.00f, 1.0f) },
        { "gray",           glm::vec4(0.50f, 0.50f, 0.50f, 1.0f) },
        { "grey",           glm::vec4(0.50f, 0.50f, 0.50f, 1.0f) },
        { "transparent",    glm::vec4(0.00f, 0.00f, 0.00f, 0.0f) },
        { "orange",         glm::vec4(1.00f, 0.50f, 0.00f, 1.0f) },
        { "purple",         glm::vec4(0.50f, 0.00f, 0.50f, 1.0f) },
        { "pink",           glm::vec4(1.00f, 0.75f, 0.80f, 1.0f) },
        { "brown",          glm::vec4(0.65f, 0.16f, 0.16f, 1.0f) },
        { "navy",           glm::vec4(0.00f, 0.00f, 0.50f, 1.0f) },
        { "teal",           glm::vec4(0.00f, 0.50f, 0.50f, 1.0f) },
        { "lime",           glm::vec4(0.00f, 1.00f, 0.50f, 1.0f) },
        { "gold",           glm::vec4(1.00f, 0.84f, 0.00f, 1.0f) },
        { "silver",         glm::vec4(0.75f, 0.75f, 0.75f, 1.0f) },
        { "indigo",         glm::vec4(0.29f, 0.00f, 0.51f, 1.0f) },
        { "violet",         glm::vec4(0.93f, 0.51f, 0.93f, 1.0f) },
        { "orchid",         glm::vec4(0.85f, 0.44f, 0.84f, 1.0f) },
        { "coral",          glm::vec4(1.00f, 0.50f, 0.31f, 1.0f) },
        { "salmon",         glm::vec4(0.98f, 0.50f, 0.45f, 1.0f) },
        { "khaki",          glm::vec4(0.94f, 0.90f, 0.55f, 1.0f) },
        { "plum",           glm::vec4(0.87f, 0.63f, 0.87f, 1.0f) },
        { "lavender",       glm::vec4(0.90f, 0.90f, 0.98f, 1.0f) },
        { "peachpuff",      glm::vec4(1.00f, 0.85f, 0.73f, 1.0f) },
        { "tan",            glm::vec4(0.82f, 0.71f, 0.55f, 1.0f) },
        { "wheat",          glm::vec4(0.96f, 0.87f, 0.70f, 1.0f) },
        { "thistle",        glm::vec4(0.85f, 0.75f, 0.85f, 1.0f) },
        { "mintcream",      glm::vec4(0.96f, 1.00f, 0.98f, 1.0f) },
        { "seashell",       glm::vec4(1.00f, 0.96f, 0.93f, 1.0f) },
        { "snow",           glm::vec4(1.00f, 0.98f, 0.98f, 1.0f) },
        { "aliceblue",      glm::vec4(0.94f, 0.97f, 1.00f, 1.0f) },
        { "antiquewhite",   glm::vec4(0.98f, 0.92f, 0.84f, 1.0f) },
        { "beige",          glm::vec4(0.96f, 0.96f, 0.86f, 1.0f) },
        { "bisque",         glm::vec4(1.00f, 0.89f, 0.77f, 1.0f) },
        { "blanchedalmond", glm::vec4(1.00f, 0.92f, 0.80f, 1.0f) },
        { "burlywood",      glm::vec4(0.87f, 0.72f, 0.53f, 1.0f) },
        { "chartreuse",     glm::vec4(0.50f, 1.00f, 0.31f, 1.0f) },
        { "chocolate",      glm::vec4(0.82f, 0.41f, 0.12f, 1.0f) },
        { "cornflowerblue", glm::vec4(0.39f, 0.58f, 0.93f, 1.0f) },
        { "cornsilk",       glm::vec4(1.00f, 0.97f, 0.86f, 1.0f) },
        { "crimson",        glm::vec4(0.86f, 0.08f, 0.24f, 1.0f) },
        { "darkblue",       glm::vec4(0.00f, 0.00f, 0.55f, 1.0f) },
        { "darkcyan",       glm::vec4(0.00f, 0.55f, 0.55f, 1.0f) },
        { "darkgray",       glm::vec4(0.66f, 0.66f, 0.66f, 1.0f) },
        { "darkgrey",       glm::vec4(0.66f, 0.66f, 0.66f, 1.0f) },
        { "darkgreen",      glm::vec4(0.00f, 0.39f, 0.00f, 1.0f) },
        { "darkkhaki",      glm::vec4(0.74f, 0.72f, 0.42f, 1.0f) },
        { "darkorange",     glm::vec4(1.00f, 0.55f, 0.00f, 1.0f) },
        { "darkorchid",     glm::vec4(0.60f, 0.20f, 0.80f, 1.0f) },
        { "darkred",        glm::vec4(0.55f, 0.00f, 0.00f, 1.0f) },
        { "darksalmon",     glm::vec4(0.91f, 0.59f, 0.48f, 1.0f) },
        { "darkseagreen",   glm::vec4(0.56f, 0.74f, 0.56f, 1.0f) },
        { "darkslateblue",  glm::vec4(0.28f, 0.24f, 0.55f, 1.0f) },
        { "darkslategray",  glm::vec4(0.18f, 0.31f, 0.31f, 1.0f) },
        { "darkslategrey",  glm::vec4(0.18f, 0.31f, 0.31f, 1.0f) },
        { "darkturquoise",  glm::vec4(0.00f, 0.81f, 1.00f, 1.0f) },
        { "darkviolet",     glm::vec4(0.58f, 0.00f, 0.83f, 1.0f) },
    };
    return table;
}

glm::vec4 toVec4(const std::string& _color) {

    // CSS named color lookup \u2014 O(1) hash table instead of O(n) if-else chain
    const auto& table = getCSSColorTable();
    auto it = table.find(_color);
    if (it != table.end())
        return it->second;

    // Hexadecimal color format: #RRGGBB or #RGB
    if (!_color.empty() && _color[0] == '#') {
        std::string hex = _color.substr(1);
        if (hex.length() == 6) {
            float r = std::stoi(hex.substr(0, 2), nullptr, 16) / 255.0f;
            float g = std::stoi(hex.substr(2, 2), nullptr, 16) / 255.0f;
            float b = std::stoi(hex.substr(4, 2), nullptr, 16) / 255.0f;
            return glm::vec4(r, g, b, 1.0f);
        }
        else if (hex.length() == 3) {
            // Short form: each nibble is doubled (e.g. #F0A == #FF00AA)
            float r = std::stoi(hex.substr(0, 1), nullptr, 16) / 15.0f;
            float g = std::stoi(hex.substr(1, 1), nullptr, 16) / 15.0f;
            float b = std::stoi(hex.substr(2, 1), nullptr, 16) / 15.0f;
            return glm::vec4(r, g, b, 1.0f);
        }
    }

    // // RGB/RGBA format
    // if (_color.find("rgb(") == 0 || _color.find("rgba(") == 0) {
    //     std::string values = _color.substr(_color.find("(") + 1, _color.find(")") - _color.find("(") - 1);
    //     std::stringstream ss(values);
    //     std::string item;
    //     float r, g, b, a = 1.0f;

    //     std::getline(ss, item, ',');
    //     r = std::stof(item) / 255.0f;
    //     std::getline(ss, item, ',');
    //     g = std::stof(item) / 255.0f;
    //     std::getline(ss, item, ',');
    //     b = std::stof(item) / 255.0f;

    //     if (_color.find("rgba(") == 0) {
    //         std::getline(ss, item, ',');
    //         a = std::stof(item);
    //     }

    //     return glm::vec4(r, g, b, a);
    // }

    // Default to opaque white for unrecognised colour strings
    return glm::vec4(1.0f);
}
