#include "vera/ops/intersection.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp"

#include "vera/ops/math.h"
#include "vera/ops/string.h"

#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

#ifndef EPS
#define EPS 1.0E-10
#endif

namespace vera {

#ifdef DEBUG_INTERSECTIONS
static uint64_t numRayBoxTests = 0;
static uint64_t numRayTrianglesTests = 0;
static uint64_t numRayTrianglesIsect = 0;
static uint64_t numLineLineTests = 0;
static uint64_t numLineLineIsect = 0;
#endif

const float infinity = std::numeric_limits<float>::infinity();

// http://www.geeksforgeeks.org/how-to-check-if-a-given-point-lies-inside-a-polygon/
//
bool inside(const std::vector<glm::vec2> _points, const glm::vec2 _v) {
    int counter = 0;
    double xinters;
    glm::vec2 p1,p2;

    size_t N = _points.size();
    p1 = _points[0];
    for (size_t i = 1; i <= N; i++) {
        p2 = _points[i % N];
        if (_v.y > std::min(p1.y, p2.y)) {
            if (_v.y <= std::max(p1.y, p2.y)) {
                if (_v.x <= std::max(p1.x, p2.x)) {
                    if (p1.y != p2.y) {
                        xinters = (_v.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;
                        if (p1.x == p2.x || _v.x <= xinters)
                            counter++;
                    }
                }
            }
        }
        p1 = p2;
    }

    if (counter % 2 == 0) return false;
    else return true;
}

bool intersection(  const glm::vec2 &_line1Start, const glm::vec2 &_line1End,
                    const glm::vec2 &_line2Start, const glm::vec2 &_line2End,
                    glm::vec2 &_intersection ) {
    glm::vec2 diffLA, diffLB;
    double compareA, compareB;
    diffLA = _line1End - _line1Start;
    diffLB = _line2End - _line2Start;
    compareA = diffLA.x*_line1Start.y - diffLA.y*_line1Start.x;
    compareB = diffLB.x*_line2Start.y - diffLB.y*_line2Start.x;
    if (
        (
         ( ( diffLA.x*_line2Start.y - diffLA.y*_line2Start.x ) < compareA ) ^
         ( ( diffLA.x*_line2End.y - diffLA.y*_line2End.x ) < compareA )
         )
        &&
        (
         ( ( diffLB.x*_line1Start.y - diffLB.y*_line1Start.x ) < compareB ) ^
         ( ( diffLB.x*_line1End.y - diffLB.y*_line1End.x) < compareB )
         )
        )
    {
        double lDetDivInv = 1 / ((diffLA.x*diffLB.y) - (diffLA.y*diffLB.x));
        _intersection.x =  -((diffLA.x*compareB) - (compareA*diffLB.x)) * lDetDivInv ;
        _intersection.y =  -((diffLA.y*compareB) - (compareA*diffLB.y)) * lDetDivInv ;
        
        return true;
    }
    
    return false;
};



/************** Ray ************/
IntersectionData intersection(const Ray& _ray, const Plane& _plane) {
    IntersectionData idata;
    
    float u = 0.0f;
    float denom = glm::dot(_plane.getNormal(), _ray.getDirection());
    
    // check if _ray is paralles to _plane:
    idata.hit = fabs(denom) > EPS;
    if (idata.hit) {
        u = glm::dot(_plane.getNormal(), _plane.getOrigin() - _ray.getOrigin()) / denom;
        idata.position = _ray.getOrigin() + _ray.getDirection() * u;
    }

    return idata;
}

// By Andrew Kensler
//
bool intersection(const Ray& _ray, const BoundingBox& _bbox, float &_tmin, float &_tmax) {
    #ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numRayBoxTests, 1); 
    #endif

    for (int a = 0; a < 3; a++) {
        float invD = _ray.getInvertDirection()[a];
        float t0 = (_bbox.min[a] - _ray.getOrigin()[a]) * invD;
        float t1 = (_bbox.max[a] - _ray.getOrigin()[a]) * invD;
        if (invD < 0.0f)
            std::swap(t0, t1);
        _tmin = t0 > _tmin ? t0 : _tmin;
        _tmax = t1 < _tmax ? t1 : _tmax;
        if (_tmax <= _tmin)
            return false;
    }

    return true;
}

IntersectionData intersection(const Ray& _ray, const BoundingBox& _bbox) {
    IntersectionData idata;

    float tmin, tmax;
    idata.hit = intersection(_ray, _bbox, tmin, tmax);

    if (!idata.hit) return idata;

    idata.distance = tmin;
    idata.position = _ray.getAt(tmin);

    return idata;
}

// MOLLER_TRUMBORE
// #define CULLING
bool intersection(const Ray& _ray, const Triangle& _triangle, float& _t, float& _u, float& _v) {
#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numRayTrianglesTests, 1); 
#endif
    glm::vec3 v0v1 = _triangle[1] - _triangle[0]; 
    glm::vec3 v0v2 = _triangle[2] - _triangle[0]; 
    glm::vec3 pvec = glm::cross(_ray.getDirection(), v0v2); 
    float det = glm::dot(v0v1, pvec); 

#ifdef CULLING 
    // if the determinant is negative the triangle is backfacing
    // if the determinant is close to 0, the ray misses the triangle
    if (det < EPS) return false; 
#else 
    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < EPS) return false; 
#endif 

    float invDet = 1.0f / det; 
 
    glm::vec3 tvec = _ray.getOrigin() - _triangle[0]; 
    _u = glm::dot(tvec, pvec) * invDet; 
    if (_u < 0.0f || _u > 1.0f) return false; 
 
    glm::vec3 qvec = glm::cross(tvec, v0v1); 
    _v = glm::dot(_ray.getDirection(), qvec) * invDet; 
    if (_v < 0.0f || _u + _v > 1.0f) return false; 
 
    _t = glm::dot(v0v2, qvec) * invDet; 
 
#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numRayTrianglesIsect, 1); 
#endif

    return true; // this ray hits the triangle 
} 

#ifdef DEBUG_INTERSECTIONS
uint64_t getTotalRayBoundingBoxTests() {
    return numRayBoxTests;
}
uint64_t getTotalRayTriangleTests() {
    return numRayTrianglesTests;
}

uint64_t getTotalRayTrianglesIntersections() {
    return numRayTrianglesIsect;
}

uint64_t getTotalLineLineTests() {
    return numLineLineTests;
}
uint64_t getTotalLineLineIntersections() {
    return numLineLineIsect;
}
#endif

IntersectionData intersection(const Ray& _ray, const Triangle& _triangle) {

    IntersectionData idata;

    float t,u,v;
    idata.hit = intersection(_ray, _triangle, t, u, v);
    // idata.hit = intersectionMT(_ray, _triangle, t, u, v);

    if (!idata.hit) return idata;

    // float vn = glm::dot(_ray.getDirection(), _triangle.getNormal());
    // glm::vec3 aa = _ray.getOrigin() - _triangle[0];
    // float xpn = glm::dot(aa, _triangle.getNormal());
    // idata.distance = -xpn / vn;
    idata.distance = t;
    idata.position = _ray.getAt(t);
    return idata;

}

float distance(const glm::vec3& _point, const Plane& _plane) {
    return glm::dot(_plane.getNormal(), _point - _plane.getOrigin());
}

// /************** Line  ************/

IntersectionData intersection(const Line& _line, const Plane& _plane) {
    IntersectionData idata;
    
    float dist1 = distance(_line[0], _plane);
    float dist2 = distance(_line[1], _plane);
    int pos1 = sign(dist1);
    int pos2 = sign(dist2);
    
    if (pos1==pos2) {
        idata.hit = false;
        return idata;
    }
    
    float u = 0.0;
    float denom = glm::dot(_plane.getNormal(),_line.getDireciton());
    
    // check if ray is paralles to plane:
    
    if (fabs(denom) > EPS) {
        u= glm::dot(_plane.getNormal(),_plane.getOrigin()-_line[0])/denom;
        // check if intersection is within line-segment:
        if (u>1.0 || u<0) {
            idata.hit = false;
            return idata;
        }
        idata.hit=true;
        idata.position = _line[0]+_line.getDireciton()*u;
    }
    else
        idata.hit = false;
    

    return idata;
}

// http://mathworld.wolfram.com/Line-LineIntersection.html
// in 3d; will also work in 2d if z components are 0

bool intersection(const Line& _A, const Line& _B, glm::vec3& _p) {
#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numLineLineTests, 1); 
#endif

    // Unnormalized direction of the ray
    glm::vec3 da = _A.getDireciton(); 
    glm::vec3 db = _B.getDireciton();
    glm::vec3 dc = _B[0] - _A[0];
    glm::vec3 cross_da_db = glm::cross(da, db);

    // lines are not coplanar
    if (glm::dot(dc,  cross_da_db) != 0.0)
        return false;

    float s = glm::dot( glm::cross(dc,db),  cross_da_db) / glm::length2(  cross_da_db );
    if (s >= 0.0 && s <= 1.0) {
        _p = _A[0] + da * s;

#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numLineLineIsect, 1); 
#endif
        return true;
    }

    return false;
}

const double lengthErrorThreshold = 1e-3;
bool intersection(const Line& _A, const Line& _B, glm::vec3& _p, float _threshold ) {

#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numLineLineTests, 1); 
#endif

    // Unnormalized direction of the ray
    glm::vec3 da = _A.getDireciton();
    glm::vec3 db = _B.getDireciton();
    glm::vec3 dc = _B[0] - _A[0];
    glm::vec3 cross_da_db = glm::cross(da, db);
    
    // lines are not coplanar
    if ( std::abs( glm::dot(dc, cross_da_db)) > _threshold)
        return false;
        
    // Means we have an intersection
    float s = glm::dot( glm::cross(dc, db),  cross_da_db) / glm::length2( cross_da_db );
    if (s >= 0.0 && s <= 1.0) {
        _p = _A[0] + da * s;
        
        // See if this lies on the segment
        if ( (glm::length2(_p - _B[0]) + glm::length2(_p - _B[1])) <= (glm::length2(_B.getDireciton()) + lengthErrorThreshold) ) 
        {

#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numLineLineIsect, 1); 
#endif
            return true;
        }
    }

    return false;
}

IntersectionData intersection(const Line& _line1, const Line& _line2) {

#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numLineLineTests, 1); 
#endif
    IntersectionData idata;
    
    glm::vec3 p13,p43,p21;
    
    glm::vec3 p1 = _line1[0];
    glm::vec3 p2 = _line1[1];
    glm::vec3 p3 = _line2[0];
    glm::vec3 p4 = _line2[1];
    
    
    double d1343, d4321, d1321, d4343, d2121;
    double numer, denom;
    double mua, mub;
    glm::vec3 pa, pb;
    
    p13.x = p1.x - p3.x;
    p13.y = p1.y - p3.y;
    p13.z = p1.z - p3.z;
    p43.x = p4.x - p3.x;
    p43.y = p4.y - p3.y;
    p43.z = p4.z - p3.z;
    if (ABS(p43.x) < EPS && ABS(p43.y) < EPS && ABS(p43.z) < EPS) {
        idata.hit = false;
        return idata;
    }
    
    p21.x = p2.x - p1.x;
    p21.y = p2.y - p1.y;
    p21.z = p2.z - p1.z;
    
    if (ABS(p21.x) < EPS && ABS(p21.y) < EPS && ABS(p21.z) < EPS) {
        idata.hit = false;
        return idata;
    }
    
    d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
    d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
    d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
    d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
    d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;
    
    denom = d2121 * d4343 - d4321 * d4321;
    if (ABS(denom) < EPS) {
        idata.hit = false;
        return idata;
    }
    
    numer = d1343 * d4321 - d1321 * d4343;
    mua = numer / denom;
    mub = (d1343 + d4321 * (mua)) / d4343;
    
    pa.x = p1.x + mua * p21.x;
    pa.y = p1.y + mua * p21.y;
    pa.z = p1.z + mua * p21.z;
    pb.x = p3.x + mub * p43.x;
    pb.y = p3.y + mub * p43.y;
    pb.z = p3.z + mub * p43.z;
    
    idata.hit = true;
    idata.position = pa;
    idata.direction = pb - pa;


#ifdef DEBUG_INTERSECTIONS
    __sync_fetch_and_add(&numLineLineIsect, 1); 
#endif
    return idata;
}

float distance(const glm::vec3& _point, const Line& _line, glm::vec3& _closes_point) {
   glm::vec3 dx = _line.getDireciton();
   float m2 = _line.getMagintude2();

   // find parameter value of closest point on segment
   float s12 = glm::dot(_line[1]-_point, dx) / m2;

   if (s12 < 0)
      s12 = 0;
   else if(s12 > 1)
      s12 = 1;
   
   // and find the distance
   _closes_point = s12 * _line[0] + (1.0f - s12) * _line[1];
   return glm::distance(_point, s12 * _line[0] + (1.0f - s12) * _line[1]);
}

IntersectionData intersection(const glm::vec3& _point, const Line& _line) {
    float u;
    glm::vec3 dist;
    IntersectionData idata;
    
    u = ( ( ( _point.x - _line[0].x ) * ( _line.getDireciton().x ) +
        ( ( _point.y - _line[0].y ) * ( _line.getDireciton().y) ) +
        ( ( _point.z - _line[0].z ) * ( _line.getDireciton().z) ) ) /
        ( glm::length2( _line.getDireciton() ) ) );

    if ( u < 0.0f || u > 1.0f ) {
        idata.hit = false;
        return idata;
    }
    
    idata.hit = true;
    idata.position = _line[0] + u * _line.getDireciton();
    idata.direction = idata.position - _point;
    
    return idata;
}


// /************** Plane  ************/

IntersectionData intersection(const Plane& _plane1, const Plane& _plane2) {
    IntersectionData idata;
    
    glm::vec3 n1 = _plane1.getNormal();
    glm::vec3 n2 = _plane2.getNormal();
    float d1 = _plane1.getDCoeff();
    float d2 = _plane2.getDCoeff();
    
    // Check if planes are parallel, if so return false:
    glm::vec3 dir= glm::cross(_plane1.getNormal(),_plane2.getNormal());
    
    idata.hit = dir.length() < EPS;

    if (idata.hit) {
        // Direction of intersection is the cross product of the two normals:
        dir = glm::normalize(dir);
        
        // Thank you Toxi!
        float offDiagonal = glm::dot(n1,n2);
        float det = 1.0 / (1 - offDiagonal * offDiagonal);
        float a = (d1 - d2 * offDiagonal) * det;
        float b = (d2 - d1 * offDiagonal) * det;
        glm::vec3 anchor = glm::normalize(n1) * a + glm::normalize(n2) * b;
        
        idata.position = anchor;
        idata.direction = dir;
    }
    
    return idata;
}


glm::vec3 LinePlaneIntersectionFast(const glm::vec3& _p0, const glm::vec3& _p1, const Plane& _plane) {
    glm::vec3 vec(_p1-_p0);
    float denom = glm::dot(_plane.getNormal(),vec);
    float u = glm::dot(_plane.getNormal(),_plane.getOrigin()-_p0)/denom;
    return _p0 + vec * u;
}

bool containsValue(std::vector<glm::vec3>* _points, const glm::vec3& _point) {
    for (int i = 0; i < _points->size(); i++)
        if ((_points->at(i)-_point).length()<EPS )
            return true;   
    return false;
}

IntersectionData intersection(const Plane& _plane, const Triangle& _triangle) {
    
    IntersectionData idata;
    glm::vec3 tp0 = _triangle[0];
    glm::vec3 tp1 = _triangle[1];
    glm::vec3 tp2 = _triangle[2];
    
    float dist1 = distance(tp0, _plane);
    float dist2 = distance(tp1, _plane);
    float dist3 = distance(tp2, _plane);
    
    int pos1 = sign(dist1);
    int pos2 = sign(dist2);
    int pos3 = sign(dist3);
    
    if (pos1==pos2 && pos1==pos3) {
        idata.hit = false;
        return idata;
    };
    
    std::vector<glm::vec3>ispoints;
    bool bintersects=false;
    glm::vec3 ip;
    
    if (pos1!=pos2) {
        ip = LinePlaneIntersectionFast(tp0, tp1, _plane);
        if (!containsValue(&ispoints, ip)) {
            ispoints.push_back(ip);
        };
    }
    if (pos2!=pos3) {
        ip = LinePlaneIntersectionFast(tp1, tp2, _plane);
        if (!containsValue(&ispoints, ip)) {
            ispoints.push_back(ip);
        };
    }
    if (pos3!=pos1) {
        ip = LinePlaneIntersectionFast(tp2, tp0, _plane);
        if (!containsValue(&ispoints, ip)) {
            ispoints.push_back(ip);
        };
    }
    
    idata.hit = true;
    idata.position = ispoints.at(0);
    
    if ( ispoints.size() == 2) {
        idata.direction = ispoints.at(1);
        idata.direction -= idata.position;
        idata.distance = idata.direction.length();
    }
    
    return idata;
}

float distance(const glm::vec3& _point, const Triangle& _tri, glm::vec3& _closesPoint) {

    // first find barycentric coordinates of closest point on infinite plane
    glm::vec3 A(_tri[0] - _tri[2]);
    glm::vec3 B(_tri[1] - _tri[2]);
    glm::vec3 C(_point - _tri[2]);

    float m13 = glm::dot(A, A);
    float m23 = glm::dot(B, B);
    float d = glm::dot(A, B);
    float invdet = 1.f/std::fmax(m13 * m23 - d*d, 1e-30f);
    float a = glm::dot(A, C);
    float b = glm::dot(B, C);
    // the barycentric coordinates themselves
    float w23 = invdet * (m23 * a - d * b);
    float w31 = invdet * (m13 * b - d * a);
    float w12 = 1.0f - w23 - w31;

    if ( w23 >= 0.0f && w31 >= 0.0f && w12 >= 0.0f){ // if we're inside the triangle
        _closesPoint = w23 * _tri[0] + w31 * _tri[1] + w12 * _tri[2];
        return glm::distance(_point, w23 * _tri[0] + w31 * _tri[1] + w12 * _tri[2]);
    } 
    else { // we have to clamp to one of the edges

        Line L0 = Line(_tri[0], _tri[1]);
        Line L1 = Line(_tri[0], _tri[2]);
        Line L2 = Line(_tri[1], _tri[2]);

        glm::vec3 r1(0);
        glm::vec3 r2(0);

        if (w23 > 0.0f) { // this rules out edge 2-3 for us
            //return min(distance(_point,_tri[0],_tri[1]), distance(_point,_tri[0], _tri[2],r));
            float d1 = distance(_point, L0, r1);
            float d2 = distance(_point, L1, r2);

            if (d1 < d2) {
                _closesPoint = r1;
                return d1;
            }
            else {
                _closesPoint = r2;
                return d2;
            }
        } 
        else if(w31 > 0.0f) { // this rules out edge 1-3
            //return min(distance(_point,_tri[0],_tri[1]), distance(_point,_tri[1], _tri[2]));
            float d1 = distance(_point, L0, r1);
            float d2 = distance(_point, L2, r2);

            if (d1 < d2) {
                _closesPoint = r1;
                return d1;
            }
            else{
                _closesPoint = r2;
                return d2;
            }
        }
        else { // w12 must be >0, ruling out edge 1-2
            //return min(distance(_point,_tri[0], _tri[2]), distance(_point,_tri[1], _tri[2]));
            float d1 = distance(_point, L1, r1);
            float d2 = distance(_point, L2, r2);

            if (d1 < d2) {
                _closesPoint = r1;
                return d1;
            }
            else{
                _closesPoint = r2;
                return d2;
            }
        }
    }
}

}