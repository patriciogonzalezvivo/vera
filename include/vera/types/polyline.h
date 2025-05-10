#include <vector>
#include "glm/glm.hpp"

#include "vera/types/boundingBox.h"

namespace vera {

class Polyline {
public:
    
    Polyline();
    Polyline(const Polyline &_poly);
    Polyline(const std::vector<glm::vec2> &_points);
    Polyline(const std::vector<glm::vec3> &_points);
    // virtual ~Polyline();
    
    virtual void        add(const glm::vec2 &_point);
    virtual void        add(const std::vector<glm::vec2> &_points);
    virtual void        add(const glm::vec2* verts, int numverts);

    virtual void        add(const glm::vec3 &_point);
    void                add(const std::vector<glm::vec3> &_points);
    void                add(const glm::vec3* verts, int numverts);
    
    virtual glm::vec3 & operator [](const int &_index);
    virtual const glm::vec3 & operator [](const int &_index) const;
    
    virtual float               get2DArea();
    virtual glm::vec3           getCentroid();
    virtual glm::vec3           getPositionAt(const float &_dist) const;

    virtual const std::vector<glm::vec2> get2DPoints() const;
    virtual const std::vector<glm::vec3>& get3DPoints() const;

    virtual Polyline    get2DConvexHull();
    vera::BoundingBox   getBoundingBox() const;
    
    bool                isInside2D(float _x, float _y);
    
    virtual int         size() const;

    std::vector<Polyline> splitAt(float _dist);
    std::vector<Polyline> splitAt2DIntersection(const Polyline &_other, float _gap = 1.0);
    
    virtual void        clear();
    virtual void        simplify2D(float _tolerance=0.3f);
    
protected:
    std::vector<glm::vec3>  m_points;
    glm::vec3   m_centroid;
    
    bool        m_bChange;
};

}