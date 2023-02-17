#!/usr/bin/env python

"""
Hilma is 3D geometry library in C++ with Python bindings (with support for Numpy) ready to be embed into your projects. 
"""

from distutils.core import setup, Extension

# Third-party modules - we depend on numpy for everything
import numpy

# Obtain the numpy include directory.  This logic works across numpy versions.
try:
    numpy_include = numpy.get_include()
except AttributeError:
    numpy_include = numpy.get_numpy_include()

doc_lines = __doc__.split('\n')
hilma_module = Extension(  
    '_vera',
    include_dirs=['include', numpy_include],
    sources= [ 
      'vera_wrap.cxx',
      # 'deps/miniz/miniz.cpp'
      # 'deps/glob/glob.cpp'
      # 'deps/phonedepth/extract_depthmap.cpp'
      # 'deps/skymodel/ArHosekSkyModel.cpp'
      # 'deps/stb/stb_image_write.cpp'
      # 'deps/stb/stb_image.cpp'
      # 'src/app.cpp',
      # 'src/window.cpp',
      # 'src/gl/defines.cpp',
      # 'src/gl/fbo.cpp',
      # 'src/gl/pingpong.cpp'
      # 'src/gl/pyramid.cpp'
      # 'src/gl/shader.cpp'
      # 'src/gl/texture.cpp'
      # 'src/gl/textureBump.cpp'
      # 'src/gl/textureCube.cpp'
      # 'src/gl/textureProps.cpp'
      # 'src/gl/textureStreamAudio.cpp'
      # 'src/gl/textureStreamAV.cpp'
      # 'src/gl/textureStreamMMAL.cpp'
      # 'src/gl/textureStreamOMX.cpp'
      # 'src/gl/textureStreamSequence.cpp'
      # 'src/gl/vbo.cpp'
      # 'src/gl/vertexLayout.cpp'
      # 'src/types/bvh.cpp',
      # 'src/types/camera.cpp',
      # 'src/types/font.cpp',
      # 'src/types/image.cpp',
      # 'src/types/label.cpp',
      # 'src/types/line.cpp',
      # 'src/types/material.cpp',
      # 'src/types/mesh.cpp',
      # 'src/types/model.cpp',
      'src/types/node.cpp',
      # 'src/types/scene.cpp',
      # 'src/types/triangle.cpp',
      # 'src/ops/draw.cpp',
      # 'src/ops/env.cpp',
      # 'src/ops/fs.cpp',
      # 'src/ops/geom.cpp',
      # 'src/ops/image.cpp',
      # 'src/ops/intersection.cpp',
      # 'src/ops/meshes.cpp',
      # 'src/ops/pixel.cpp',
      # 'src/ops/string.cpp',
      # 'src/ops/time.cpp',
      # 'src/io/obj.cpp',
      # 'src/io/ply.cpp',
      # 'src/io/stl.cpp',
      # 'src/io/gltf.cpp',
      # 'src/shaders/defaultShaders.cpp',
      # 'src/xr/holoPlay.cpp',
      # 'src/xr/xr.cpp',
    ],
    swig_opts = ['-c++']
)

setup(  
    name = 'vera',
    description = doc_lines[0],
    long_description = '\n'.join(doc_lines[2:]),
    version     = '0.1',
    author      = 'Patricio Gonzalez Vivo',
    author_email = 'patriciogonzalezvivo@gmail.com',
    license     = "Prosperity Public License 3.0.0",
    ext_modules = [ vera_module ],
    py_modules = [ 'vera' ],
)
