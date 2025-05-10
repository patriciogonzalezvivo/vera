%begin %{
#if defined( __WIN32__ ) || defined( _WIN32 )
	#include <cmath>
#endif
%}

// %include "glfw3.i"

%module vera

%include <typemaps.i>
%include <std_vector.i>
%include <std_string.i>

%ignore *::operator[];
%ignore *::operator=;
%ignore *::operator==;
%ignore *::operator!=;
%ignore *::operator<;
%ignore *::operator<=;
%ignore *::operator>;
%ignore *::operator>=;
%ignore *::operator<<;
%ignore *::operator>>;
%ignore operator<<;

%{
    #define SWIG_FILE_WITH_INIT
    #include "vera/gl/gl.h"
    #include "vera/gl/texture.h"
    #include "vera/gl/vbo.h"
    #include "vera/types/node.h"
    #include "vera/gl/fbo.h"
    #include "vera/gl/cubemapFace.h"
    #include "vera/gl/defines.h"
    #include "vera/gl/pingpong.h"
    #include "vera/gl/pyramid.h"
    #include "vera/gl/shader.h"
    #include "vera/gl/textureBump.h"
    #include "vera/gl/textureCube.h"
    #include "vera/gl/textureProps.h"
    #include "vera/gl/textureStream.h"
    #include "vera/gl/textureStreamAudio.h"
    #include "vera/gl/textureStreamAV.h"
    #include "vera/gl/textureStreamMMAL.h"
    #include "vera/gl/textureStreamOMX.h"
    #include "vera/gl/textureStreamSequence.h"
    #include "vera/gl/vertexLayout.h"
    #include "vera/io/ply.h"
    #include "vera/io/stl.h"
    #include "vera/io/obj.h"
    #include "vera/io/gltf.h"
    #include "vera/ops/color.h"
    #include "vera/ops/draw.h"
    #include "vera/ops/env.h"
    #include "vera/ops/fs.h"
    #include "vera/ops/geom.h"
    #include "vera/ops/image.h"
    #include "vera/ops/intersection.h"
    #include "vera/ops/math.h"
    #include "vera/ops/meshes.h"
    #include "vera/ops/pixel.h"
    #include "vera/ops/string.h"
    #include "vera/ops/time.h"
    #include "vera/types/boundingBox.h"
    #include "vera/types/bvh.h"
    #include "vera/types/camera.h"
    #include "vera/types/font.h"
    #include "vera/types/image.h"
    #include "vera/types/label.h"
    #include "vera/types/line.h"
    #include "vera/types/material.h"
    #include "vera/types/mesh.h"
    #include "vera/types/model.h"
    #include "vera/types/polarPoint.h"
    #include "vera/types/polyline.h"
    #include "vera/types/plane.h"
    #include "vera/types/props.h"
    #include "vera/types/ray.h"
    #include "vera/types/scene.h"
    #include "vera/types/sky.h"
    #include "vera/types/triangle.h"
    #include "vera/xr/holoPlay.h"
    #include "vera/xr/xr.h"
    #include "vera/app.h"
    #include "vera/window.h"
%}

%include "glm.i"
%include "numpy.i"
%init %{
    import_array();
%}

%typemap(in) size_t {
    $1 = PyInt_AsLong($input);
}

%typemap(in) uint16_t {
    $1 = PyInt_AsLong($input);
}

%typemap(in) uint32_t {
    $1 = PyInt_AsLong($input);
}

// %apply (int* IN_ARRAY1, int DIM1 ) {(const int* _array1D, int _n )};
// %apply (int* IN_ARRAY2, int DIM1, int DIM2 ) {(const int* _array2D, int _m, int _n )};

// %apply (uint16_t* IN_ARRAY1, int DIM1 ) {(const uint16_t* _array1D, int _n )};
// %apply (uint16_t* IN_ARRAY2, int DIM1, int DIM2 ) {(const uint16_t* _array2D, int _m, int _n )};

// %apply (uint32_t* IN_ARRAY1, int DIM1 ) {(const uint32_t* _array1D, int _n )};
// %apply (uint32_t* IN_ARRAY2, int DIM1, int DIM2 ) {(const uint32_t* _array2D, int _m, int _n )};

// %apply (float* IN_ARRAY1, int DIM1 ) {(const float* _array1D, int _n )};
// %apply (float* IN_ARRAY2, int DIM1, int DIM2 ) {(const float* _array2D, int _m, int _n )};

// %apply (uint8_t* IN_ARRAY3, int DIM1, int DIM2, int DIM3) { (const uint8_t* _array3D, int _height, int _width, int _channels) }
// %apply (uint8_t** ARGOUTVIEWM_ARRAY3, int* DIM1, int* DIM2, int* DIM3) { (uint8_t **_array3D, int *_height, int *_width, int *_channels) }

namespace std {
    %template(Vector4DVector)   vector<glm::vec4>;
    %template(Vector3DVector)   vector<glm::vec3>;
    %template(Vector2DVector)   vector<glm::vec2>;

    %template(FacesVector)      vector<glm::ivec3>;

    %template(TrianglesVector)  vector<vera::Triangle>;
    %template(LinesVector)      vector<vera::Line>;
    %template(MeshesVector)     vector<vera::Mesh>;
    %template(ImagesVector)     vector<vera::Image>;

    %template(StringList)       vector<string>;
    %template(FloatVector)      vector<float>;
};

%include "include/vera/types/node.h"
%include "include/vera/ops/color.h"
%include "include/vera/ops/math.h"
%include "include/vera/ops/pixel.h"
%include "include/vera/ops/string.h"
%include "include/vera/ops/time.h"
%include "include/vera/types/ray.h"
%include "include/vera/types/line.h"
%include "include/vera/types/image.h"
%include "include/vera/types/props.h"
%include "include/vera/types/sky.h"
%include "include/vera/gl/gl.h"
%include "include/vera/gl/defines.h"
%include "include/vera/gl/cubemapFace.h"
%include "include/vera/gl/textureProps.h"
%include "include/vera/gl/texture.h"
%include "include/vera/gl/textureBump.h"
%include "include/vera/gl/textureCube.h"
%include "include/vera/gl/textureStream.h"
%include "include/vera/gl/textureStreamAudio.h"
%include "include/vera/gl/textureStreamAV.h"
%include "include/vera/gl/textureStreamMMAL.h"
%include "include/vera/gl/textureStreamOMX.h"
%include "include/vera/gl/textureStreamSequence.h"
%include "include/vera/gl/fbo.h"
%include "include/vera/gl/pingpong.h"
%include "include/vera/gl/pyramid.h"
%include "include/vera/gl/shader.h"
%include "include/vera/gl/vertexLayout.h"
%include "include/vera/types/material.h"
%include "include/vera/types/triangle.h"
%include "include/vera/types/boundingBox.h"
%include "include/vera/types/bvh.h"
%include "include/vera/types/plane.h"
%include "include/vera/types/mesh.h"
%include "include/vera/types/camera.h"
%include "include/vera/types/light.h"
%include "include/vera/gl/vbo.h"
%include "include/vera/types/font.h"
%include "include/vera/types/model.h"
%include "include/vera/types/polarPoint.h"
%include "include/vera/types/polyline.h"
%include "include/vera/types/label.h"
%include "include/vera/types/scene.h"
%include "include/vera/io/ply.h"
%include "include/vera/io/stl.h"
%include "include/vera/io/obj.h"
%include "include/vera/io/gltf.h"
%include "include/vera/ops/image.h"
%include "include/vera/ops/meshes.h"
%include "include/vera/ops/intersection.h"
%include "include/vera/ops/env.h"
%include "include/vera/ops/fs.h"
%include "include/vera/ops/geom.h"
%include "include/vera/shaders/defaultShaders.h"
%include "include/vera/ops/draw.h"
%include "include/vera/xr/holoPlay.h"
%include "include/vera/xr/xr.h"
%include "include/vera/window.h"
%include "include/vera/app.h"

// using namespace vera;