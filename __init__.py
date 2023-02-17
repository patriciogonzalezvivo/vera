# outer __init__.py

from .hilma import *

import importlib
bpy_found = importlib.util.find_spec("bpy")

def toBlenderImage(_name, _image, 
    float_buffer=True, alpha=True, alpha_mode='STRAIGHT', file_format='PNG'):

    if bpy_found is None:
        raise Exception('hilma.toBlenderImage() requires Blender enviroment')

    import bpy
    from itertools import chain

    if _name in bpy.data.images:
        bimage = bpy.data.images[_name]
    else:
        bimage = bpy.data.images.new(   _name, 
                                        _image.getWidth(), _image.getHeight(), 
                                        float_buffer=float_buffer, alpha=alpha)

    bimage.alpha_mode = alpha_mode
    bimage.file_format = file_format

    def feedGrayscalePixels(img):
        for y in range(img.getHeight()):
            for x in range(img.getWidth()):
                v = img.getValue( img.getIndex(x,y) )
                yield v, v, v, 1.0

    def feedColorPixels(img):
        for y in range(img.getHeight()):
            for x in range(img.getWidth()):
                v = img.getColor( img.getIndex(x,y) )
                yield v[0], v[1], v[2], v[3]
            
    if _image.getChannels() == 1:
        bimage.pixels = tuple(chain.from_iterable(feedGrayscalePixels(_image)))
    else:
        bimage.pixels = tuple(chain.from_iterable(feedColorPixels(_image)))

    bimage.update()


def toBlenderPolyline(_name, _polyline, _collection_name="Collection"):
    if bpy is None:
        raise Exception('hilma.toBlenderPolyline() requires Blender enviroment')

    import bpy

    if _name in bpy.data.curves:
        bcurve = bpy.data.curves[_name]
    else:
        bcurve = bpy.data.curves.new(_name, type="CURVE")
        obj = bpy.data.objects.new(bcurve.name, bcurve)
        col = bpy.data.collections.get(_collection_name)
        col.objects.link(obj)
        bpy.context.view_layer.objects.active = obj
        
    bcurve.dimensions = '3D'
    bcurve.splines.clear()
    polyline = bcurve.splines.new("POLY")
    polyline.points.add( _polyline.size()-1 )
    for i, v in enumerate( _polylines.getVertices()):
        polyline.points[i].co = (v.x, v.y, v.z, 1)
        

def toBlenderPolygon(_name, _polygon, _collection_name="Collection"):
    if bpy_found is None:
        raise Exception('hilma.toBlenderPolygon() requires Blender enviroment')

    import bpy

    if _name in bpy.data.curves:
        bcurve = bpy.data.curves[_name]
    else:
        bcurve = bpy.data.curves.new(_name, type="CURVE")
        obj = bpy.data.objects.new(bcurve.name, bcurve)
        col = bpy.data.collections.get(_collection_name)
        col.objects.link(obj)
        bpy.context.view_layer.objects.active = obj
        
    bcurve.dimensions = '3D'
    bcurve.splines.clear()
    
    for p in range(_polygon.size()):
        polyline = bcurve.splines.new("POLY")
        points = _polygon.get(p).getVertices()
        polyline.points.add( len(points)-1 )
        for i in range( len(points) ):
            polyline.points[i].co = (points[i].x, points[i].y, points[i].z, 1)
    
    
def toBlenderMesh(_name, _mesh, _collection_name="Collection"):   
    if bpy_found is None:
        raise Exception('hilma.toBlendermesh() requires Blender enviroment')

    import bpy

    if _name in bpy.data.meshes:
        bmesh = bpy.data.meshes[_name]
        bmesh.clear_geometry()
    else:
        bmesh = bpy.data.meshes.new(_name)
        obj = bpy.data.objects.new(bmesh.name, bmesh)
        col = bpy.data.collections.get(_collection_name)
        col.objects.link(obj)
        bpy.context.view_layer.objects.active = obj
            
    verts = []
    faces = []
    edges = []
    
    for v in _mesh.getVertices():
        verts.append( (v.x, v.y, v.z) )

    for t in _mesh.getTrianglesIndices():
        faces.append( (t.x, t.y, t.z ) )

    for e in _mesh.getLinesIndices():
        edges.append( (e.x, e.y) )
        
    bmesh.from_pydata(verts, edges, faces)
        
    # Texture coordinates per vertex *per polygon loop*.
    # Create UV coordinate layer and set values
    if _mesh.haveTexCoords():
        uv_layer = bmesh.uv_layers.new()
        for i, uv in enumerate(uv_layer.data):
            st = _mesh.getTexCoord( _mesh.getFaceIndex(i) )
            uv.uv = [st.x, st.y]

    # Vertex color per vertex *per polygon loop*    
    # Create vertex color layer and set values
    if _mesh.haveColors():
        vcol_lay = bmesh.vertex_colors.new()
        for i, col in enumerate(vcol_lay.data):
            color = _mesh.getColor( _mesh.getFaceIndex(i) )
            col.color[0] = color[0]
            col.color[1] = color[1]
            col.color[2] = color[2]
            col.color[3] = color[3]
            
    # We're done setting up the mesh values, update mesh object and 
    # let Blender do some checks on it
    bmesh.update()
    bmesh.validate()