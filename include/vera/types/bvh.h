#pragma once

#include "vera/types/boundingBox.h"
#include "vera/types/triangle.h"
#include "vera/types/ray.h"

#include <memory>

namespace vera {

class BVH : public BoundingBox {
public:
    BVH();
    BVH( const std::vector<Triangle>& _elements);
    virtual ~BVH();

    virtual void load( const std::vector<Triangle>& _elements);
    
    virtual std::shared_ptr<BVH> hit(const Ray& _ray, float& _minDistance, float& _maxDistance);
    virtual std::shared_ptr<BVH> hit(const glm::vec3& _point);
    virtual void hit(const glm::vec3& _point, std::vector<Triangle>& _results);
    virtual void hit(const glm::vec3& _point, float _r2, std::vector<Triangle>& _results);
    virtual void hit(const BoundingBox& _bbox, std::vector<Triangle>& _results);

    virtual void clear();

    std::vector<Triangle>       elements;

    std::shared_ptr<BVH>        parent;
    std::shared_ptr<BVH>        left;
    std::shared_ptr<BVH>        right;
    bool                        leaf;

protected:
    virtual void _split();

};

}