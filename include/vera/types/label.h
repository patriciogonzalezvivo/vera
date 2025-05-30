#pragma once

#include <string>
#include <functional>

#include "camera.h"
#include "model.h"
#include "font.h"
#include "../window.h"

namespace vera {

enum LabelType {
    LABEL_CENTER = 0,
    LABEL_UP, LABEL_DOWN, LABEL_LEFT, LABEL_RIGHT,
    LABEL_LINE_TO_WINDOW_BORDER
};

class Label : public BoundingBox {
public:
    Label();

    Label(const char* _text, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
    Label(const char* _text, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
    Label(const char* _text, Model* _model, LabelType _type = LABEL_CENTER, float _margin = 0.0f );

    Label(const std::string& _text, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
    Label(const std::string& _text, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
    Label(const std::string& _text, Model* _model, LabelType _type = LABEL_CENTER, float _margin = 0.0f );
    
    Label(std::function<std::string(void)> _func, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
    Label(std::function<std::string(void)> _func, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
    Label(std::function<std::string(void)> _func, Model* _model, LabelType _type = LABEL_CENTER, float _margin = 0.0f );

    void setText(const char* _text) { m_text = std::string(_text); }
    void setText(const std::string& _text) { m_text = _text; }
    void setText(std::function<std::string(void)> _func) { m_textFunc = _func; }

    void linkTo(glm::vec3* _position);
    void linkTo(Node* _position);
    void linkTo(Model* _position);

    void setType(LabelType _type) { m_type = _type; };
    void setMargin(float _margin) { m_margin = _margin * vera::getDisplayPixelRatio(); };

    virtual glm::vec3   getScreenPosition() const { return m_screenPos; }
    virtual float       getMargin() const { return m_margin; }
    virtual std::string getText();
    
    void update(Camera* _cam = nullptr, Font *_font = nullptr);
    void render(Font *_font = nullptr);

    bool            bVisible;
    bool            bEnabled = true;

private:
    std::function<std::string(void)> m_textFunc;

    std::string     m_text;
    BoundingBox     m_screenBox;
    glm::vec3       m_screenPos;
    glm::vec2       m_line_points[3]; 
    LabelType       m_type;
    float           m_margin;

    BoundingBox*    m_bbox;
    glm::vec3*      m_worldPos;
};

}
