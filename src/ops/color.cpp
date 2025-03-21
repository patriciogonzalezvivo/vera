#include "vera/ops/color.h"


glm::vec4 toVec4(const std::string& _color) {

    // TODO replace this with a table

    // CSS color names
    if (_color == "black") return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    else if (_color == "white") return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    else if (_color == "red") return glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    else if (_color == "green") return glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    else if (_color == "blue") return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    else if (_color == "yellow") return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    else if (_color == "cyan") return glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
    else if (_color == "magenta") return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
    else if (_color == "gray" || _color == "grey") return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    else if (_color == "transparent") return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    else if (_color == "orange") return glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    else if (_color == "purple") return glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
    else if (_color == "pink") return glm::vec4(1.0f, 0.75f, 0.8f, 1.0f);
    else if (_color == "brown") return glm::vec4(0.65f, 0.16f, 0.16f, 1.0f);
    else if (_color == "navy") return glm::vec4(0.0f, 0.0f, 0.5f, 1.0f);
    else if (_color == "teal") return glm::vec4(0.0f, 0.5f, 0.5f, 1.0f);
    else if (_color == "lime") return glm::vec4(0.0f, 1.0f, 0.5f, 1.0f);
    else if (_color == "gold") return glm::vec4(1.0f, 0.84f, 0.0f, 1.0f);
    else if (_color == "silver") return glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);
    else if (_color == "indigo") return glm::vec4(0.29f, 0.0f, 0.51f, 1.0f);
    else if (_color == "violet") return glm::vec4(0.93f, 0.51f, 0.93f, 1.0f);
    else if (_color == "orchid") return glm::vec4(0.85f, 0.44f, 0.84f, 1.0f);
    else if (_color == "coral") return glm::vec4(1.0f, 0.5f, 0.31f, 1.0f);
    else if (_color == "salmon") return glm::vec4(0.98f, 0.5f, 0.45f, 1.0f);
    else if (_color == "khaki") return glm::vec4(0.94f, 0.9f, 0.55f, 1.0f);
    else if (_color == "plum") return glm::vec4(0.87f, 0.63f, 0.87f, 1.0f);
    else if (_color == "lavender") return glm::vec4(0.9f, 0.9f, 0.98f, 1.0f);
    else if (_color == "peachpuff") return glm::vec4(1.0f, 0.85f, 0.73f, 1.0f);
    else if (_color == "tan") return glm::vec4(0.82f, 0.71f, 0.55f, 1.0f);
    else if (_color == "wheat") return glm::vec4(0.96f, 0.87f, 0.7f, 1.0f);
    else if (_color == "thistle") return glm::vec4(0.85f, 0.75f, 0.85f, 1.0f);
    else if (_color == "mintcream") return glm::vec4(0.96f, 1.0f, 0.98f, 1.0f);
    else if (_color == "seashell") return glm::vec4(1.0f, 0.96f, 0.93f, 1.0f);
    else if (_color == "snow") return glm::vec4(1.0f, 0.98f, 0.98f, 1.0f);
    else if (_color == "aliceblue") return glm::vec4(0.94f, 0.97f, 1.0f, 1.0f);
    else if (_color == "antiquewhite") return glm::vec4(0.98f, 0.92f, 0.84f, 1.0f);
    else if (_color == "beige") return glm::vec4(0.96f, 0.96f, 0.86f, 1.0f);
    else if (_color == "bisque") return glm::vec4(1.0f, 0.89f, 0.77f, 1.0f);
    else if (_color == "blanchedalmond") return glm::vec4(1.0f, 0.92f, 0.8f, 1.0f);
    else if (_color == "burlywood") return glm::vec4(0.87f, 0.72f, 0.53f, 1.0f);
    else if (_color == "chartreuse") return glm::vec4(0.5f, 1.0f, 0.31f, 1.0f);
    else if (_color == "chocolate") return glm::vec4(0.82f, 0.41f, 0.12f, 1.0f);
    else if (_color == "cornflowerblue") return glm::vec4(0.39f, 0.58f, 0.93f, 1.0f);
    else if (_color == "cornsilk") return glm::vec4(1.0f, 0.97f, 0.86f, 1.0f);
    else if (_color == "crimson") return glm::vec4(0.86f, 0.08f, 0.24f, 1.0f);
    else if (_color == "darkblue") return glm::vec4(0.00f, 0.00f, 0.55f, 1.0f);
    else if (_color == "darkcyan") return glm::vec4(0.00f, 0.55f, 0.55f, 1.0f);
    else if (_color == "darkgray" || _color == "darkgrey") return glm::vec4(0.66f, 0.66f, 0.66f, 1.0f);
    else if (_color == "darkgreen") return glm::vec4(0.00f, 0.39f, 0.00f, 1.0f);
    else if (_color == "darkkhaki") return glm::vec4(0.74f, 0.72f, 0.42f, 1.0f);
    else if (_color == "darkorange") return glm::vec4(1.00f, 0.55f, 0.00f, 1.0f);
    else if (_color == "darkorchid") return glm::vec4(0.6f, 0.2f, 0.8f, 1.0f);
    else if (_color == "darkred") return glm::vec4(0.55f, 0.00f, 0.00f, 1.0f);
    else if (_color == "darksalmon") return glm::vec4(0.91f, 0.59f, 0.48f, 1.0f);
    else if (_color == "darkseagreen") return glm::vec4(0.56f, 0.74f, 0.56f, 1.0f);
    else if (_color == "darkslateblue") return glm::vec4(0.28f, 0.24f, 0.55f, 1.0f);
    else if (_color == "darkslategray" || _color == "darkslategrey") return glm::vec4(0.18f, 0.31f, 0.31f, 1.0f);
    else if (_color == "darkturquoise") return glm::vec4(0.00f, 0.81f, 1.00f, 1.0f);
    else if (_color == "darkviolet") return glm::vec4(0.58f, 0.00f, 0.83f, 1.0f);

    // Hexadecimal color format
    if (_color[0] == '#') {
        std::string hex = _color.substr(1);
        if (hex.length() == 6) {
            float r = std::stoi(hex.substr(0, 2), nullptr, 16) / 255.0f;
            float g = std::stoi(hex.substr(2, 2), nullptr, 16) / 255.0f;
            float b = std::stoi(hex.substr(4, 2), nullptr, 16) / 255.0f;
            return glm::vec4(r, g, b, 1.0f);
        }
        else if (hex.length() == 3) {
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

    // Default to white if color is not recognized
    return glm::vec4(1.0f);
}