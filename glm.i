// minimal SWIG (http://www.swig.org) interface wrapper for the glm library
// defines only the types used in openFrameworks: vec2, vec3, mat3, mat4, quat
// 2018 Dan Wilcox <danomatika@gmail.com>
// some parts adapted from https://github.com/IndiumGames/swig-wrapper-glm

// main MODULE
%module glm
%{
// #include <stdexcept>
#include <string>
#include <sstream>

// // these included in math/ofVectorMath.h
// // we declare some things manually, so some includes are commented out
// #include "glm/vec2.hpp"
// #include "glm/vec3.hpp"
// #include "glm/vec4.hpp"
// #include "glm/mat3x3.hpp"
// #include "glm/mat4x4.hpp"
// #include "glm/geometric.hpp"
// #include "glm/common.hpp"
// #include "glm/trigonometric.hpp"
// #include "glm/exponential.hpp"
// //#include "glm/vector_relational.hpp"
// //#include "glm/ext.hpp"

// //#include "glm/gtc/constants.hpp"
// #include "glm/gtc/matrix_transform.hpp"
// #include "glm/gtc/matrix_inverse.hpp"
// //#include "glm/gtc/quaternion.hpp"
// #include "glm/gtc/epsilon.hpp"
// #include "glm/gtx/norm.hpp"
// #include "glm/gtx/perpendicular.hpp"
// #include "glm/gtx/quaternion.hpp"
// #include "glm/gtx/rotate_vector.hpp"
// #include "glm/gtx/spline.hpp"
// #include "glm/gtx/transform.hpp"
// #include "glm/gtx/vector_angle.hpp"
// //#include "glm/gtx/scalar_multiplication.hpp"
// //#include <glm/gtc/type_ptr.hpp>

// // extras included via glm/ext.h
// #include <glm/gtc/matrix_access.hpp>
// #include <glm/ext/matrix_clip_space.hpp>
// #include <glm/ext/matrix_projection.hpp>
// #include <glm/gtx/compatibility.hpp>
// #include <glm/gtx/fast_square_root.hpp>
%}

// // ----- C++ -----

// %include <std_except.i>
%include <std_string.i>

// expanded primitives
%typedef unsigned int std::size_t;

namespace glm {

    %rename(add) operator+;
    %rename(sub) operator-;
    %rename(mul) operator*;
    %rename(div) operator/;
    %rename(eq) operator==;

    %typedef int length_t;

    // VEC2
    //
    // -----------------------------------------------------------
    //
    struct ivec2 {

        int x, y;

        ivec2();
        ivec2(ivec2 const & v);
        ivec2(int scalar);
        ivec2(int s1, int s2);
        ivec2(glm::ivec3 const & v);
        // ivec2(glm::vec4 const & v);

        ivec2 & operator=(ivec2 const & v);
    };

    ivec2 operator+(ivec2 const & v, int scalar);
    ivec2 operator+(int scalar, ivec2 const & v);
    ivec2 operator+(ivec2 const & v1, ivec2 const & v2);
    ivec2 operator-(ivec2 const & v, int scalar);
    ivec2 operator-(int scalar, ivec2 const & v);
    ivec2 operator-(ivec2 const & v1, ivec2 const & v2);
    ivec2 operator*(ivec2 const & v, int scalar);
    ivec2 operator*(int scalar, ivec2 const & v);
    ivec2 operator*(ivec2 const & v1, ivec2 const & v2);
    ivec2 operator/(ivec2 const & v, int scalar);
    ivec2 operator/(int scalar, ivec2 const & v);
    ivec2 operator/(ivec2 const & v1, ivec2 const & v2);
    bool operator==(ivec2 const & v1, ivec2 const & v2);
    bool operator!=(ivec2 const & v1, ivec2 const & v2);

    %extend ivec2 {
        
        // [] getter
        // out of bounds throws a string, which causes a Lua error
        int __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::ivec2::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::ivec2::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, int f) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::ivec2::__setitem__()");
                }
                (*$self)[i-1] = f;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::ivec2::__setitem__()");
                }
                (*$self)[i] = f;
            #endif
        }

        // tostring operator
        std::string __tostring() {
            std::stringstream str;
            for(glm::length_t i = 0; i < $self->length(); ++i) {
                str << (*$self)[i];
                if(i + 1 != $self->length()) {
                    str << " ";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        ivec2 operator+(ivec2 const & v) {return (*$self) + v;}
        ivec2 operator+(int scalar) {return (*$self) + scalar;}
        ivec2 operator-(ivec2 const & v) {return (*$self) - v;}
        ivec2 operator-(int scalar) {return (*$self) - scalar;}
        ivec2 operator*(ivec2 const & v) {return (*$self) * v;}
        ivec2 operator*(int scalar) {return (*$self) * scalar;}
        ivec2 operator/(ivec2 const & v) {return (*$self) / v;}
        ivec2 operator/(int scalar) {return (*$self) / scalar;}
        bool operator==(ivec2 const & v) {return (*$self) == v;}
        bool operator!=(ivec2 const & v) {return (*$self) != v;}
    };

    // IVEC3
    //
    // ----------------------------------------------------------
    //
    struct ivec3 {
    
        int x, y, z;

        // static length_t length();

        ivec3();
        ivec3(ivec3 const & v);
        ivec3(int scalar);
        ivec3(int s1, int s2, int s3);
        ivec3(glm::ivec2 const & a, int b);
        ivec3(int a, glm::ivec2 const & b);
        // ivec3(glm::ivec4 const & v);

        ivec3 & operator=(ivec3 const & v);
    };

    ivec3 operator+(ivec3 const & v, int scalar);
    ivec3 operator+(int scalar, ivec3 const & v);
    ivec3 operator+(ivec3 const & v1, ivec3 const & v2);
    ivec3 operator-(ivec3 const & v, int scalar);
    ivec3 operator-(int scalar, ivec3 const & v);
    ivec3 operator-(ivec3 const & v1, ivec3 const & v2);
    ivec3 operator*(ivec3 const & v, int scalar);
    ivec3 operator*(int scalar, ivec3 const & v);
    ivec3 operator*(ivec3 const & v1, ivec3 const & v2);
    ivec3 operator/(ivec3 const & v, int scalar);
    ivec3 operator/(int scalar, ivec3 const & v);
    ivec3 operator/(ivec3 const & v1, ivec3 const & v2);
    bool operator==(ivec3 const & v1, ivec3 const & v2);
    bool operator!=(ivec3 const & v1, ivec3 const & v2);

    %extend ivec3 {

        // [] getter
        // out of bounds throws a string, which causes a Lua error
        int __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::ivec3::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::ivec3::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, int f) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::ivec3::__setitem__()");
                }
                (*$self)[i-1] = f;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::ivec3::__setitem__()");
                }
                (*$self)[i] = f;
            #endif
        }

        // tostring operator
        std::string __tostring() {
            std::stringstream str;
            for(glm::length_t i = 0; i < $self->length(); ++i) {
                str << (*$self)[i];
                if(i + 1 != $self->length()) {
                    str << " ";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        ivec3 operator+(ivec3 const & v) {return (*$self) + v;}
        ivec3 operator+(int scalar) {return (*$self) + scalar;}
        ivec3 operator-(ivec3 const & v) {return (*$self) - v;}
        ivec3 operator-(int scalar) {return (*$self) - scalar;}
        ivec3 operator*(ivec3 const & v) {return (*$self) * v;}
        ivec3 operator*(int scalar) {return (*$self) * scalar;}
        ivec3 operator/(ivec3 const & v) {return (*$self) / v;}
        ivec3 operator/(int scalar) {return (*$self) / scalar;}
        bool operator==(ivec3 const & v) {return (*$self) == v;}
        bool operator!=(ivec3 const & v) {return (*$self) != v;}
    };

    // VEC2
    //
    // -----------------------------------------------------------
    //
    struct vec2 {

        float x, y;

        vec2();
        vec2(vec2 const & v);
        vec2(float scalar);
        vec2(float s1, float s2);
        vec2(glm::vec3 const & v);
        vec2(glm::vec4 const & v);

        vec2 & operator=(vec2 const & v);
    };

    vec2 operator+(vec2 const & v, float scalar);
    vec2 operator+(float scalar, vec2 const & v);
    vec2 operator+(vec2 const & v1, vec2 const & v2);
    vec2 operator-(vec2 const & v, float scalar);
    vec2 operator-(float scalar, vec2 const & v);
    vec2 operator-(vec2 const & v1, vec2 const & v2);
    vec2 operator*(vec2 const & v, float scalar);
    vec2 operator*(float scalar, vec2 const & v);
    vec2 operator*(vec2 const & v1, vec2 const & v2);
    vec2 operator/(vec2 const & v, float scalar);
    vec2 operator/(float scalar, vec2 const & v);
    vec2 operator/(vec2 const & v1, vec2 const & v2);
    bool operator==(vec2 const & v1, vec2 const & v2);
    bool operator!=(vec2 const & v1, vec2 const & v2);

    %extend vec2 {
        
        // [] getter
        // out of bounds throws a string, which causes a Lua error
        float __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::vec2::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::vec2::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, float f) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::vec2::__setitem__()");
                }
                (*$self)[i-1] = f;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::vec2::__setitem__()");
                }
                (*$self)[i] = f;
            #endif
        }

        // tostring operator
        std::string __tostring() {
            std::stringstream str;
            for(glm::length_t i = 0; i < $self->length(); ++i) {
                str << (*$self)[i];
                if(i + 1 != $self->length()) {
                    str << " ";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        vec2 operator+(vec2 const & v) {return (*$self) + v;}
        vec2 operator+(float scalar) {return (*$self) + scalar;}
        vec2 operator-(vec2 const & v) {return (*$self) - v;}
        vec2 operator-(float scalar) {return (*$self) - scalar;}
        vec2 operator*(vec2 const & v) {return (*$self) * v;}
        vec2 operator*(float scalar) {return (*$self) * scalar;}
        vec2 operator/(vec2 const & v) {return (*$self) / v;}
        vec2 operator/(float scalar) {return (*$self) / scalar;}
        bool operator==(vec2 const & v) {return (*$self) == v;}
        bool operator!=(vec2 const & v) {return (*$self) != v;}
    };

    // VEC3
    //
    // -----------------------------------------------------------
    //
    struct vec3 {
    
        float x, y, z;

        static length_t length();

        vec3();
        vec3(vec3 const & v);
        vec3(float scalar);
        vec3(float s1, float s2, float s3);
        vec3(glm::vec2 const & a, float b);
        vec3(float a, glm::vec2 const & b);
        vec3(glm::vec4 const & v);

        vec3 & operator=(vec3 const & v);
    };

    vec3 operator+(vec3 const & v, float scalar);
    vec3 operator+(float scalar, vec3 const & v);
    vec3 operator+(vec3 const & v1, vec3 const & v2);
    vec3 operator-(vec3 const & v, float scalar);
    vec3 operator-(float scalar, vec3 const & v);
    vec3 operator-(vec3 const & v1, vec3 const & v2);
    vec3 operator*(vec3 const & v, float scalar);
    vec3 operator*(float scalar, vec3 const & v);
    vec3 operator*(vec3 const & v1, vec3 const & v2);
    vec3 operator/(vec3 const & v, float scalar);
    vec3 operator/(float scalar, vec3 const & v);
    vec3 operator/(vec3 const & v1, vec3 const & v2);
    bool operator==(vec3 const & v1, vec3 const & v2);
    bool operator!=(vec3 const & v1, vec3 const & v2);


    %extend vec3 {

        // [] getter
        // out of bounds throws a string, which causes a Lua error
        float __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::vec3::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::vec3::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, float f) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::vec3::__setitem__()");
                }
                (*$self)[i-1] = f;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::vec3::__setitem__()");
                }
                (*$self)[i] = f;
            #endif
        }

        // tostring operator
        std::string __tostring() {
            std::stringstream str;
            for(glm::length_t i = 0; i < $self->length(); ++i) {
                str << (*$self)[i];
                if(i + 1 != $self->length()) {
                    str << " ";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        vec3 operator+(vec3 const & v) {return (*$self) + v;}
        vec3 operator+(float scalar) {return (*$self) + scalar;}
        vec3 operator-(vec3 const & v) {return (*$self) - v;}
        vec3 operator-(float scalar) {return (*$self) - scalar;}
        vec3 operator*(vec3 const & v) {return (*$self) * v;}
        vec3 operator*(float scalar) {return (*$self) * scalar;}
        vec3 operator/(vec3 const & v) {return (*$self) / v;}
        vec3 operator/(float scalar) {return (*$self) / scalar;}
        bool operator==(vec3 const & v) {return (*$self) == v;}
        bool operator!=(vec3 const & v) {return (*$self) != v;}
    };


    // VEC4
    //
    // -----------------------------------------------------------
    //
    struct vec4 {
    
        float x, y, z, w;

        static length_t length();

        vec4();
        vec4(vec4 const & v);
        vec4(float scalar);
        vec4(float s1, float s2, float s3, float s4);
        vec4(vec2 const & a, vec2 const & b);
        vec4(vec2 const & a, float b, float c);
        vec4(float a, vec2 const & b, float c);
        vec4(float a, float b, vec2 const & c);
        vec4(vec3 const & a, float b);
        vec4(float a, vec3 const & b);

        vec4 & operator=(vec4 const & v);
    };

    vec4 operator+(vec4 const & v, float scalar);
    vec4 operator+(float scalar, vec4 const & v);
    vec4 operator+(vec4 const & v1, vec4 const & v2);
    vec4 operator-(vec4 const & v, float scalar);
    vec4 operator-(float scalar, vec4 const & v);
    vec4 operator-(vec4 const & v1, vec4 const & v2);
    vec4 operator*(vec4 const & v, float scalar);
    vec4 operator*(float scalar, vec4 const & v);
    vec4 operator*(vec4 const & v1, vec4 const & v2);
    vec4 operator/(vec4 const & v, float scalar);
    vec4 operator/(float scalar, vec4 const & v);
    vec4 operator/(vec4 const & v1, vec4 const & v2);
    bool operator==(vec4 const & v1, vec4 const & v2);
    bool operator!=(vec4 const & v1, vec4 const & v2);

    %extend vec4 {

        // [] getter
        // out of bounds throws a string, which causes a Lua error
        float __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::vec4::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::vec4::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, float f) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::vec4::__setitem__()");
                }
                (*$self)[i-1] = f;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::vec4::__setitem__()");
                }
                (*$self)[i] = f;
            #endif
        }

        // tostring operator
        std::string __tostring() {
            std::stringstream str;
            for(glm::length_t i = 0; i < $self->length(); ++i) {
                str << (*$self)[i];
                if(i + 1 != $self->length()) {
                    str << " ";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        vec4 operator+(vec4 const & v) {return (*$self) + v;}
        vec4 operator+(float scalar) {return (*$self) + scalar;}
        vec4 operator-(vec4 const & v) {return (*$self) - v;}
        vec4 operator-(float scalar) {return (*$self) - scalar;}
        vec4 operator*(vec4 const & v) {return (*$self) * v;}
        vec4 operator*(float scalar) {return (*$self) * scalar;}
        vec4 operator/(vec4 const & v) {return (*$self) / v;}
        vec4 operator/(float scalar) {return (*$self) / scalar;}
        bool operator==(vec4 const & v) {return (*$self) == v;}
        bool operator!=(vec4 const & v) {return (*$self) != v;}
    };

    // MAT3
    //
    // -----------------------------------------------------------
    //
    struct mat3 {

        static length_t length();

        mat3();
        mat3(mat3 const & v);
        mat3(float scalar);
        mat3(float x0, float y0, float z0,
            float x1, float y1, float z1,
            float x2, float y2, float z2);
        mat3(vec3 const & v1, vec3 const & v2, vec3 const & v3);
        mat3(glm::mat4 const & m);

        mat3 & operator=(mat3 const & m);
    };

    mat3 operator+(mat3 const & m, float scalar);
    mat3 operator+(float scalar, mat3 const & m);
    mat3 operator+(mat3 const & m1, mat3 const & m2);
    mat3 operator-(mat3 const & m, float scalar);
    mat3 operator-(float scalar, mat3 const & m);
    mat3 operator-(mat3 const & m1, mat3 const & m2);
    mat3 operator*(mat3 const & m, float scalar);
    mat3 operator*(float scalar, mat3 const & m);
    mat3 operator*(mat3 const & m1, mat3 const & m2);
    vec3 operator*(vec3 const & v, mat3 const & m);
    vec3 operator*(mat3 const & m, vec3 const & v);
    mat3 operator/(mat3 const & m, float scalar);
    mat3 operator/(float scalar, mat3 const & m);
    mat3 operator/(mat3 const & m1, mat3 const & m2);
    vec3 operator/(mat3 const & m, vec3 const & v);
    vec3 operator/(vec3 const & v, mat3 const & m);
    bool operator==(mat3 const & m1, mat3 const & m2);
    bool operator!=(mat3 const & m1, mat3 const & m2);

    %extend mat3 {

        // [] getter
        // out of bounds throws a string, which causes a Lua error
        vec3 __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::mat3::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::mat3::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, vec3 v) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::mat3::__setitem__()");
                }
                (*$self)[i-1] = v;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::mat3::__setitem__()");
                }
                (*$self)[i] = v;
            #endif
        }

        // tostring operator
        std::string __tostring() {
            std::stringstream str;
            const glm::length_t width = $self->length();
            const glm::length_t height = (*$self)[0].length();
            for(glm::length_t row = 0; row < height; ++row) {
                for(glm::length_t col = 0; col < width; ++col) {
                    str << (*$self)[col][row];
                    if(col + 1 != width) {
                        str << "\t";
                    }
                }
                if(row + 1 != height) {
                    str << "\n";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        mat3 operator+(float scalar) {return (*$self) + scalar;}
        mat3 operator+(mat3 const & m) {return (*$self) + m;}
        mat3 operator-(float scalar) {return (*$self) - scalar;}
        mat3 operator-(mat3 const & m) {return (*$self) - m;}
        mat3 operator*(float scalar) {return (*$self) * scalar;}
        mat3 operator*(mat3 const & m) {return (*$self) * m;}
        vec3 operator*(vec3 const & v) {return (*$self) * v;}
        mat3 operator/(float scalar) {return (*$self) / scalar;}
        mat3 operator/(mat3 const & m) {return (*$self) / m;}
        vec3 operator/(vec3 const & v) {return (*$self) / v;}
        bool operator==(mat3 const & m) {return (*$self) == m;}
        bool operator!=(mat3 const & m1) {return (*$self) != m;}
    };

    // MAT4
    //
    // -----------------------------------------------------------
    //
    struct mat4 {

        static length_t length();

        mat4();
        mat4(mat4 const & v);
        mat4(float scalar);
        mat4(float x0, float y0, float z0, float w0,
            float x1, float y1, float z1, float w1,
            float x2, float y2, float z2, float w2,
            float x3, float y3, float z3, float w3);
        mat4(vec4 const & v1, vec4 const & v2, vec4 const & v3, vec4 const & v4);
        mat4(mat3 const & m);

        mat4 & operator=(mat4 const & m);
    };

    mat4 operator+(mat4 const & m, float scalar);
    mat4 operator+(float scalar, mat4 const & m);
    mat4 operator+(mat4 const & m1, mat4 const & m2);
    mat4 operator-(mat4 const & m, float scalar);
    mat4 operator-(float scalar, mat4 const & m);
    mat4 operator-(mat4 const & m1, mat4 const & m2);
    mat4 operator*(mat4 const & m, float scalar);
    mat4 operator*(float scalar, mat4 const & v);
    mat4 operator*(mat4 const & m1, mat4 const & m2);
    vec4 operator*(mat4 const & m, vec4 const & v);
    vec4 operator*(vec4 const & v, mat4 const & m);
    mat4 operator/(mat4 const & m, float scalar);
    mat4 operator/(float scalar, mat4 const & m);
    mat4 operator/(mat4 const & m1, mat4 const & m2);
    vec4 operator/(mat4 const & m, vec4 const & v);
    vec4 operator/(vec4 const & v, mat4 const & m);
    bool operator==(mat4 const & m1, mat4 const & m2);
    bool operator!=(mat4 const & m1, mat4 const & m2);

    %extend mat4 {

        // [] getter
        // out of bounds throws a string, which causes a Lua error
        vec4 __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::mat4::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::mat4::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, vec4 v) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::mat4::__setitem__()");
                }
                (*$self)[i-1] = v;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::mat4::__setitem__()");
                }
                (*$self)[i] = v;
            #endif
        }

        // tostring operator
        std::string __tostring() {
            std::stringstream str;
            const glm::length_t width = $self->length();
            const glm::length_t height = (*$self)[0].length();
            for(glm::length_t row = 0; row < height; ++row) {
                for(glm::length_t col = 0; col < width; ++col) {
                    str << (*$self)[col][row];
                    if(col + 1 != width) {
                        str << "\t";
                    }
                }
                if(row + 1 != height) {
                    str << "\n";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        mat4 operator+(float scalar) {return (*$self) + scalar;}
        mat4 operator+(mat4 const & m) {return (*$self) + m;}
        mat4 operator-(float scalar) {return (*$self) - scalar;}
        mat4 operator-(mat4 const & m) {return (*$self) - m;}
        mat4 operator*(float scalar) {return (*$self) * scalar;}
        mat4 operator*(mat4 const & m) {return (*$self) * m;}
        vec4 operator*(vec4 const & v) {return (*$self) * v;}
        mat4 operator/(float scalar) {return (*$self) / scalar;}
        mat4 operator/(mat4 const & m) {return (*$self) / m;}
        vec4 operator/(vec4 const & v) {return (*$self) / v;}
        bool operator==(mat4 const & m) {return (*$self) == m;}
        bool operator!=(mat4 const & m1) {return (*$self) != m;}
    };

    // QUAT
    //
    // -----------------------------------------------------------
    //
    struct quat {

        float x, y, z, w;

        static length_t length();

        quat();
        quat(quat const & q);
        quat(float s, vec3 const & v);
        quat(float w, float x, float y, float z);
        quat(vec3 const & u, vec3 const & v);
        quat(vec3 const & eulerAngles);
        quat(mat3 const & m);
        quat(mat4 const & m);
    };

    quat operator+(quat const & q, quat const & p);
    quat operator*(quat const & q, quat const & p);
    vec3 operator*(quat const & q, vec3 const & v);
    vec3 operator*(vec3 const & v, quat const & q);
    vec4 operator*(quat const & q, vec4 const & v);
    vec4 operator*(vec4 const & v, quat const & q);
    quat operator*(quat const & q, float const & s);
    quat operator*(float const & s, quat const & q);
    quat operator/(quat const & q, float const & s);
    bool operator==(quat const & q1, quat const & q2);
    bool operator!=(quat const & q1, quat const & q2);

    float length(quat const & q);

    quat normalize(quat const & q);
    float dot(quat const & x, quat const & y);
    quat mix(quat const & x, quat const & y, float a);
    // quat lerp(quat const & x, quat const & y, float a);
    quat slerp(quat const & x, quat const & y, float a);
    quat conjugate(quat const & q);
    quat inverse(quat const & q);
    // quat rotate(quat const & q, float const & angle, vec3 const & axis);
    vec3 eulerAngles(quat const & x);
    float roll(quat const & x);
    float pitch(quat const & x);
    float yaw(quat const & x);
    mat3 mat3_cast(quat const & x);
    mat4 mat4_cast(quat const & x);
    quat quat_cast(glm::mat3 const & x);
    quat quat_cast(glm::mat4 const & x);
    float angle(quat const & x);
    vec3 axis(quat const & x);
    quat angleAxis(float const & angle, vec3 const & axis);

    // not sure what to do with these yet
    // vec4<bool, P> lessThan(quat const & x, quat const & y);
    // vec4<bool, P> lessThanEqual(quat const & x, quat const & y);
    // vec4<bool, P> greaterThan(quat const & x, quat const & y);
    // vec4<bool, P> greaterThanEqual(quat const & x, quat const & y);
    // vec4<bool, P> equal(quat const & x, quat const & y);
    // vec4<bool, P> notEqual(quat const & x, quat const & y);
    // vec4<bool, P> isnan(quat const & x);
    // vec4<bool, P> isinf(quat const & x);

    %extend quat {

        // [] getter
        // out of bounds throws a string, which causes a Lua error
        float __getitem__(int i) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::quat::__getitem__()");
                }
                return (*$self)[i-1];
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::quat::__getitem__()");
                }
                return (*$self)[i];
            #endif
        }

        // [] setter
        // out of bounds throws a string, which causes a Lua error
        void __setitem__(int i, float f) throw (std::out_of_range) {
            #ifdef SWIGLUA
                if(i < 1 || i > $self->length()) {
                    throw std::out_of_range("in glm::quat::__setitem__()");
                }
                (*$self)[i-1] = f;
            #else
                if(i < 0 || i >= $self->length()) {
                    throw std::out_of_range("in glm::quat::__setitem__()");
                }
                (*$self)[i] = f;
            #endif
        }

        // to string operator
        std::string __tostring() {
            std::stringstream str;
            for(glm::length_t i = 0; i < $self->length(); ++i) {
                str << (*$self)[i];   
                if(i + 1 != $self->length()) {
                    str << " ";
                }
            }
            return str.str();
        }

        // extend operators, otherwise some languages (lua)
        // won't be able to act on objects directly (ie. v1 + v2)
        quat operator+(quat const & q) {return (*$self) + q;}
        quat operator*(quat const & q) {return (*$self) * q;}
        vec3 operator*(vec3 const & v) {return (*$self) * v;}
        vec4 operator*(vec4 const & v) {return (*$self) * v;}
        quat operator*(float const & s) {return (*$self) * s;}
        quat operator/(float const & s) {return (*$self) / s;}
        bool operator==(quat const & q) {return (*$self) == q;}
        bool operator!=(quat const & q) {return (*$self) != q;}
    };
}