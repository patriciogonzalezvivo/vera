#pragma once

#include <vector>

#include "vera/types/polyline.h"
#include "vera/types/mesh.h"
#include "vera/types/boundingBox.h"

namespace vera {

using Segment2D = std::pair<glm::vec2, glm::vec2>;
using Segment2DList = std::vector<Segment2D>;

using Segment3D = std::pair<glm::vec3, glm::vec3>;
using Segment3DList = std::vector<Segment3D>;

class Shader;

/// A 2-D shape defined by one outer contour (boundary) and zero or more inner
/// contours (holes / islands, sometimes called "counter-contours").
///
/// Rendering:
///   drawContour()  — draws each polyline as a line loop (outline only)
///   drawFill()     — tessellates contour + holes into triangles and fills
///   draw()         — draws fill first, then contour on top
///
/// Tessellation is performed lazily and cached; call markDirty() whenever you
/// modify contour or holes manually.
class Shape {
public:

    // -----------------------------------------------------------------------
    // Data
    // -----------------------------------------------------------------------

    Polyline              contour;   ///< Outer boundary (CCW in Y-up convention)
    std::vector<Polyline> holes;     ///< Inner holes / islands (CW in Y-up convention)

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    Shape();
    explicit Shape(const Polyline& _contour);

    // -----------------------------------------------------------------------
    // Geometry queries
    // -----------------------------------------------------------------------

    /// Signed area of the outer contour (positive = CCW in Y-up).
    float       getArea() const;

    Segment2DList getSegments2D() const;
    Segment3DList getSegments3D() const;

    /// True when the outer contour winds clockwise (Y-up convention).
    bool        isClockwise() const { return getArea() < 0.0f; }

    BoundingBox getBoundingBox() const { return contour.getBoundingBox(); }

    // -----------------------------------------------------------------------
    // Tessellation / mesh access
    // -----------------------------------------------------------------------

    /// Returns a TRIANGLES mesh for the filled shape.
    /// The mesh is cached and regenerated only when the shape is dirty.
    const Mesh& getMesh() const;

    /// Mark the cached tessellation as stale (call after editing contour/holes).
    void markDirty() { m_dirty = true; }

    // -----------------------------------------------------------------------
    // Drawing (uses the active vera fill / stroke colours)
    // -----------------------------------------------------------------------

    /// Draw the filled shape (tessellated triangles).
    void drawFill(Shader* _program = nullptr) const;

    /// Draw only the outline (each polyline as a line loop).
    void drawContour(Shader* _program = nullptr) const;

    /// Draw fill first, then contour on top.
    void draw(Shader* _program = nullptr) const;

private:

    mutable Mesh m_mesh;
    mutable bool m_dirty = true;

    void tessellate() const;
};

/// Convenience alias for a list of Shapes (e.g. all glyphs in a string).
using ShapeList = std::vector<Shape>;

} // namespace vera
