#pragma once

#include "vera/types/boundingBox.h"
#include "vera/types/triangle.h"
#include "vera/types/ray.h"

#include <memory>

namespace vera {

enum BVH_Split{
    SPLIT_ARRAY = 0,
    SPLIT_PLANE
};

class BVH : public BoundingBox {
public:
    BVH();
    BVH( const std::vector<Triangle>& _elements, BVH_Split _strategy = SPLIT_ARRAY );
    virtual ~BVH();

    virtual void            load( const std::vector<Triangle>& _elements, BVH_Split _strategy = SPLIT_ARRAY);
    
    virtual std::shared_ptr<BVH> hit(const Ray& _ray, float& _minDistance, float& _maxDistance);
    virtual std::shared_ptr<BVH> hit(const glm::vec3& _point);
    virtual void            hit(const glm::vec3& _point, std::vector<Triangle>& _results) const ;
    virtual void            hit(const glm::vec3& _point, float _r2, std::vector<Triangle>& _results) const;
    virtual void            hit(const BoundingBox& _bbox, std::vector<Triangle>& _results) const ;

    virtual float           minDistance(const glm::vec3& _point) const ;
    virtual float           minSignedDistance(const glm::vec3& _point) const ;
    virtual void            closestTriangles(const glm::vec3& _point, std::vector<Triangle>& _results) const;

    virtual void            clear();

    std::vector<Triangle>   elements;

    std::shared_ptr<BVH>    left;
    std::shared_ptr<BVH>    right;

    size_t                  axis;
    bool                    leaf;

protected:
    virtual void            _splitArray();
    virtual void            _splitPlane();
};

}