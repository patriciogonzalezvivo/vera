//
//  Polyline.cpp
//
//  Created by Patricio Gonzalez Vivo on 8/20/14.
//
//

#include "vera/types/polyline.h"

#include "vera/types/polarPoint.h"
#include "vera/ops/geom.h"
#include "vera/ops/intersection.h"

namespace vera {

Polyline::Polyline():m_centroid(0.0,0.0,0.0),m_bChange(true){
    m_points.clear();
}

Polyline::Polyline(const Polyline &_poly):m_centroid(0.0,0.0,0.0),m_bChange(true){
    for (int i = 0; i < _poly.size(); i++) {
        add(_poly[i]);
    }
}


Polyline::Polyline(const std::vector<glm::vec2> & _points):m_centroid(0.0,0.0,0.0),m_bChange(true){
    add(_points);
}

Polyline::Polyline(const std::vector<glm::vec3> & _points):m_centroid(0.0,0.0,0.0),m_bChange(true){
    add(_points);
}

//Polyline::~Polyline(){
//    m_points.clear();
//}

int Polyline::size() const { return m_points.size(); }

void Polyline::clear(){
    m_points.clear();
}

void Polyline::add( const glm::vec2 & _point ){
    m_points.push_back(glm::vec3(_point.x, _point.y, 0.0));
    m_bChange = true;
}

void Polyline::add(const std::vector<glm::vec2> & _points){
    for (int i = 0; i < _points.size(); i++) {
        add(_points[i]);
    }
}

void Polyline::add(const glm::vec2* verts, int numverts) {
    for (int i = 0 ; i < numverts ; i++) {
        add(verts[i]);
    }
}

void Polyline::add( const glm::vec3 & _point ){
    m_points.push_back(_point);
    m_bChange = true;
}

void Polyline::add(const std::vector<glm::vec3> & _points){
    for (int i = 0; i < _points.size(); i++) {
        add(_points[i]);
    }
}

void Polyline::add(const glm::vec3* verts, int numverts) {
    for (int i = 0 ; i < numverts ; i++) {
        add(verts[i]);
    }
}

glm::vec3 & Polyline::operator [](const int &_index){
    return m_points[_index];
}

const glm::vec3 & Polyline::operator [](const int &_index) const {
    return m_points[_index];
}

//This is for polygon/contour simplification - we use it to reduce the number of m_points needed in
//representing the letters as openGL shapes - will soon be moved to ofGraphics.cpp

// From: http://softsurfer.com/Archive/algorithm_0205/algorithm_0205.htm
// Copyright 2002, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

typedef struct{
    glm::vec3 P0, P1;
} Segment;

#define norm2(v)   glm::dot(v,v)        // norm2 = squared length of vector
#define norm(v)    sqrt(norm2(v))  // norm = length of vector
#define d2(u,v)    norm2(u-v)      // distance squared = norm2 of difference
#define d(u,v)     norm(u-v)       // distance = norm of difference

//--------------------------------------------------
static void simplifyDP(float tol, glm::vec3* v, int j, int k, int* mk ){
    if (k <= j+1) // there is nothing to simplify
        return;
    
    // check for adequate approximation by segment S from v[j] to v[k]
    int     maxi	= j;          // index of vertex farthest from S
    float   maxd2	= 0;         // distance squared of farthest vertex
    float   tol2	= tol * tol;  // tolerance squared
    Segment S		= {v[j], v[k]};  // segment from v[j] to v[k]
    glm::vec3 u;
    u				= S.P1 - S.P0;   // segment direction vector
    float  cu		= glm::dot(u,u);     // segment length squared
    
    // test each vertex v[i] for max distance from S
    // compute using the Feb 2001 Algorithm's dist_ofPoint_to_Segment()
    // Note: this works in any dimension (2D, 3D, ...)
    glm::vec3  w;
    glm::vec3   Pb;                // base of perpendicular from v[i] to S
    float  b, cw, dv2;        // dv2 = distance v[i] to S squared
    
    for (int i=j+1; i<k; i++){
        // compute distance squared
        w = v[i] - S.P0;
        cw = glm::dot(w,u);
        if ( cw <= 0 ) dv2 = d2(v[i], S.P0);
        else if ( cu <= cw ) dv2 = d2(v[i], S.P1);
        else {
            b = (float)(cw / cu);
            Pb = S.P0 + u*b;
            dv2 = d2(v[i], Pb);
        }
        // test with current max distance squared
        if (dv2 <= maxd2) continue;
        
        // v[i] is a new max vertex
        maxi = i;
        maxd2 = dv2;
    }
    if (maxd2 > tol2)        // error is worse than the tolerance
    {
        // split the polyline at the farthest vertex from S
        mk[maxi] = 1;      // mark v[maxi] for the simplified polyline
        // recursively simplify the two subpolylines at v[maxi]
        simplifyDP( tol, v, j, maxi, mk );  // polyline v[j] to v[maxi]
        simplifyDP( tol, v, maxi, k, mk );  // polyline v[maxi] to v[k]
    }
    // else the approximation is OK, so ignore intermediate vertices
    return;
}

void Polyline::simplify2D(float _tolerance) {
    //  convert vec3 to vec2
    std::vector<glm::vec2> points = get2DPoints();
    vera::simplify(points, _tolerance);

    //  convert vec2 back to vec3
    m_points.clear();
    m_points.reserve(points.size());
    for (int i = 0; i < points.size(); i++) {
        m_points.push_back(glm::vec3(points[i].x, points[i].y, 0.0));
    }
    m_bChange = true;
}

const std::vector<glm::vec2> Polyline::get2DPoints() const {
    std::vector<glm::vec2> points;
    for (int i = 0; i < m_points.size(); i++) {
        points.push_back(glm::vec2(m_points[i].x, m_points[i].y));
    }
    return points;
}

const std::vector<glm::vec3>& Polyline::get3DPoints() const {
    return m_points;
}

glm::vec3 Polyline::getPositionAt(const float &_dist) const{
    float distance = 0.0;
    for (int i = 0; i < m_points.size()-1; i++){
        PolarPoint polar(m_points[i],m_points[i+1]);
        if(distance+polar.r <= _dist){
            return  m_points[i] + PolarPoint(polar.a,_dist-distance).getXY();
        }
        distance += polar.r;
	}
    
    return m_points[m_points.size()-1];
}

BoundingBox Polyline::getBoundingBox() const {
	BoundingBox box;
    for (int i = 0; i < m_points.size(); i++){
        box.expand(m_points[i]);
    }
    return box;
}

glm::vec3 Polyline::getCentroid() {
    if(m_bChange){
        m_centroid = vera::getCentroid(m_points);
        m_bChange = false;
    }
    return m_centroid;
}

float Polyline::get2DArea(){
    return vera::getArea( get2DPoints() );
}

std::vector<Polyline> Polyline::splitAt(float _dist){
    std::vector<Polyline> RTA;
    if (size()>0) {
        Polyline buffer;
        
        buffer.add(m_points[0]);
        float distance = 0.0;
        for (int i = 0; i < m_points.size()-1; i++){
            PolarPoint polar(m_points[i],m_points[i+1]);
            if(distance+polar.r <= _dist){
                buffer.add(m_points[i] + PolarPoint(polar.a,_dist-distance).getXY());
                RTA.push_back(buffer);
                buffer.clear();
                buffer.add(m_points[i] + PolarPoint(polar.a,_dist-distance).getXY());
                break;
            }
            buffer.add(m_points[i+1]);
            distance += polar.r;
        }
        RTA.push_back(buffer);
    }
    return RTA;
}

std::vector<Polyline> Polyline::splitAt2DIntersection(const Polyline &_other, float _gap){
    std::vector<Polyline> RTA;
    
    if (size()>0 && _other.size()>0) {
        Polyline buffer;
        
        buffer.add(m_points[0]);
        for (int i = 0; i < m_points.size()-1; i++){
            for (int j = 0; j < _other.size()-1; j++){
                
                glm::vec2 intersection;
                const glm::vec2 A = glm::vec2(m_points[i].x, m_points[i].y);
                const glm::vec2 B = glm::vec2(m_points[i+1].x, m_points[i+1].y);
                const glm::vec2 C = glm::vec2(_other[j].x, _other[j].y);
                const glm::vec2 D = glm::vec2(_other[j+1].x, _other[j+1].y);
                if (vera::intersection(A, B, C, D, intersection)) {
                    
                    PolarPoint polar = PolarPoint(m_points[i],m_points[i+1]);
                    glm::vec2 gap = PolarPoint(polar.a, _gap, m_points[i].z).getXY();
                    
                    buffer.add(intersection - gap);
                    RTA.push_back(buffer);
                    buffer.clear();
                    buffer.add(intersection + gap);
                }
            }
            buffer.add(m_points[i+1]);
        }
        RTA.push_back(buffer);
    }
    return RTA;
}

// http://www.geeksforgeeks.org/how-to-check-if-a-given-point-lies-inside-a-polygon/
//
bool Polyline::isInside2D(float _x, float _y){
	int counter = 0;
	double xinters;
    glm::vec3 p1,p2;
    
	int N = size();
    
	p1 = m_points[0];
	for (int i=1;i<=N;i++) {
		p2 = m_points[i % N];
		if (_y > MIN(p1.y,p2.y)) {
            if (_y <= MAX(p1.y,p2.y)) {
                if (_x <= MAX(p1.x,p2.x)) {
                    if (p1.y != p2.y) {
                        xinters = (_y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
                        if (p1.x == p2.x || _x <= xinters)
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

//  http://geomalgorithms.com/a12-_hull-3.html
//
Polyline Polyline::get2DConvexHull() {
    return Polyline( vera::getConvexHull( get2DPoints() ) );
}

}