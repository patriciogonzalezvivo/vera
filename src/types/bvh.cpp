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

    m_average_normal = glm::vec3(0.0);
    
    // Exapand bounds to contain all elements
    for (size_t i = 0; i < elements.size(); i++ ) {
        expand(elements[i]);
        // m_average_normal += elements[i].getNormal();
    }

    // m_average_normal /= (float)elements.size();
    // m_average_normal = glm::normalize(m_average_normal);

    // // Exapand a bit for padding
    // glm::vec3   bdiagonal = getDiagonal();
    // float max_dist = glm::length(bdiagonal);
    // expand(max_dist * 0.01f);

    leaf = elements.size() == 1;

    if (!leaf)
        _split();
}

// Returns element closest to target in arr[]
size_t findClosest(const std::vector<Triangle>& _list, size_t _axis, size_t _size, float _target) {
    // left-side case
    if (_target <= _list[0].getCentroid()[_axis] )
        return 0;
    //right-side case
    if (_target >= _list[_size - 1].getCentroid()[_axis])
        return _size - 1;

    // binary search
    size_t i = 0, j = _size, mid = 0;
    while (i < j) {
        mid = (i + j) / 2;
        if (_list[mid].getCentroid()[_axis] == _target)
            return mid;

        /* If target is less than _list element,
            then search in left */
        if (_target < _list[mid].getCentroid()[_axis]) {
            // If target is greater than previous
            // to mid, return closest of two
            if (mid > 0 && _target > _list[mid - 1].getCentroid()[_axis])
                // return getClosest(_list[mid - 1], _list[mid], target);
                return mid;

            j = mid;
        }
        /* Repeat for left half */
        // If target is greater than mid
        else {
            if (mid < _size - 1 && _target < _list[mid + 1].getCentroid()[_axis])
                // return getClosest(  _list[mid], _list[mid + 1], target);
                return mid;

            // update i
            i = mid + 1;
        }
    }

    // Only single element left after search
    return mid;
}

void BVH::_split() {
    float width = getWidth();
    float height = getHeight();
    float depth = getDepth();
    glm::vec3 center = getCenter();

    size_t axis =   (width > std::max(height, depth) ) ? 0
                   :(height > std::max(width, depth) ) ? 1
                   :2;
                    
    auto comparator =   (width > std::max(height, depth) ) ? Triangle::compareX
                        :(height > std::max(width, depth) ) ? Triangle::compareY
                        :  Triangle::compareZ;

    // Sort elements by the longest axis
    std::sort(elements.begin(), elements.end(), comparator);
    
    std::size_t half_array_size = elements.size() / 2;
    // std::size_t half_array_size = findClosest(elements, axis, elements.size(), center[axis]);

    if (half_array_size == 0)
        half_array_size++;

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

void BVH::hit(const glm::vec3& _p, std::vector<Triangle>& _results) const {
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


void BVH::hit(const glm::vec3& _p, float _r2, std::vector<Triangle>& _results ) const {
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


void BVH::hit(const BoundingBox& _bbox, std::vector<Triangle>& _results ) const {
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

float BVH::minDistance(const glm::vec3& _point) const {
    float minDist = 3.0e+038;
    if (leaf) {
        // return distanceToClosest(_point);
        // return glm::length( _point - getCenter() );
        for (size_t i = 0; i < elements.size(); i++) {
            // float d = glm::length( _point - elements[i].closest(_point) );
            float d = elements[i].unsignedDistance(_point);
            if (d < minDist)
                minDist = d;
        }
    }
    else if (right != nullptr && left != nullptr) {
        float left_dist = left->minDistance(_point);
        float right_dist = right->minDistance(_point);
        if (left_dist <= right_dist)
            return left_dist;
        else 
            return right_dist;
    }
    return minDist;
}

float BVH::minSignedDistance(const glm::vec3& _point) const {
    float minDist = 3.0e+038;
    if (leaf) {
        for (size_t i = 0; i < elements.size(); i++) {
            float d = elements[i].signedDistance(_point);
            if (abs(d) < abs(minDist)) {
                minDist = d;
            }
        }
        return minDist;
    }
    else if (right != nullptr && left != nullptr) {
        float left_dist = left->minSignedDistance(_point);
        float right_dist = right->minSignedDistance(_point);
        if (abs(left_dist) <= abs(right_dist))
            return left_dist;
        else 
            return right_dist;
    }
    return minDist;
}

void BVH::closest(const glm::vec3& _p, std::vector<Triangle>& _results) const {
    if (leaf)
        _results.insert(_results.end(), elements.begin(), elements.end());
    
    else if (right != nullptr && left != nullptr) {
        if ( contains(_p) ) {
            right->closest(_p, _results);
            left->closest(_p, _results);
        } 
        else {
            float left_dist  = left->distanceToClosest(_p);
            float right_dist = right->distanceToClosest(_p);

            if (left_dist == right_dist) {
                left_dist  = left->minDistance(_p);
                right_dist = right->minDistance(_p);
            }

            if (left_dist == right_dist)
                _results.insert(_results.end(), elements.begin(), elements.end());
            else if (left_dist < right_dist)
                left->closest(_p, _results);
            else 
                right->closest(_p, _results);
        }
    }
}

}