#pragma once

#include "vera/types/boundingBox.h"
#include "vera/types/triangle.h"
#include "vera/types/ray.h"

#include <memory>

namespace vera {

enum BVH_Split{
    SPLIT_BALANCED = 0,
    SPLIT_MIDPOINT,
    SPLIT_SORTED_MIDPOINT,
    SPLIT_SAH
};

class BVH : public BoundingBox {
public:
    BVH();
    BVH( const std::vector<Triangle>& _elements, BVH_Split _strategy = SPLIT_BALANCED );
    virtual ~BVH();

    virtual void            load( const std::vector<Triangle>& _elements, BVH_Split _strategy = SPLIT_BALANCED);
    
    virtual std::shared_ptr<BVH> hit(const Ray& _ray, float& _minDistance, float& _maxDistance);
    virtual std::shared_ptr<BVH> hit(const glm::vec3& _point);
    virtual void            hit(const glm::vec3& _point, std::vector<Triangle>& _results) const ;
    virtual void            hit(const glm::vec3& _point, float _r2, std::vector<Triangle>& _results) const;
    virtual void            hit(const BoundingBox& _bbox, std::vector<Triangle>& _results) const ;

    virtual float           getCost() { return float(elements.size()) * getArea(); }
    virtual float           getMinDistance(const glm::vec3& _point) const ;
    virtual float           getMinSignedDistance(const glm::vec3& _point) const ;

    virtual void            clear();

    std::vector<Triangle>   elements;

    std::shared_ptr<BVH>    left;
    std::shared_ptr<BVH>    right;

    size_t                  axis;
    bool                    leaf;

protected:
    virtual void            _split_balanced();
    virtual void            _split_midpoint();
    virtual void            _split_sorted_midpoint();
    virtual void            _split_sah();
};

}