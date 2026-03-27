// earcut.hpp - Minimal polygon ear-clipping triangulation with hole support
// Suitable for 2D polygon tessellation (e.g. TrueType font glyph outlines)
//
// Usage:
//   std::vector<uint32_t> indices = vera::earcut::triangulate(pts, outerSize, holeSizes);
//
// Where:
//   pts        - flat array of all polygon points (outer contour first, then holes)
//   outerSize  - number of points in the outer contour
//   holeSizes  - sizes of each hole contour in order
//
// Returns indices into pts forming triangles (3 indices per triangle, CCW winding in Y-up).
//
// Winding convention:
//   Outer contour should be CCW when viewed in standard Y-up coordinates.
//   Hole contours should be CW. The function auto-detects and normalizes winding.

#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <limits>
#include "glm/glm.hpp"

namespace vera {
namespace earcut {

using Point = glm::vec2;

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

// Signed area using the shoelace formula (positive = CCW in Y-up)
inline float signedArea(const std::vector<Point>& pts, const std::vector<int>& ring) {
    float area = 0.0f;
    int n = (int)ring.size();
    for (int i = 0, j = n - 1; i < n; j = i++) {
        area += (pts[ring[j]].x + pts[ring[i]].x) * (pts[ring[j]].y - pts[ring[i]].y);
    }
    return area * 0.5f;
}

// 2D cross product of vectors (A→B) × (A→C) — positive means CCW turn in Y-up
inline float cross2D(const Point& A, const Point& B, const Point& C) {
    return (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
}

// Is point P strictly inside triangle A-B-C? (using barycentric / cross product signs)
inline bool insideTriangle(const Point& A, const Point& B, const Point& C, const Point& P) {
    float d1 = cross2D(A, B, P);
    float d2 = cross2D(B, C, P);
    float d3 = cross2D(C, A, P);
    bool has_neg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
    bool has_pos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
    return !(has_neg && has_pos); // all same sign → inside or on edge
}

// ---------------------------------------------------------------------------
// Ear detection (O(n) per vertex, O(n²) overall — fine for font glyphs)
// ---------------------------------------------------------------------------

// Check whether ring[idx] is an ear in the polygon defined by (pts, ring).
// The ring is expected to be CCW (positive area) in Y-up coords.
inline bool isEar(const std::vector<Point>& pts, const std::vector<int>& ring, int idx) {
    int n = (int)ring.size();
    int prev = (idx + n - 1) % n;
    int next = (idx + 1) % n;

    const Point& A = pts[ring[prev]];
    const Point& B = pts[ring[idx]];
    const Point& C = pts[ring[next]];

    // Triangle A-B-C must be CCW (positive cross) to be a valid ear
    if (cross2D(A, B, C) <= 0.0f) return false;

    // No other polygon vertex should lie strictly inside triangle A-B-C
    for (int i = 0; i < n; i++) {
        if (i == prev || i == idx || i == next) continue;
        if (insideTriangle(A, B, C, pts[ring[i]])) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Ear-clipping on a simple (possibly non-convex) polygon ring
// Appends triangle indices to 'out'
// ---------------------------------------------------------------------------
inline void clipEars(const std::vector<Point>& pts, std::vector<int> ring,
                     std::vector<uint32_t>& out) {
    int n = (int)ring.size();
    if (n < 3) return;

    if (n == 3) {
        out.push_back((uint32_t)ring[0]);
        out.push_back((uint32_t)ring[1]);
        out.push_back((uint32_t)ring[2]);
        return;
    }

    // Ensure CCW winding (positive area) before clipping
    if (signedArea(pts, ring) < 0.0f) {
        std::reverse(ring.begin(), ring.end());
    }

    int i = 0;
    int maxIters = n * n; // upper bound to prevent infinite loop on degenerate input
    int iters = 0;

    while (n > 3 && iters < maxIters) {
        if (isEar(pts, ring, i)) {
            int prev = (i + n - 1) % n;
            int next = (i + 1) % n;
            out.push_back((uint32_t)ring[prev]);
            out.push_back((uint32_t)ring[i]);
            out.push_back((uint32_t)ring[next]);
            ring.erase(ring.begin() + i);
            --n;
            i = (i > 0) ? i - 1 : 0;
            iters = 0;
            maxIters = n * n;
        } else {
            i = (i + 1) % n;
            ++iters;
        }
    }

    // Final triangle
    if (n == 3) {
        out.push_back((uint32_t)ring[0]);
        out.push_back((uint32_t)ring[1]);
        out.push_back((uint32_t)ring[2]);
    }
}

// ---------------------------------------------------------------------------
// Hole elimination: merge a hole ring into the outer ring using bridge edges
// Returns the new merged ring (larger ring with the hole "cut in").
// ---------------------------------------------------------------------------
inline std::vector<int> eliminateHole(const std::vector<Point>& pts,
                                       const std::vector<int>& outerRing,
                                       const std::vector<int>& holeRing) {
    if (holeRing.empty()) return outerRing;

    // 1. Find the rightmost vertex of the hole
    int holeRightIdx = 0;
    for (int i = 1; i < (int)holeRing.size(); i++) {
        if (pts[holeRing[i]].x > pts[holeRing[holeRightIdx]].x) {
            holeRightIdx = i;
        }
    }
    const Point hv = pts[holeRing[holeRightIdx]];

    // 2. Cast horizontal ray to the right from hv; find the nearest outer edge crossing
    int bridgeIdx = -1;       // index in outerRing
    float bestIsectX = std::numeric_limits<float>::max();

    int on = (int)outerRing.size();
    for (int i = 0, j = on - 1; i < on; j = i++) {
        const Point& va = pts[outerRing[j]];
        const Point& vb = pts[outerRing[i]];

        // Does edge va→vb straddle the horizontal ray from hv going right?
        if ((va.y <= hv.y && vb.y > hv.y) || (vb.y <= hv.y && va.y > hv.y)) {
            float t = (hv.y - va.y) / (vb.y - va.y);
            float ix = va.x + t * (vb.x - va.x);
            if (ix >= hv.x && ix < bestIsectX) {
                bestIsectX = ix;
                // Choose the outer vertex with the larger x (closer to intersection)
                bridgeIdx = (pts[outerRing[j]].x > pts[outerRing[i]].x) ? j : i;
            }
        }
    }

    if (bridgeIdx < 0) {
        // Fallback: just pick the nearest outer vertex
        float bestDist = std::numeric_limits<float>::max();
        for (int i = 0; i < on; i++) {
            float dx = pts[outerRing[i]].x - hv.x;
            float dy = pts[outerRing[i]].y - hv.y;
            float d = dx * dx + dy * dy;
            if (d < bestDist) { bestDist = d; bridgeIdx = i; }
        }
    }

    // 3. Build merged ring:
    //    outer[0..bridge] → hole[rightmost..end] → hole[0..rightmost] → outer[bridge..end]
    std::vector<int> merged;
    merged.reserve(outerRing.size() + holeRing.size() + 2);

    for (int i = 0; i <= bridgeIdx; i++) merged.push_back(outerRing[i]);

    int hs = (int)holeRing.size();
    for (int i = 0; i <= hs; i++) merged.push_back(holeRing[(holeRightIdx + i) % hs]);

    for (int i = bridgeIdx; i < on; i++) merged.push_back(outerRing[i]);

    return merged;
}

// ---------------------------------------------------------------------------
// Main entry point
// ---------------------------------------------------------------------------

// Triangulate a polygon described by:
//   pts        - all vertices: outer contour (indices 0..outerSize-1) followed by holes
//   outerSize  - number of outer contour vertices
//   holeSizes  - list of hole vertex counts (each consecutive block in pts)
//
// Returns flat index array into pts (3 indices per triangle).
// Winding in output matches the input coordinate system.
inline std::vector<uint32_t> triangulate(const std::vector<Point>& pts,
                                          int outerSize,
                                          const std::vector<int>& holeSizes = {}) {
    std::vector<uint32_t> out;
    if (outerSize < 3) return out;

    // Build outer ring as index array
    std::vector<int> ring(outerSize);
    for (int i = 0; i < outerSize; i++) ring[i] = i;

    // Merge holes (process largest-x hole first for better bridges)
    struct HoleEntry {
        std::vector<int> indices;
        float maxX;
    };
    std::vector<HoleEntry> holes;
    int offset = outerSize;
    for (int hs : holeSizes) {
        if (hs < 3) { offset += hs; continue; }
        HoleEntry he;
        he.indices.resize(hs);
        he.maxX = -std::numeric_limits<float>::max();
        for (int i = 0; i < hs; i++) {
            he.indices[i] = offset + i;
            he.maxX = std::max(he.maxX, pts[offset + i].x);
        }
        holes.push_back(std::move(he));
        offset += hs;
    }

    std::sort(holes.begin(), holes.end(), [](const HoleEntry& a, const HoleEntry& b) {
        return a.maxX > b.maxX; // largest x first
    });

    for (auto& he : holes) {
        ring = eliminateHole(pts, ring, he.indices);
    }

    clipEars(pts, ring, out);
    return out;
}

} // namespace earcut
} // namespace vera
