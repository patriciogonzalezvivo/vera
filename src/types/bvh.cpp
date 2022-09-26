#include "vera/types/bvh.h"

#include "vera/ops/intersection.h"

#include <algorithm>

namespace vera {

BVH::BVH() : parent(nullptr), left(nullptr), right(nullptr), leaf(false){
}

BVH::BVH( const std::vector<Triangle>& _elements) {
    parent = nullptr;
    left = nullptr;
    right = nullptr;
    leaf = false;

    load(_elements);
}

BVH::~BVH() {
    clear();
}

void BVH::clear() {
    left = nullptr;
    right = nullptr;
    parent = nullptr;
}

void BVH::load( const std::vector<Triangle>& _elements) {
    elements = _elements;

    // Exapand bounds to contain all elements
    for (size_t i = 0; i < elements.size(); i++ )
        expand(elements[i]);

    // Exapand a bit for padding
    glm::vec3   bdiagonal = getDiagonal();
    float max_dist = glm::length(bdiagonal);
    expand(max_dist * 0.01f);

    leaf = elements.size() == 1;

    if (!leaf)
        _split();
}

void BVH::_split() {
    float width = getWidth();
    float height = getHeight();
    float depth = getDepth();

    size_t axis =   (width > std::max(height, depth) ) ? 0
                   :(height > std::max(width, depth) ) ? 1
                   :2;
                    
    auto comparator =   (width > std::max(height, depth) ) ? Triangle::compareX
                        :(height > std::max(width, depth) ) ? Triangle::compareY
                        :  Triangle::compareZ;

    // Sort elements by the longest axis
    std::sort(elements.begin(), elements.end(), comparator);

    std::size_t half_array_size = elements.size() / 2;
    left = std::make_shared<BVH>( std::vector<Triangle>(elements.begin(), elements.begin() + half_array_size) );
    left->parent = std::make_shared<BVH>( *this );

    right = std::make_shared<BVH>( std::vector<Triangle>(elements.begin() + half_array_size, elements.end()) );
    right->parent = std::make_shared<BVH>( *this );
}

std::shared_ptr<BVH> BVH::hit(const Ray& _ray, float& _minDistance, float& _maxDistance) {
    if ( !intersection(_ray, (BoundingBox)*this, _minDistance, _maxDistance) )
        return nullptr;

    if (!leaf) {
        float left_minT = _minDistance;
        float left_maxT = _maxDistance;
        std::shared_ptr<BVH> left_hit = left->hit(_ray, left_minT, left_maxT);

        float right_minT = _minDistance;
        float right_maxT = _maxDistance;
        std::shared_ptr<BVH> right_hit = right->hit(_ray, right_minT, right_maxT);

        if (left_hit != nullptr && right_hit != nullptr) {
            return std::make_shared<BVH>( *this );
        }
        else if (left_hit != nullptr) 
            return left_hit;
        else if (right_hit != nullptr)
            return right_hit;

        return nullptr;
    }
    else
        return std::make_shared<BVH>( *this );
}

std::shared_ptr<BVH> BVH::hit(const glm::vec3& _point) {
    if ( !contains(_point) )
        return nullptr;

    if (!leaf) {
        std::shared_ptr<BVH> left_c = left->hit(_point);
        std::shared_ptr<BVH> right_c = right->hit(_point);

        if (left_c != nullptr && right_c != nullptr) {
            // both hits return it self
            return std::make_shared<BVH>( *this );
        }
        else if (left_c != nullptr) 
            return left_c;
        else if (right_c != nullptr)
            return right_c;

        return nullptr;
    }
    else
        return std::make_shared<BVH>( *this );
}

void BVH::hit(const glm::vec3& _p, std::vector<Triangle>& _results) {
    if (leaf) {
        _results.insert(_results.end(), elements.begin(), elements.end());
    }
    else {
        // If the query bbox intersect one of the childs add them

        if (right != nullptr && right->contains(_p))
            right->hit(_p, _results);

        if (left != nullptr && left->contains(_p))
            left->hit(_p, _results);
    }
}


void BVH::hit(const glm::vec3& _p, float _r2, std::vector<Triangle>& _results ) {
    if (leaf) {
        _results.insert(_results.end(), elements.begin(), elements.end());
    }
    else {
        // If the query bbox intersect one of the childs add them

        if (right != nullptr && right->intersects(_p, _r2))
            right->hit(_p, _r2, _results);

        if (left != nullptr && left->intersects(_p, _r2))
            left->hit(_p, _r2, _results);
    }
}


void BVH::hit(const BoundingBox& _bbox, std::vector<Triangle>& _results ) {
    if (leaf) {
        _results.insert(_results.end(), elements.begin(), elements.end());
    }
    else {
        // If the query bbox intersect one of the childs add them

        if (right != nullptr && right->intersects(_bbox))
            right->hit(_bbox, _results);

        if (left != nullptr && left->intersects(_bbox))
            left->hit(_bbox, _results);
    }
}

}