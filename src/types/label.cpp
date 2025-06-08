#include "vera/types/label.h"

#include "vera/window.h"
#include "vera/ops/draw.h"
#include "vera/ops/math.h"
#include "vera/ops/intersection.h"

#include <iostream>
#include <algorithm>

#define SCREEN_MARGIN 25.0
#define TEXT_MARGIN 10.0

namespace vera {

Label::Label() : m_text(""), m_type(LABEL_CENTER), m_bbox(nullptr), m_worldPos(nullptr) { 
}

Label::~Label() {
    // If not, we should delete the linked position if it was created by this label
    if (m_bEphemeral && m_worldPos != nullptr) {
        delete m_worldPos;
    }
}

// Ephemeral labels
Label::Label(const char* _text, glm::vec3 _position, LabelType _type, float _margin) {
    setText(_text);
    linkTo(new glm::vec3(_position));
    setType(_type);
    setMargin(_margin);
    m_bEphemeral = true; // This label is created to be used only once, so it will be deleted after rendering
}

Label::Label(const std::string& _text, glm::vec3 _position, LabelType _type, float _margin) : m_bbox(nullptr){
    setText(_text);
    linkTo(new glm::vec3(_position));
    setType(_type);
    setMargin(_margin);
    m_bEphemeral = true; // This label is created to be used only once, so it will be deleted after rendering
}

// Anchored labels
Label::Label(const char* _text, glm::vec3* _position, LabelType _type, float _margin) : m_bbox(nullptr) { 
    setText(_text);
    linkTo(_position);
    setType(_type);
    setMargin(_margin);
}

Label::Label(const char* _text, Node* _node, LabelType _type, float _margin) : m_bbox(nullptr) { 
    setText(_text);
    linkTo(_node);
    setType(_type);
    setMargin(_margin);
}

Label::Label(const char* _text, Model* _model, LabelType _type, float _margin) { 
    setText(_text);
    linkTo(_model);
    setType(_type);
    setMargin(_margin);
}

Label::Label(const std::string& _text, glm::vec3* _position, LabelType _type, float _margin) : m_bbox(nullptr){ 
    setText(_text);
    linkTo(_position);
    setType(_type);
    setMargin(_margin);
}

Label::Label(const std::string& _text, Node* _node, LabelType _type, float _margin) : m_bbox(nullptr) { 
    setText(_text);
    linkTo(_node);
    setType(_type);
    setMargin(_margin);
}

Label::Label(const std::string& _text, Model* _model, LabelType _type, float _margin) { 
    setText(_text);
    linkTo(_model);
    setType(_type);
    setMargin(_margin);
}

Label::Label(std::function<std::string(void)> _func, glm::vec3* _position, LabelType _type, float _margin) : m_bbox(nullptr){ 
    setText(_func);
    linkTo(_position);
    setType(_type);
    setMargin(_margin);
}

Label::Label(std::function<std::string(void)> _func, Node* _node, LabelType _type, float _margin) : m_bbox(nullptr) { 
    setText(_func);
    linkTo(_node);
    setType(_type);
    setMargin(_margin);
}

Label::Label(std::function<std::string(void)> _func, Model* _model, LabelType _type, float _margin) { 
    setText(_func);
    linkTo(_model);
    setType(_type);
    setMargin(_margin);
}

void Label::linkTo(glm::vec3* _position) { m_worldPos = _position; }
void Label::linkTo(Node* _node) { m_worldPos = &(_node->m_position); }
void Label::linkTo(Model* _model) { 
    m_worldPos = &(_model->m_position);
    m_bbox = &(_model->m_bbox);
}

std::string Label::getText() {
    if (m_textFunc)
        return m_textFunc();
    return m_text;
}
void Label::updateVisibility(Camera* _cam, float margin) {
    m_bVisible = true;

    // _cam->bChange
    if (_cam == nullptr)
        _cam = getCamera();

    if (m_bbox) {
        m_screenBox = _cam->worldToScreen(*m_bbox, getWorldMatrixPtr());
        m_screenBox.min.x *= vera::getWindowWidth(); 
        m_screenBox.max.x *= vera::getWindowWidth();
        m_screenBox.min.y *= vera::getWindowHeight(); 
        m_screenBox.max.y *= vera::getWindowHeight(); 
        m_screenPos = m_screenBox.getCenter();
    }
    else {
        m_screenPos = _cam->worldToScreen(m_worldPos, getWorldMatrixPtr());
        m_screenPos.x *= vera::getWindowWidth();
        m_screenPos.y *= vera::getWindowHeight();
        m_screenBox.set(m_screenPos);
    }
    
    m_screenBox.expand(m_margin);
    
    // Is in view? (depth and in viewport)
    if (m_screenPos.z >= 1.0  ||
        m_screenBox.max.x < margin || m_screenBox.min.x > vera::getWindowWidth() - margin ||
        m_screenBox.max.y < margin || m_screenBox.min.y > vera::getWindowHeight() - margin) {
        m_bVisible = false;
    }
}

void Label::updatePosition(Font *_font, float margin) {
    if (_font == nullptr)
        _font = getFont();
    
    if (m_type == LABEL_UP)
        m_screenPos.y -= m_screenBox.getHeight() * 0.5;
    else if (m_type == LABEL_DOWN)
        m_screenPos.y += m_screenBox.getHeight() * 0.5;
    else if (m_type == LABEL_LEFT)
        m_screenPos.x -= m_screenBox.getWidth() * 0.5;
    else if (m_type == LABEL_RIGHT)
        m_screenPos.x += m_screenBox.getWidth() * 0.5;

    if (m_type == LABEL_LINE_TO_WINDOW_BORDER) {
        _font->setAlign(vera::ALIGN_MIDDLE);
        _font->setAlign(vera::ALIGN_LEFT);
        m_line_points[0] = m_screenPos;
        m_line_points[1] = m_screenPos;
        m_line_points[2] = m_screenPos;

        // this function doesn't compare the screen positions of other labels
        // and never creates coorner on the lines. It just naively place labels
        // at the left or right of the screen.
        float w = getWidth() * vera::getDisplayPixelRatio();
        if (m_screenPos.x < vera::getWindowWidth() * 0.5) {
            m_screenPos.x = 0.0;
            m_line_points[0].x -= m_screenBox.getWidth();
            m_line_points[1].x = m_line_points[0].x;
            m_line_points[2].x = w + TEXT_MARGIN;
        }
        else {
            m_screenPos.x = vera::getWindowWidth() - w;
            m_line_points[0].x += m_screenBox.getWidth();
            m_line_points[1].x = m_line_points[0].x;
            m_line_points[2].x = m_screenPos.x - TEXT_MARGIN;
        }
    }

    max.z = min.z = m_screenPos.z;

    if (m_textFunc)
        m_text = m_textFunc();

    if (m_text.size() > 0) {
        set( _font->getBoundingBox( m_text, m_screenPos.x, m_screenPos.y) );
    }
}

void Label::updateFont(Font *_font) {
    _font->setAngle(0.0f);

    if (m_type == LABEL_LINE_TO_WINDOW_BORDER) {
        if (m_bLeft)
            _font->setAlign(vera::ALIGN_LEFT);
        else
            _font->setAlign(vera::ALIGN_RIGHT);
        
        if (m_bTop)
            _font->setAlign(vera::ALIGN_TOP);
        else
            _font->setAlign(vera::ALIGN_BOTTOM);
    }
    else {
        if (m_type == LABEL_CENTER) {
            _font->setAlign(vera::ALIGN_CENTER);
            _font->setAlign(vera::ALIGN_MIDDLE);
        }
        else if (m_type == LABEL_UP) {
            _font->setAlign(vera::ALIGN_CENTER);
            _font->setAlign(vera::ALIGN_BOTTOM);
        }
        else {
            _font->setAlign(vera::ALIGN_CENTER);
            _font->setAlign(vera::ALIGN_TOP);
        }

        if (m_bbox) {
            if (m_type == LABEL_LEFT)
                _font->setAngle(HALF_PI);
            else if (m_type == LABEL_RIGHT)
                _font->setAngle(-HALF_PI);
        }
    }
}

void Label::updateBoundingBox(Font *_font) {
    if (_font == nullptr)
        _font = getFont();

    if (m_textFunc)
        m_text = m_textFunc();

    glm::vec2 screenPos = m_screenPos;

    if (m_type == LABEL_LINE_TO_WINDOW_BORDER)
        if (m_line_points.size() > 0)
            screenPos = m_line_points[ m_line_points.size() - 1 ];

    updateFont(_font);

    if (m_textFunc)
        m_text = m_textFunc();

    if (m_text.size() > 0)
        set( textBoundingBox(m_text, screenPos, _font) );

    expand(m_margin);
    min.z = 0.0f;
    max.z = 0.0f;
}

void Label::update(Camera* _cam, Font *_font, float margin) {
    if (margin == 0.0f)
        margin = labelSettings().screenMargin * vera::getDisplayPixelRatio();

    updateVisibility(_cam, margin);
    if (!m_bVisible)
        return;
        
    // Update the screen position
    updatePosition(_font, margin);

    // Update bounding box
    updateBoundingBox(_font);
}


bool Label::_depthCheck (const Label* _a, const Label* _b) {
    return _a->m_screenPos.z < _b->m_screenPos.z;
}

bool Label::_heightCheck (const Label* _a, const Label* _b) {
    return _a->m_screenPos.y < _b->m_screenPos.y;
}

bool Label::collides(std::vector<Label*>& _labels, size_t _index) {
    if (_index >= _labels.size())
        return false;

    if (_labels[_index] == nullptr)
        return false;

    if (!_labels[_index]->m_bVisible)
        return false;

    for (size_t i = 0; i < _labels.size(); i++) {
        if (i == _index || _labels[i] == nullptr || !_labels[i]->m_bVisible)
            continue;

        if (_labels[i]->intersects((const BoundingBox*)_labels[_index]))
            return true;
    }

    return false;
}

bool Label::updateOnBorderPosition(const BoundingBox& window, float margin, Font* _font) {
    // if m_line_points[1] is out of the screen clamp it to the screen
    // if (m_line_points[1].x < margin || m_line_points[1].x > vera::getWindowWidth() - margin ||
    //     m_line_points[1].y < margin || m_line_points[1].y > vera::getWindowHeight() - margin) 
    if (!window.contains(m_line_points[1])) {
        glm::vec2 clamped = m_line_points[1];
        if (intersection(m_line_points[0], m_line_points[1], window, clamped)) {
            m_line_points[1] = clamped;
        }
        updateBoundingBox(_font);
        return true;
    }
    else {

        if (m_line_points.size() == 2) {
            m_line_points.push_back(m_line_points[1]);
        }

        // update the 3 and the final point
        if (m_bLeft) {
            m_line_points[2].x = margin;
        }
        else {
            m_line_points[2].x = vera::getWindowWidth() - margin;
        }

        m_line_points[2].y = m_line_points[1].y;
        updateBoundingBox(_font);
    }
    
    return false;
}

void Label::updateList(std::vector<Label*>& _labels, Camera* _cam, Font *_font) {
            
    // Get elements
    if (_cam == nullptr)
        _cam = getCamera();

    if (_font == nullptr)
        _font = getFont();

    float occlution_margin = labelSettings().occlusionMargin * vera::getDisplayPixelRatio();
    float margin = labelSettings().screenMargin * vera::getDisplayPixelRatio();
    const BoundingBox window = BoundingBox(margin, margin, vera::getWindowWidth() - margin * 2.0f, vera::getWindowHeight() - margin * 2.0f);
    const bool dynamicDisplacementOnBorder = labelSettings().dynamicDisplacementOnBorder;

    // NOTE: the original implementation at 
    // https://github.com/patriciogonzalezvivo/ofxLabels/blob/master/src/ofxLabels.cpp
    // have a screen center per label
    glm::vec2 radialCenter = labelSettings().radialCenter * glm::vec2(vera::getWindowWidth(), vera::getWindowHeight());

    // Update the 2D screen position
    for (size_t i = 0; i < _labels.size(); i++) {
        if (_labels[i] == nullptr || !_labels[i]->bEnabled)
            continue;

        _labels[i]->updateVisibility(_cam, margin);
    }

    // Account for depth collisions
    std::sort(_labels.begin(), _labels.end(), _depthCheck);
    for (unsigned int i = 0; i < _labels.size(); i++) {
        // Skip non visibles
        if (!_labels[i]->m_bVisible)
            continue;
        
        for (int j = i - 1; j >= 0; j--) {
            if (_labels[j]->m_bVisible) {
                
                // You don't need the z value any more
                _labels[i]->m_screenPos.z = 0;
                
                // Do they collide on screen space?
                float screen_distance = length(_labels[i]->m_screenPos - _labels[j]->m_screenPos);
                if ( screen_distance < occlution_margin) {
                    _labels[i]->m_bVisible = false;
                    break;
                }
            }
        }
    }

    // Account for label collisions place
    std::sort(_labels.begin(), _labels.end(), _heightCheck);
    std::vector<bool> bLeft = std::vector<bool>(_labels.size(), false);
    for (unsigned int i = 0; i < _labels.size(); i++) {
        _labels[i]->m_line_points.clear();

        // Skip non visibles
        if (!_labels[i]->m_bVisible || !_labels[i]->bEnabled || _labels[i]->m_type != LABEL_LINE_TO_WINDOW_BORDER) {
            continue;
        }

        const float label_width = _labels[i]->getWidth();// * vera::getDisplayPixelRatio();
        const float label_height = _labels[i]->getHeight();// * vera::getDisplayPixelRatio();

        // what boorder is closer: Right or Left / Top or Bottom
        _labels[i]->m_bLeft =  _labels[i]->m_screenPos.x < radialCenter.x;
        _labels[i]->m_bTop = _labels[i]->m_screenPos.y < radialCenter.y;

        // line_pooints 0, 1, 2 are 2d screen positions
        //         Text
        //     1------2
        //    /
        //   /
        //  0
        // +

        // 0 is the screen position of the object associated with the label
        _labels[i]->m_line_points.push_back(_labels[i]->m_screenPos);

        // 1 Offset diagonally from a custom screen center
        glm::vec2 fromFocus = _labels[i]->m_line_points[0] - radialCenter;
        _labels[i]->m_line_points.push_back( _labels[i]->m_line_points[0] + fromFocus * 0.5f );
        
        // add marging between the object and the begining of the line
        // This should be a parameter
        float dist = glm::length(_labels[i]->m_line_points[1] - _labels[i]->m_line_points[0]);
        glm::vec2 fromFocusDir = glm::normalize(fromFocus);
        if (margin < dist * 0.5f)
            _labels[i]->m_line_points[0] += fromFocusDir * _labels[i]->m_margin;

        // if it's outside the window, exit;
        if (!window.contains(_labels[i]->m_line_points[0])) {
            _labels[i]->m_bVisible = false;
            continue;
        }

        // update the bounding box
        _labels[i]->updateOnBorderPosition(window, margin, _font);

        // if it collides with other labels, march the tip of the line until it finds a free space
        if (dynamicDisplacementOnBorder) {
            while (collides(_labels, i)) {
                _labels[i]->m_line_points[1] += fromFocus * 0.1f;//* _labels[i]->m_margin;
                if (_labels[i]->updateOnBorderPosition(window, margin, _font))
                    break;
            }
        }

        _labels[i]->m_bVisible = !collides(_labels, i);
        if (_labels[i]->m_bVisible)
            _labels[i]->m_bVisible = !_labels[i]->contains(glm::vec2(_labels[i]->m_screenPos));
    }
}

void Label::render(Font *_font) {
    
    if (!m_bVisible || !bEnabled) {
        // noFill();
        // stroke(0.5f, 0.0f, 0.0f, 1.0f);
        // line(m_line_points);
        return;
    }
    
    if (_font == nullptr)
        _font = getFont();

    const BoundingBox bbox = BoundingBox((const BoundingBox*)this);
    glm::vec4 lineColor = labelSettings().lineColor;
    glm::vec4 textColor = labelSettings().textColor;
    const bool lineBorder = labelSettings().lineWidth == labelSettings().lineBorderWidth;
    
    if (m_type == LABEL_LINE_TO_WINDOW_BORDER) {
        if (m_line_points.size() < 3) {
            strokeWeight(10.0f);
            stroke(getBackground());
            line(m_line_points);

            // draw a line to the text position
            strokeWeight(1.0f);
            stroke(1.0f);
            line(m_line_points);
        }

        noStroke();
        
        fill( getBackground() );
        rect(bbox);
        
        if (m_line_points.size() >= 3) {
            strokeWeight(10.0f);
            stroke(getBackground());
            line(m_line_points);

            strokeWeight(1.0f);
            stroke(1.0f);
            line(m_line_points);
        }

        updateFont(_font);
        _font->render( m_text, m_line_points[ m_line_points.size() - 1] );
    }
    else {
        updateFont(_font);
        if (m_worldPos != nullptr)
            _font->render( m_text, m_screenPos.x, m_screenPos.y );
    }
}

void Label::renderList(std::vector<Label*>& _labels, Font *_font) {
    if (_labels.size() == 0)
        return;

    // Render all labels
    for (size_t i = 0; i < _labels.size(); i++) {
        if (_labels[i] == nullptr || !_labels[i]->bEnabled)
            continue;

        _labels[i]->render(_font);
        
        // If the label is ephemeral, remove it from the list
        if (_labels[i]->m_bEphemeral) {
            delete _labels[i];
            _labels[i] = nullptr;
        }
    }

    // Remove null elements
    _labels.erase(std::remove(_labels.begin(), _labels.end(), nullptr), _labels.end());
}

};
