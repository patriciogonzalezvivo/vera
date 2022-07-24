#include "vera/xr/holoPlay.h"
#include "vera/gl/gl.h"
#include "vera/ops/draw.h"
#include "vera/ops/string.h"

#include <fstream> 

#include "json.hpp"

namespace vera {

static QuiltProperties  quilt;
static Fbo              quilt_fbo;
static Shader           quilt_shader;
static int              currentViewIndex = 0;

//  QUILT 
QuiltProperties::QuiltProperties() {};
QuiltProperties::QuiltProperties(int _width, int _height, int _cols, int _rows) {
    width = _width;
    height = _height;
    columns = _cols;
    rows = _rows;
    totalViews = _cols * _rows;
};

void setQuiltProperties(const QuiltProperties& _quilt) { quilt = _quilt; }

static int quilt_resolutions[8][4] = {
    {2048, 2048, 4, 8},
    {4096, 4096, 5, 9},
    {8192, 8192, 5, 9},
    {3360, 3360, 8, 6},
    {4026, 4096, 11, 8},
    {4225, 4095, 13, 7},
    {4224, 4096, 12, 8},
    {4224, 4230, 12, 9}
};

void setQuiltProperties(size_t _index) {
    quilt = QuiltProperties(quilt_resolutions[_index][0], quilt_resolutions[_index][1], 
                            quilt_resolutions[_index][2], quilt_resolutions[_index][3] );
}

int getQuiltWidth() { return quilt.width; }
int getQuiltHeight() { return quilt.height; }
int getQuiltColumns() { return quilt.columns; }
int getQuiltRows() { return quilt.rows; }
int getQuiltTotalViews() { return quilt.totalViews; }
int getQuiltCurrentViewIndex() { return currentViewIndex; }

void renderQuilt(std::function<void(const QuiltProperties&, glm::vec4&, int&)> _renderFnc, bool _justQuilt) {

    Camera* cam = getCamera();
    if (!cam)
        return;


    // save the viewport for the total quilt
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // get quilt view dimensions
    int qs_viewWidth = int(float(quilt.width) / float(quilt.columns));
    int qs_viewHeight = int(float(quilt.height) / float(quilt.rows));

    if (!_justQuilt) {

        if (!quilt_fbo.isAllocated()) {
            quilt_fbo.allocate(quilt.width, quilt.height, COLOR_TEXTURE_DEPTH_BUFFER);
            quilt_shader.load(vera::getLenticularFragShader(vera::getVersion()), vera::getDefaultSrc(vera::VERT_BILLBOARD));
            quilt_shader.addDefine("QUILT");
            quilt_shader.addDefine("QUILT_WIDTH", vera::toString( quilt.width ));
            quilt_shader.addDefine("QUILT_HEIGHT", vera::toString( quilt.height ));
            quilt_shader.addDefine("QUILT_COLUMNS", vera::toString( quilt.columns ));
            quilt_shader.addDefine("QUILT_ROWS", vera::toString( quilt.rows ));
            quilt_shader.addDefine("QUILT_TOTALVIEWS", vera::toString( quilt.totalViews ));
        }

        Camera* cam = getCamera();
        if (cam)
            if (cam->getProjectionType() != ProjectionType::PERSPECTIVE_VIRTUAL_OFFSET) {
                // cam->setAspect(1.0);
                cam->setViewport(getWindowWidth(),getWindowHeight());
                cam->setFOV(glm::radians(14.0f));
                cam->setProjection(ProjectionType::PERSPECTIVE_VIRTUAL_OFFSET);
                // cam->setClipping(0.01, 100.0);
            }

        quilt_fbo.bind();
    }

    // render views and copy each view to the quilt
    for (int viewIndex = 0; viewIndex < quilt.totalViews; viewIndex++) {
        // get the x and y origin for this view
        int x = (viewIndex % quilt.columns) * qs_viewWidth;
        int y = int(float(viewIndex) / float(quilt.columns)) * qs_viewHeight;

        // get the x and y origin for this view
        // set the viewport to the view to control the projection extent
        glViewport(x, y, qs_viewWidth, qs_viewHeight);

        // // set the scissor to the view to restrict calls like glClear from making modifications
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, qs_viewWidth, qs_viewHeight);
        glm::vec4 vp = glm::vec4(x, y, qs_viewWidth, qs_viewHeight);

        currentViewIndex = viewIndex;
        
        _renderFnc(quilt, vp, viewIndex);

        // reset viewport
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

        // // restore scissor
        glDisable(GL_SCISSOR_TEST);
        glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
    }

    if (!_justQuilt) {
        quilt_fbo.unbind();

        quilt_shader.use();
        feedLenticularUniforms( quilt_shader );
        quilt_shader.setUniformTexture("u_scene", &quilt_fbo, 0);
        quilt_shader.setUniform("u_resolution", float(vera::getWindowWidth()), float(vera::getWindowHeight()) );
        quilt_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.));
        vera::getBillboard()->render( &quilt_shader );
    }
}

// LENTICULAR

static LenticularProperties lenticular;

void setLenticularProperties(const LenticularProperties& _lenticular) { lenticular = _lenticular; }
void setLenticularProperties(const std::string& _path) {
    std::ifstream file(_path.c_str(), std::ifstream::in);
    nlohmann::json manifest;
    file >> manifest;

    std::cout << "// LGF Holo Device found ("<< manifest["serial"] << ")" << std::endl;

    lenticular.dpi      = manifest["DPI"]["value"];
    lenticular.pitch    = manifest["pitch"]["value"];
    lenticular.slope    = manifest["slope"]["value"];
    lenticular.center   = manifest["center"]["value"];

    if (manifest["flipSubp"]["value"] == 0.0) {
        lenticular.ri       = 0;
        lenticular.bi       = 2;
    }
    else {
        lenticular.ri       = 2;
        lenticular.bi       = 0;
    }
}

const std::string lenticular_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;

uniform vec3        u_quilt;
uniform vec4        u_lenticularCalibration;  // dpi, pitch, slope, center
uniform vec2        u_lenticularRB;           // ri, bi

// GET CORRECT VIEW
vec2 quilt_map(vec3 tile, vec2 pos, float a) {
    vec2 tile2 = vec2(tile.x - 1.0, tile.y - 1.0);
    vec2 dir = vec2(-1.0, -1.0);

    a = fract(a) * tile.y;
    tile2.y += dir.y * floor(a);
    a = fract(a) * tile.x;
    tile2.x += dir.x * floor(a);
    return (tile2 + pos) / tile.xy;
}

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

    float pitch = -u_resolution.x / u_lenticularCalibration.x  * u_lenticularCalibration.y * sin(atan(abs(u_lenticularCalibration.z)));
    float tilt = u_resolution.y / (u_resolution.x * u_lenticularCalibration.z);
    float subp = 1.0 / (3.0 * u_resolution.x);
    float subp2 = subp * pitch;

    float a = (-st.x - st.y * tilt) * pitch - u_lenticularCalibration.w;
    color.r = texture2D(u_scene, quilt_map(u_quilt, st, a-u_lenticularRB.x*subp2)).r;
    color.g = texture2D(u_scene, quilt_map(u_quilt, st, a-subp2)).g;
    color.b = texture2D(u_scene, quilt_map(u_quilt, st, a-u_lenticularRB.y*subp2)).b;

    #if defined(HOLOPLAY_DEBUG_CENTER)
    // Mark center line only in central view
    color.r = color.r * 0.001 + (st.x>0.49 && st.x<0.51 && fract(a)>0.48&&fract(a)<0.51 ?1.0:0.0);
    color.g = color.g * 0.001 + st.x;
    color.b = color.b * 0.001 + st.y;

    #elif defined(HOLOPLAY_DEBUG)
    // use quilt texture
    color = texture2D(u_scene, st).rgb;
    #endif

    gl_FragColor = vec4(color,1.0);
}
)";

const std::string lenticular_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;

uniform vec3        u_quilt;
uniform vec4        u_lenticularCalibration;  // dpi, pitch, slope, center
uniform vec2        u_lenticularRB;           // ri, bi

out     vec4        fragColor;

// GET CORRECT VIEW
vec2 quilt_map(vec3 tile, vec2 pos, float a) {
    vec2 tile2 = vec2(tile.x - 1.0, tile.y - 1.0);
    vec2 dir = vec2(-1.0, -1.0);

    a = fract(a) * tile.y;
    tile2.y += dir.y * floor(a);
    a = fract(a) * tile.x;
    tile2.x += dir.x * floor(a);
    return (tile2 + pos) / tile.xy;
}

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;

    float pitch = -u_resolution.x / u_lenticularCalibration.x  * u_lenticularCalibration.y * sin(atan(abs(u_lenticularCalibration.z)));
    float tilt = u_resolution.y / (u_resolution.x * u_lenticularCalibration.z);
    float subp = 1.0 / (3.0 * u_resolution.x);
    float subp2 = subp * pitch;

    float a = (-st.x - st.y * tilt) * pitch - u_lenticularCalibration.w;
    color.r = texture(u_scene, quilt_map(u_quilt, st, a-u_lenticularRB.x*subp2)).r;
    color.g = texture(u_scene, quilt_map(u_quilt, st, a-subp2)).g;
    color.b = texture(u_scene, quilt_map(u_quilt, st, a-u_lenticularRB.y*subp2)).b;

    #if defined(HOLOPLAY_DEBUG_CENTER)
    // Mark center line only in central view
    color.r = color.r * 0.001 + (st.x>0.49 && st.x<0.51 && fract(a)>0.48&&fract(a)<0.51 ?1.0:0.0);
    color.g = color.g * 0.001 + st.x;
    color.b = color.b * 0.001 + st.y;

    #elif defined(HOLOPLAY_DEBUG)
    // use quilt texture
    color = texture(u_scene, st).rgb;
    #endif

    fragColor = vec4(color,1.0);
}
)";

std::string getLenticularFragShader(size_t _versionNumber) {
    std::string rta = "";

    if (_versionNumber < 130)
        rta += lenticular_frag;
    else if (_versionNumber >= 130) 
        rta += lenticular_frag_300;

    return rta;
}

void feedLenticularUniforms(Shader& _shader) {
    _shader.setUniform("u_quilt", float(quilt.columns), float(quilt.rows), float(quilt.totalViews));
    _shader.setUniform("u_lenticularCalibration", lenticular.dpi, lenticular.pitch, lenticular.slope, lenticular.center);
    _shader.setUniform("u_lenticularRB", float(lenticular.ri), float(lenticular.bi));
}

}