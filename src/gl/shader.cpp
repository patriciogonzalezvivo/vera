#include <cstring>
#include <chrono>
#include <algorithm>
#include <iostream>

#include "vera/window.h"
#include "vera/ops/string.h"
#include "vera/gl/shader.h"
#include "vera/shaders/defaultShaders.h"
#include "vera/xr/xr.h"

#include "glm/gtc/type_ptr.hpp"

namespace vera {

Shader::Shader():
    m_fragmentSource(""),
    m_vertexSource(""),
    m_previousFragmentSource(""),
    m_previousVertexSource(""),
    m_program(0), m_fragmentShader(0), m_vertexShader(0),
    m_error_screen(SHOW_MAGENTA_SHADER),
    m_needsReloading(true) {

    // Define PLATFORM
    #if defined(__APPLE__)
    addDefine("PLATFORM_OSX");

    #elif defined(_WIN32)
    addDefine("PLATFORM_WIN");

    #elif defined(PLATFORM_RPI)
    addDefine("PLATFORM_RPI");

    #elif defined(__EMSCRIPTEN__)
    addDefine("PLATFORM_WEBGL");//, toString(getWebGLVersionNumber()));

    if (getXR() != NONE_XR_MODE)
        addDefine("PLATFORM_WEBXR", toString((int)getXR()));

    #else
    addDefine("PLATFORM_LINUX");

    #endif

    m_defineChange = true;
}

Shader::~Shader() {
    // Avoid crash when no command line arguments supplied
    if (isLoaded())
        glDeleteProgram(m_program);
}

void Shader::operator = (const Shader &_parent ) {
    m_fragmentSource = _parent.m_fragmentSource;
    m_vertexSource = _parent.m_vertexSource;
    m_defineChange = true;
    m_needsReloading = true;
}

void Shader::setSource(const std::string& _fragmentSrc, const std::string& _vertexSrc) {
    m_fragmentSource = _fragmentSrc;
    m_vertexSource = _vertexSrc;
    m_needsReloading = true;
}

bool Shader::load(const std::string& _fragmentSrc, const std::string& _vertexSrc, ShaderErrorResolve _onError, bool _verbose) {
    setVersionFromCode(_fragmentSrc);
    if (m_fragmentSource == "" || m_vertexSource == "") {
        m_fragmentSource = getDefaultSrc(FRAG_ERROR);
        m_vertexSource = getDefaultSrc(VERT_ERROR);
    }


    // VERTEX
    m_vertexShader = compileShader(_vertexSrc, GL_VERTEX_SHADER, _verbose);
    if (!m_vertexShader) {
        if (_onError == SHOW_MAGENTA_SHADER) {
            if (_verbose)
                printf("Error compiling vertex shader, loading magenta shader\n");
            load(getDefaultSrc(FRAG_ERROR), getDefaultSrc(VERT_ERROR), DONT_KEEP_SHADER, _verbose);
            return false;
        }
        else if (_onError == REVERT_TO_PREVIOUS_SHADER) {
            if (_verbose)
                printf("Error compiling vertex shader, reverting to default shader\n");
            load(m_previousFragmentSource, m_previousVertexSource, SHOW_MAGENTA_SHADER, _verbose);
            return false;
        }
    }

    // FRAGMENT
    m_fragmentShader = compileShader(_fragmentSrc, GL_FRAGMENT_SHADER, _verbose);
    if (!m_fragmentShader) {
        if (_onError == SHOW_MAGENTA_SHADER) {
            if (_verbose)
                printf("Error compiling fragment shader, loading magenta shader\n");
            load(getDefaultSrc(FRAG_ERROR), getDefaultSrc(VERT_ERROR), DONT_KEEP_SHADER, _verbose);
            return false;
        }
        else if (_onError == REVERT_TO_PREVIOUS_SHADER) {
            if (_verbose)
                printf("Error compiling fragment shader, reverting to default shader\n");
            load(m_previousFragmentSource, m_previousVertexSource, SHOW_MAGENTA_SHADER, _verbose);
            return false;
        }
    }

    // PROGRAM
    if (m_program != 0)
        glDeleteProgram(m_program);
    m_program = glCreateProgram();
    glAttachShader(m_program, m_vertexShader);
    glAttachShader(m_program, m_fragmentShader);
    glLinkProgram(m_program);

    // SUCCESS
    if (_onError != DONT_KEEP_SHADER) {
        // we need to explicitely remember the previous shader or REVERT_TO_PREVIOUS_SHADER always falls through to SHOW_MAGENTA_SHADER.
        m_fragmentSource = _fragmentSrc;
        m_vertexSource = _vertexSrc;
        m_previousFragmentSource = _fragmentSrc;
        m_previousVertexSource = _vertexSrc;
    }

    GLint isLinked;
    glGetProgramiv(m_program, GL_LINK_STATUS, &isLinked);

    if (isLinked == GL_FALSE) {
        GLint infoLength = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &infoLength);

        #if !defined(SWIG)
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            glGetProgramInfoLog(m_program, infoLength, NULL, &infoLog[0]);
            std::string error(infoLog.begin(),infoLog.end());
            // printf("Error linking shader:\n%s\n", error);
            std::cerr << "Error linking shader: " << error << std::endl;

            std::size_t start = error.find("line ")+5;
            std::size_t end = error.find_last_of(")");
            std::string lineNum = error.substr(start,end-start);
            if (toInt(lineNum) > 0)
                std::cerr << (unsigned)toInt(lineNum) << ": " << getLineNumber(_fragmentSrc,(unsigned)toInt(lineNum)) << std::endl;
        }
        #endif

        if (_verbose)
            printf("Linking fail, deleting program and loading error shader\n");
        glDeleteProgram(m_program);
        load(getDefaultSrc(FRAG_ERROR), getDefaultSrc(VERT_ERROR), DONT_KEEP_SHADER, _verbose);
        return false;
    }
    else {
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);

        m_needsReloading = false;
        // m_defineChange = false;
        return true;
    }
}

const GLint Shader::getAttribLocation(const std::string& _attribute) const {
    return glGetAttribLocation(m_program, _attribute.c_str());
}

bool Shader::reload() {
    if (inUse() && isLoaded()) {
        std::cout << "Reloading shader program " << getProgram() << ", all previous uniforms will be lost!" << std::endl;
        glUseProgram(0); // Unbind the current shader progra;
    } 
    return load(m_fragmentSource, m_vertexSource, m_error_screen, false);
}

void Shader::use() {
    if (isDirty()) {
        reload();
    }
    
    if (!inUse()) {
        glUseProgram(getProgram());
    }
        
    textureIndex = 0;
}

bool Shader::inUse() const {
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    return (getProgram() == (GLuint)currentProgram);
}

bool Shader::isLoaded() const {
    return m_program != 0;
}

GLuint Shader::compileShader(const std::string& _src, GLenum _type, bool _verbose) {
    std::string prolog = "";

    //
    // detect #version directive at the beginning of the shader, move it to the prolog and remove it from the shader
    //

    std::string srcBody; // _src stripped of any #version directive at the beginning
    bool zeroBasedLineDirective; // true for GLSL core 1.10 to 1.50
    bool srcVersionFound = _src.substr(0, 8) == "#version"; // true if user provided a #version directive at the beginning of _src

    if (srcVersionFound) {
        //
        // split _src into srcVersion and srcBody
        //

        std::istringstream srcIss(_src);

        // the version line can be read without checking the result of getline(), srcVersionFound == true implies this
        std::string srcVersion;
        std::getline(srcIss,srcVersion);

        // move the #version directive to the top of the prolog
        prolog += srcVersion + '\n';

        // copy the rest of the shader into srcBody
        std::ostringstream srcOss("");
        std::string dataRead;
        while (std::getline(srcIss,dataRead)) {
            srcOss << dataRead << '\n';
        }
        srcBody = srcOss.str();

        //
        // try to determine which version are we actually using
        //

        size_t glslVersionNumber = 0;
        std::istringstream versionIss(srcVersion);
        versionIss >> dataRead; // consume the "#version" string which is guaranteed to be there
        versionIss >> glslVersionNumber; // try to read the next token and convert it to a number

        //
        // determine if the glsl version number starts numbering the #line directive from 0 or from 1
        //
        // #version 100        : "es"   profile, numbering starts from 1
        // #version 110 to 150 : "core" profile, numbering starts from 0
        // #version 200        : "es"   profile, numbering starts from 1
        // #version 300 to 320 : "es"   profile, numbering starts from 1
        // #version 330+       : all           , numbering starts from 1
        //
        // Any malformed or invalid #version directives are of no interest here, the shader compiler
        // will take care of reporting this to the user later.
        //

        zeroBasedLineDirective = (glslVersionNumber >= 110 && glslVersionNumber <= 150);

    } else {
        // no #version directive found at the beginning of _src, which means...
        srcBody = _src; // ... _src contains the whole shader body and ...
        zeroBasedLineDirective = true; // ... glsl defaults to version 1.10, which starts numbering #line directives from 0.
    }

    // Only update the define stack string when it change
    if (m_defineChange) {
        m_defineStack = "";
        for (DefinesMap_it it = m_defines.begin(); it != m_defines.end(); it++)
            m_defineStack += "#define " + it->first + " " + it->second + '\n';
        m_defineChange = false;
    }
    prolog += m_defineStack;

    //
    // determine the #line offset to be used for conciliating lines in glsl error messages and the line number in the editor
    //
    // #line 0 : no version specified by user, glsl defaults to version 1.10, shader source used without modifications
    // #line 1 : user specified #version 1.10 to 1.50, which start numbering #line from 0, version line removed from _src
    // #line 2 : user specified a version #version other than 1.10 to 1.50, those start numbering #line from 1, version line removed from _src
    //

    size_t startLine = (srcVersionFound ? 1 : 0) + (zeroBasedLineDirective ? 0 : 1);
    prolog += "#line " + std::to_string(startLine) + "\n";

    // if (_verbose) {
    //     if (_type == GL_VERTEX_SHADER) {
    //         std::cout << "// ---------- Vertex Shader" << std::endl;
    //     }
    //     else {
    //         std::cout << "// ---------- Fragment Shader" << std::endl;
    //     }
    //     std::cout << prolog << std::endl;
    //     std::cout << srcBody << std::endl;
    // }

    const GLchar* sources[2] = {
        (const GLchar*) prolog.c_str(),
        (const GLchar*) srcBody.c_str()
    };

    GLuint shader = glCreateShader(_type);
    glShaderSource(shader, 2, sources, NULL);
    glCompileShader(shader);

    GLint isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    GLint infoLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

#if defined(PLATFORM_RPI) || defined(__EMSCRIPTEN__)
    if (infoLength > 1 && !isCompiled) {
#else
    if (infoLength > 1) {
#endif
        std::vector<GLchar> infoLog(infoLength);
        glGetShaderInfoLog(shader, infoLength, NULL, &infoLog[0]);
        std::cerr << "Found " << (isCompiled ? "warning " : "error") << " while compiling " << ((_type == GL_FRAGMENT_SHADER)? "fragment" : "vertex") << " shader" << std::endl;
        std::string error_msg = &infoLog[0];
        std::cerr << error_msg << std::endl;

        std::vector<std::string> chuncks = vera::split(error_msg, ' ');
        size_t line_number = 0;

#if defined(__APPLE__)
    // Error Message on Apple M1
        // ERROR: 0:41: 'color' : syntax error: syntax error

        std::vector<std::string> error_loc = vera::split(chuncks[1], ':');
        if (vera::isInt(error_loc[1]))
            line_number = vera::toInt(error_loc[1]);

#elif defined(_WIN32)

#elif defined(__EMSCRIPTEN__)

#else
    // Linux ARM
        // 0:41(2): error: syntax error, unexpected IDENTIFIER, expecting ',' or ';'

    // Linux iX86
        // Error Message on Mesa Intel(R) Iris(R) Plus Graohics (ICL GT2)
        // 0:41(2): error: syntax error, unexpected IDENTIFIER, expecting ',' or ';'

        // Error Message on NVdia GeForce GTX 1650
        // 0(41) : error C0000: syntax error, unexpected '.', expecting "::" at token "."

        std::string error_loc1 = vera::replaceAll(chuncks[0], "(", ":");
        error_loc1 = vera::replaceAll(error_loc1, ")", ":");
        std::vector<std::string> error_loc2 = vera::split(error_loc1, ':');
        if (vera::isInt(error_loc2[1]))
            line_number = vera::toInt(error_loc2[1]);

#endif

        // Print line (-/+ lines for context)
        if (line_number > 1) {
            line_number -= 2;
            std::vector<std::string> lines = vera::split(_src, '\n', true);
            for (size_t i = line_number; i < lines.size() && i < line_number + 3; i++)
                std::cerr << i + 1 << " " << lines[i] << std::endl;
        }

    }

    if (isCompiled == GL_FALSE) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void Shader::detach(GLenum _type) {
#ifndef __EMSCRIPTEN__
    bool vert = (GL_VERTEX_SHADER & _type) == GL_VERTEX_SHADER;
    bool frag = (GL_FRAGMENT_SHADER & _type) == GL_FRAGMENT_SHADER;

    if (vert) {
        glDeleteShader(m_vertexShader);
        glDetachShader(m_vertexShader, GL_VERTEX_SHADER);
    }

    if (frag) {
        glDeleteShader(m_fragmentShader);
        glDetachShader(m_fragmentShader, GL_FRAGMENT_SHADER);
    }
#endif
}

GLint Shader::getUniformLocation(const std::string& _uniformName) const {
    GLint loc = glGetUniformLocation(m_program, _uniformName.c_str());
    if (loc == -1){
        // std::cerr << "Uniform " << _uniformName << " not found" << std::endl;
    }
    return loc;
}

void Shader::setUniform(const std::string& _name, int _x) {
    if (inUse()) {
        glUniform1i(getUniformLocation(_name), _x);
    }
}

void Shader::setUniform(const std::string& _name, int _x, int _y) {
    if (inUse()) {
        glUniform2i(getUniformLocation(_name), _x, _y);
    }
}

void Shader::setUniform(const std::string& _name, int _x, int _y, int _z) {
    if (inUse()) {
        glUniform3i(getUniformLocation(_name), _x, _y, _z);
    }
}

void Shader::setUniform(const std::string& _name, int _x, int _y, int _z, int _w) {
    if (inUse()) {
        glUniform4i(getUniformLocation(_name), _x, _y, _z, _w);
    }
}

void Shader::setUniform(const std::string& _name, const int *_array, size_t _size) {
    GLint loc = getUniformLocation(_name);
    if (inUse()) {
        if (_size == 1) {
            glUniform1i(loc, _array[0]);
        }
        else if (_size == 2) {
            glUniform2i(loc, _array[0], _array[1]);
        }
        else if (_size == 3) {
            glUniform3i(loc, _array[0], _array[1], _array[2]);
        }
        else if (_size == 4) {
            glUniform4i(loc, _array[0], _array[1], _array[2], _array[3]);
        }
        else {
            std::cerr << "Passing matrix uniform as array, not supported yet" << std::endl;
        }
    }
}

void Shader::setUniform(const std::string& _name, float _x) {
    if (inUse()) {
        glUniform1f(getUniformLocation(_name), _x);
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y) {
    if (inUse()) {
        glUniform2f(getUniformLocation(_name), _x, _y);
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z) {
    if (inUse()) {
        glUniform3f(getUniformLocation(_name), _x, _y, _z);
    }
}

void Shader::setUniform(const std::string& _name, float _x, float _y, float _z, float _w) {
    if (inUse()) {
        glUniform4f(getUniformLocation(_name), _x, _y, _z, _w);
    }
}

void Shader::setUniform(const std::string& _name, const float *_array, size_t _size) {
    GLint loc = getUniformLocation(_name);
    if (inUse()) {
        if (_size == 1) {
            glUniform1f(loc, _array[0]);
        }
        else if (_size == 2) {
            glUniform2f(loc, _array[0], _array[1]);
        }
        else if (_size == 3) {
            glUniform3f(loc, _array[0], _array[1], _array[2]);
        }
        else if (_size == 4) {
            glUniform4f(loc, _array[0], _array[1], _array[2], _array[2]);
        }
        else {
            std::cerr << "Passing matrix uniform as array, not supported yet" << std::endl;
        }
    }
}

void Shader::setUniform(const std::string& _name, const glm::vec2 *_array, size_t _size) {
    if (inUse()) {
        glUniform2fv(getUniformLocation(_name), _size, glm::value_ptr(_array[0]));
    }
}

void Shader::setUniform(const std::string& _name, const glm::vec3 *_array, size_t _size) {
    if (inUse()) {
        glUniform3fv(getUniformLocation(_name), _size, glm::value_ptr(_array[0]));
    }
}

void Shader::setUniform(const std::string& _name, const glm::vec4 *_array, size_t _size) {
    if (inUse()) {
        glUniform4fv(getUniformLocation(_name), _size, glm::value_ptr(_array[0]));
    }
}

void Shader::setUniformTexture(const std::string& _name, GLuint _textureId, size_t _texLoc) {
    if (inUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void Shader::setUniformTexture(const std::string& _name, const Texture* _tex, size_t _texLoc) {
    setUniformTexture(_name, _tex->getTextureId(), _texLoc);
}

void Shader::setUniformTexture(const std::string& _name, const Fbo* _fbo, size_t _texLoc) {
    setUniformTexture(_name, _fbo->getTextureId(), _texLoc);
}

void Shader::setUniformDepthTexture(const std::string& _name, const Fbo* _fbo, size_t _texLoc) {
    setUniformTexture(_name, _fbo->getDepthTextureId(), _texLoc);
}

void Shader::setUniformTexture(const std::string& _name, const Texture* _tex) {
    setUniformTexture(_name, _tex->getTextureId(), textureIndex++);
}

void  Shader::setUniformTexture(const std::string& _name, const Fbo* _fbo) {
    setUniformTexture(_name, _fbo->getTextureId(), textureIndex++);
}

void  Shader::setUniformDepthTexture(const std::string& _name, const Fbo* _fbo) {
    setUniformTexture(_name, _fbo->getDepthTextureId(), textureIndex++);
}

void Shader::setUniformTextureCube(const std::string& _name, const TextureCube* _tex, size_t _texLoc) {
    if (inUse()) {
        glActiveTexture(GL_TEXTURE0 + _texLoc);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _tex->getTextureId());
        glUniform1i(getUniformLocation(_name), _texLoc);
    }
}

void  Shader::setUniformTextureCube(const std::string& _name, const TextureCube* _tex) {
    setUniformTextureCube(_name, _tex, textureIndex++);
}

void Shader::setUniform(const std::string& _name, const glm::mat2& _value, bool _transpose) {
    if (inUse()) {
        glUniformMatrix2fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat3& _value, bool _transpose) {
    if (inUse()) {
        glUniformMatrix3fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}

void Shader::setUniform(const std::string& _name, const glm::mat4& _value, bool _transpose) {
    if (inUse()) {
        glUniformMatrix4fv(getUniformLocation(_name), 1, _transpose, &_value[0][0]);
    }
}

}
