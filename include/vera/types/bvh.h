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
    SPLIT_BALANCED_MIDPOINT,
    SPLIT_SAH
};

class BVH : public BoundingBox {
public:
    BVH();
    BVH( const std::vector<Triangle>& _elements, BVH_Split _strategy = SPLIT_BALANCED );
    virtual ~BVH();

    virtual void            load( const std::vector<Triangle>& _elements, BVH_Split _strategy = SPLIT_BALANCED);
    
    virtual std::shared_ptr<BVH> hit(const Ray& _ray, float& _minDistance, float& _maxDistance);

    virtual float           getCost() { return float(elements.size()) * getArea(); }

    virtual glm::vec3       getClosestPointOnTriangle(const glm::vec3& _point) const;
    virtual float           getClosestDistance(const glm::vec3& _point) const;
    virtual float           getClosestSignedDistance(const glm::vec3& _point, float _refinement = 0.0f) const;
    virtual glm::vec4       getClosestRGBSignedDistance(const glm::vec3& _point, float _refinement = 0.0f) const;

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
    virtual void            _split_balanced_midpoint();
    virtual void            _split_sah();
};

}