#include "vera/types/shape.h"

#include "vera/ops/draw.h"      // vera::line(), vera::model()

#include "earcut.hpp"    // polygon triangulation (in deps/earcut/)

#include <algorithm>

namespace vera {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Signed area via the shoelace formula.  Positive = CCW in standard Y-up.
float polySignedArea(const std::vector<glm::vec2>& pts) {
    float area = 0.0f;
    int n = (int)pts.size();
    for (int i = 0, j = n - 1; i < n; j = i++) {
        area += (pts[j].x + pts[i].x) * (pts[j].y - pts[i].y);
    }
    return area * 0.5f;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Shape::Shape() : m_dirty(true) {}

Shape::Shape(const Polyline& _contour)
    : contour(_contour), m_dirty(true) {}

// ---------------------------------------------------------------------------
// Geometry queries
// ---------------------------------------------------------------------------

float Shape::getArea() const {
    return polySignedArea(contour.get2DPoints());
}

Segment2DList Shape::getSegments2D() const {
    Segment2DList segments;
    auto addSegments = [&segments](const std::vector<glm::vec2>& pts) {
        for (size_t i = 0; i + 1 < pts.size(); i++) {
            segments.emplace_back(pts[i], pts[i + 1]);
        }
    };
    addSegments(contour.get2DPoints());
    for (const auto& hole : holes) {
        addSegments(hole.get2DPoints());
    }
    return segments;
}

Segment3DList Shape::getSegments3D() const {
    Segment3DList segments;
    auto addSegments = [&segments](const std::vector<glm::vec3>& pts) {
        for (size_t i = 0; i + 1 < pts.size(); i++) {
            segments.emplace_back(pts[i], pts[i + 1]);
        }
    };
    addSegments(contour.get3DPoints());
    for (const auto& hole : holes) {
        addSegments(hole.get3DPoints());
    }
    return segments;
}

// ---------------------------------------------------------------------------
// Tessellation
// ---------------------------------------------------------------------------

void Shape::tessellate() const {
    m_mesh.clear();
    m_mesh.setDrawMode(TRIANGLES);

    auto outerPts = contour.get2DPoints();
    if (outerPts.size() < 3) {
        m_dirty = false;
        return;
    }

    // Flat vertex array: outer contour first, then each hole contour
    std::vector<earcut::Point> pts;
    pts.reserve(outerPts.size());
    for (const auto& p : outerPts) pts.push_back(p);

    std::vector<int> holeSizes;
    for (const auto& hole : holes) {
        auto holePts = hole.get2DPoints();
        if ((int)holePts.size() < 3) continue;
        holeSizes.push_back((int)holePts.size());
        for (const auto& p : holePts) pts.push_back(p);
    }

    // Add all points as mesh vertices
    for (const auto& p : pts) {
        m_mesh.addVertex(glm::vec3(p.x, p.y, 0.0f));
    }

    // Triangulate and register indices
    std::vector<uint32_t> indices = earcut::triangulate(pts, (int)outerPts.size(), holeSizes);
    for (int t = 0; t + 2 < (int)indices.size(); t += 3) {
        m_mesh.addTriangleIndices((INDEX_TYPE)indices[t],
                                   (INDEX_TYPE)indices[t + 1],
                                   (INDEX_TYPE)indices[t + 2]);
    }

    m_dirty = false;
}

const Mesh& Shape::getMesh() const {
    if (m_dirty) tessellate();
    return m_mesh;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void Shape::drawFill(Shader* _program) const {
    if (m_dirty) tessellate();
    if (m_mesh.haveVertices()) {
        model(m_mesh, _program);
    }
}

void Shape::drawContour(Shader* _program) const {
    // Draw the outer boundary as a closed loop
    if (contour.size() > 1) {
        auto pts3d = contour.get3DPoints();
        if (!pts3d.empty()) pts3d.push_back(pts3d.front()); // close the loop
        line(pts3d, _program);
    }
    // Draw each hole boundary as a closed loop
    for (const auto& hole : holes) {
        if (hole.size() > 1) {
            auto pts3d = hole.get3DPoints();
            if (!pts3d.empty()) pts3d.push_back(pts3d.front());
            line(pts3d, _program);
        }
    }
}

void Shape::draw(Shader* _program) const {
    drawFill(_program);
    drawContour(_program);
}

} // namespace vera
