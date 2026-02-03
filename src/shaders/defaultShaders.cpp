#include "vera/shaders/defaultShaders.h"

#include "vera/shaders/default.h"
#include "vera/shaders/default_error.h"
#include "vera/shaders/default_scene.h"
#include "vera/shaders/default_buffers.h"

#include "vera/shaders/billboard.h"
#include "vera/shaders/dynamic_billboard.h"

#include "vera/shaders/plot.h"
#include "vera/shaders/fxaa.h"
#include "vera/shaders/poissonfill.h"
#include "vera/shaders/jumpFlood.h"
#include "vera/shaders/gsplat.h"

// 3D SCENE
#include "vera/shaders/light_ui.h"
#include "vera/shaders/cubemap.h"
#include "vera/shaders/draw.h"
#include "vera/shaders/devlook.h"

#include "vera/ops/fs.h"
#include "vera/ops/string.h"

namespace vera {

static size_t versionNumber = DEFAULT_GLSL_VERSION_NUMBER;
static bool versionES = false;
static std::string versionLine = "";

bool haveVersion(const std::string& _src) {
    return _src.substr(0, 8) == "#version";
}

void setVersionFromCode(const std::string& _src) {
    versionNumber = getVersionNumber(_src);
    versionES = getVersionES(_src);
    versionLine = getVersionLine();
}

int getVersionNumber() {
    return versionNumber;
}

int getVersionNumber(const std::string& _src) {
    std::string _versionLine = "";
    int _versionNumber = DEFAULT_GLSL_VERSION_NUMBER;

    if (haveVersion(_src)) {
        // split _src into srcVersion and srcBody
        std::istringstream srcIss(_src);

        // the version line can be read without checking the result of getline(), srcVersionFound == true implies this
        std::getline(srcIss, _versionLine);

        std::istringstream versionIss(_versionLine);
        std::string dataRead;
        versionIss >> dataRead;             // consume the "#version" string which is guaranteed to be there
        versionIss >> _versionNumber;       // try to read the next token and convert it to a number
    }

    return _versionNumber;
}

bool getVersionES() {
    return versionES;
}

bool getVersionES(const std::string& _src) {
    bool _versionES = false;

    if (haveVersion(_src)) {
        // split _src into srcVersion and srcBody
        std::istringstream srcIss(_src);

        // the version line can be read without checking the result of getline(), srcVersionFound == true implies this
        std::string versionLine;
        std::getline(srcIss, versionLine);

        // if versionLine ends with " es", then we are using GLSL ES
        std::istringstream versionIss(versionLine);
        std::string dataRead;
        versionIss >> dataRead; // consume the "#version" string which is guaranteed to be there
        versionIss >> dataRead; // read the version number
        versionIss >> dataRead; // read the optional "es" token

        if (dataRead == "es") {
            _versionES = true;
        }
    }

    return _versionES;
}

std::string getVersionLine() {
    return "#version " + std::to_string(versionNumber) + (versionES ? " es" : "");
}

std::string getDefaultSrc( DefaultShaders _type ) {
    std::string rta = versionLine + "\n";

    if (_type == VERT_DEFAULT) {
        if (versionNumber < 130)
            rta += default_vert;
        else if (versionNumber >= 130) 
            rta += default_vert_300;
    }
    else if (_type == FRAG_DEFAULT) {
        rta += default_frag;
    }
    else if (_type == FRAG_DEFAULT_TEXTURE) {
        rta += default_texture_frag;
    }
    else if (_type == VERT_DEFAULT_SCENE) {
        if (versionNumber < 130)
            rta += default_scene_vert;
        else if (versionNumber >= 130) 
            rta += default_scene_vert_300;
    }
    else if (_type == FRAG_DEFAULT_SCENE) {
        if (versionNumber < 130)
            rta += default_scene_frag;
        else if (versionNumber >= 130) 
            rta += default_scene_frag_300;
    }
    else if (_type == VERT_BILLBOARD) {
        if (versionNumber < 130)
            rta += billboard_vert;
        else if (versionNumber >= 130) 
            rta += billboard_vert_300;
    }
    else if (_type == VERT_DYNAMIC_BILLBOARD) {
        if (versionNumber < 130)
            rta += dynamic_billboard_vert;
        else if (versionNumber >= 130) 
            rta += dynamic_billboard_vert_300;
    }
    else if (_type == FRAG_DYNAMIC_BILLBOARD) {
        if (versionNumber < 130)
            rta += dynamic_billboard_frag;
        else if (versionNumber >= 130) 
            rta += dynamic_billboard_frag_300;
    }
    else if (_type == VERT_CUBEMAP) {
        if (versionNumber < 130)
            rta += cube_vert;
        else if (versionNumber >= 130) 
            rta += cube_vert_300;
    }
    else if (_type == FRAG_CUBEMAP) {
        if (versionNumber < 130)
            rta += cube_frag;
        else if (versionNumber >= 130) 
            rta += cube_frag_300;
    }
    else if (_type == VERT_ERROR) {
        if (versionNumber < 130)
            rta += error_vert;
        else if (versionNumber >= 130) 
            rta += error_vert_300;
    }
    else if (_type == FRAG_ERROR) {
        if (versionNumber < 130)
            rta += error_frag;
        else if (versionNumber >= 130) 
            rta += error_frag_300;
    }
    else if (_type == VERT_LIGHT) {
        if (versionNumber < 130)
            rta += light_vert;
        else if (versionNumber >= 130) 
            rta += light_vert_300;
    }
    else if (_type == FRAG_LIGHT) {
        if (versionNumber < 130)
            rta += light_frag;
        else if (versionNumber >= 130) 
            rta += light_frag_300;
    }

    else if (_type == VERT_FILL) {
        if (versionNumber < 130)
            rta += fill_vert;
        else if (versionNumber >= 130) 
            rta += fill_vert_300;
    }
    else if (_type == FRAG_FILL) {
        if (versionNumber < 130)
            rta += fill_frag;
        else if (versionNumber >= 130) 
            rta += fill_frag_300;
    }

    else if (_type == VERT_SPLINE_2D) {
        if (versionNumber < 130)
            rta += spline_2d_vert;
        else if (versionNumber >= 130) 
            rta += spline_2d_vert_300;
    }
    else if (_type == FRAG_SPLINE_2D) {
        if (versionNumber < 130)
            rta += spline_2d_frag;
        else if (versionNumber >= 130) 
            rta += spline_2d_frag_300;
    }

    else if (_type == VERT_SPLINE_3D) {
        if (versionNumber < 130)
            rta += spline_3d_vert;
        else if (versionNumber >= 130) 
            rta += spline_3d_vert_300;
    }
    else if (_type == FRAG_SPLINE_3D) {
        if (versionNumber < 130)
            rta += spline_3d_frag;
        else if (versionNumber >= 130) 
            rta += spline_3d_frag_300;
    }

    else if (_type == VERT_STROKE) {
        if (versionNumber < 130)
            rta += stroke_vert;
        else if (versionNumber >= 130) 
            rta += stroke_vert_300;
    }
    else if (_type == FRAG_STROKE) {
        if (versionNumber < 130)
            rta += stroke_frag;
        else if (versionNumber >= 130) 
            rta += stroke_frag_300;
    }

    else if (_type == VERT_POINTS) {
        if (versionNumber < 130)
            rta += points_vert;
        else if (versionNumber >= 130) 
            rta += points_vert_300;
    }
    else if (_type == FRAG_POINTS) {
        if (versionNumber < 130)
            rta += points_frag;
        else if (versionNumber >= 130) 
            rta += points_frag_300;
    }

    else if (_type == FRAG_POSITION) {
        if (versionNumber < 130)
            rta += default_buffer_position;
        else if (versionNumber >= 130) 
            rta += default_buffer_position_300;
    }
    else if (_type == FRAG_NORMAL) {
        if (versionNumber < 130)
            rta += default_buffer_normal;
        else if (versionNumber >= 130) 
            rta += default_buffer_normal_300;
    }

    else if (_type == FRAG_PLOT) {
        if (versionNumber < 130)
            rta += vera::resolveGlsl(plot_frag);
        else if (versionNumber >= 130) 
            rta += vera::resolveGlsl(plot_frag_300);
    }
    else if (_type == FRAG_FXAA) {
        if (versionNumber < 130)
            rta += fxaa_frag;
        else if (versionNumber >= 130) 
            rta += fxaa_frag_300;
    }
    else if (_type == FRAG_POISSONFILL) {
        if (versionNumber < 130)
            rta += poissonfill_frag;
        else if (versionNumber >= 130) 
            rta += poissonfill_frag_300;
    } 
    else if (_type == FRAG_JUMPFLOOD) {
        if (versionNumber < 130)
            rta += jumpflood_frag;
        else if (versionNumber >= 130) 
            rta += jumpflood_frag_300;
    }
    else if (_type == VERT_DEVLOOK_BILLBOARD) {
        if (versionNumber < 130)
            rta += devlook_billboard_vert;
        else if (versionNumber >= 130) 
            rta += devlook_billboard_vert_300;
    }
    else if (_type == VERT_DEVLOOK_SPHERE) {
        if (versionNumber < 130)
            rta += devlook_sphere_vert;
        else if (versionNumber >= 130) 
            rta += devlook_sphere_vert_300;
    }
    else if (_type == VERT_SPLAT) {
        if (versionNumber < 130)
            rta += splat_vert;
        else if (versionNumber >= 130) 
            rta += splat_vert_300;
    }
    else if (_type == FRAG_SPLAT) {
        if (versionNumber < 130)
            rta += splat_frag;
        else if (versionNumber >= 130) 
            rta += splat_frag_300;
    }

    return rta;
}

}