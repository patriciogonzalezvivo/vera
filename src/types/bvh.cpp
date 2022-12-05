#include "vera/types/bvh.h"

#include "vera/ops/intersection.h"

#include <algorithm>

namespace vera {

BVH::BVH() : left(nullptr), right(nullptr), axis(0), leaf(true) {
}

BVH::BVH( const std::vector<Triangle>& _elements, BVH_Split _strategy) {
    left = nullptr;
    right = nullptr;
    axis = 0;
    leaf = true;

    load(_elements, _strategy);
}

BVH::~BVH() {
    clear();
}

void BVH::clear() {
    left = nullptr;
    right = nullptr;
}

void BVH::load( const std::vector<Triangle>& _elements, BVH_Split _strategy ) {
    elements = _elements;

    // Exapand bounds to contain all elements
    for (size_t i = 0; i < _elements.size(); i++ )
        expand(_elements[i]);

    // // Exapand a bit for padding
    // glm::vec3   bdiagonal = getDiagonal();
    // float max_dist = glm::length(bdiagonal);
    // expand(max_dist * 0.01f);

    leaf = _elements.size() < 2;

    if (!leaf) {
        if (_strategy == SPLIT_BALANCED)
            _split_balanced();
        else if (_strategy == SPLIT_MIDPOINT)
            _split_midpoint();
        else if (_strategy == SPLIT_SORTED_MIDPOINT)
            _split_sorted_midpoint();
        else if (_strategy == SPLIT_SAH)
            _split_sah();
    }
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

void BVH::_split_balanced() {
    float width = getWidth();
    float height = getHeight();
    float depth = getDepth();
    glm::vec3 center = getCenter();

    axis =  (width > std::max(height, depth) ) ?  0
            : (height > std::max(width, depth) ) ?  1
            : 2;
                    
    auto comparator =   (width > std::max(height, depth) ) ? Triangle::compareX
                        :(height > std::max(width, depth) ) ? Triangle::compareY
                        : Triangle::compareZ;

    // Sort elements by the longest axis
    std::sort(elements.begin(), elements.end(), comparator);    
    std::size_t half_array_size = elements.size() / 2;

    left = std::make_shared<BVH>( std::vector<Triangle>(elements.begin(), elements.begin() + half_array_size), SPLIT_BALANCED );
    right = std::make_shared<BVH>( std::vector<Triangle>(elements.begin() + half_array_size, elements.end()), SPLIT_BALANCED );
}

void BVH::_split_sorted_midpoint() {
    float width = getWidth();
    float height = getHeight();
    float depth = getDepth();

    axis =  (width > std::max(height, depth) ) ?    0
            : (height > std::max(width, depth) ) ?  1
            :                                       2;

    auto comparator =   (width > std::max(height, depth) ) ? Triangle::compareX
                        :(height > std::max(width, depth) ) ? Triangle::compareY
                        : Triangle::compareZ;

    // Sort elements by the longest axis
    std::sort(elements.begin(), elements.end(), comparator);
    float splitPos = getCenter()[axis];

    std::size_t half_array_size = findClosest(elements, axis, elements.size(), splitPos);

    if (half_array_size == 0)
        half_array_size++;

    left = std::make_shared<BVH>( std::vector<Triangle>(elements.begin(), elements.begin() + half_array_size), SPLIT_SORTED_MIDPOINT );
    right = std::make_shared<BVH>( std::vector<Triangle>(elements.begin() + half_array_size, elements.end()), SPLIT_SORTED_MIDPOINT );
}

void BVH::_split_midpoint() {
    float width = getWidth();
    float height = getHeight();
    float depth = getDepth();

    axis =  (width > std::max(height, depth) ) ?    0
            : (height > std::max(width, depth) ) ?  1
            :                                       2;

    float splitPos = getCenter()[axis];
    std::vector<Triangle> left_el;
    std::vector<Triangle> right_el;

    for (size_t i = 0; i < elements.size(); i++) {
        if (elements[i].getCentroid()[axis] < splitPos)
            left_el.push_back(elements[i]);
        else
            right_el.push_back(elements[i]);
    }

    if (left_el.size() == 0 || right_el.size() == 0)
        leaf = true;
    else {
        left = std::make_shared<BVH>( left_el, SPLIT_MIDPOINT );
        right = std::make_shared<BVH>( right_el, SPLIT_MIDPOINT );
    }
}

float evaluateSAH( const std::vector<vera::Triangle>& _triangles, int _axis, float _pos ) {
    vera::BoundingBox leftBox;
    vera::BoundingBox rightBox;
    size_t leftCount = 0;
    size_t rightCount = 0;
    for( uint i = 0; i < _triangles.size(); i++ ) {
        if (_triangles[i].getCentroid()[_axis] < _pos) {
            leftBox.expand( _triangles[i] );
            leftCount++;
        }
        else {
            rightBox.expand( _triangles[i] );
            rightCount++;
        }
    }
    float cost = leftCount * leftBox.getArea() + rightCount * rightBox.getArea();
    return cost > 0 ? cost : 1e30f;
}

void BVH::_split_sah() {
    float width = getWidth();
    float height = getHeight();
    float depth = getDepth();

    int bestAxis = -1;
    float bestPos = 0;
    float bestCost = 1e30f;

    for( int a = 0; a < 3; a++ ) 
    for( size_t i = 0; i < elements.size(); i++ ) {
        float candidatePos = elements[i].getCentroid()[a];
        float cost = evaluateSAH( elements, a, candidatePos );
        if (cost < bestCost) {
            bestPos = candidatePos;
            bestAxis = a;
            bestCost = cost;
        }
    }
    axis = bestAxis;
    float splitPos = bestPos;

    std::vector<Triangle> left_el;
    std::vector<Triangle> right_el;

    for (size_t i = 0; i < elements.size(); i++) {
        if (elements[i].getCentroid()[axis] < splitPos)
            left_el.push_back(elements[i]);
        else
            right_el.push_back(elements[i]);
    }

    std::cout << left_el.size() << "/" << right_el.size() << std::endl;
    if (left_el.size() == 0 || right_el.size() == 0)
        leaf = true;
    else {
        left = std::make_shared<BVH>( left_el, SPLIT_SAH );
        right = std::make_shared<BVH>( right_el, SPLIT_SAH );
    }
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

        if (left_hit != nullptr && right_hit != nullptr)
            return std::make_shared<BVH>( *this );
        else if (left_hit != nullptr) 
            return left_hit;
        else if (right_hit != nullptr)
            return right_hit;

        return nullptr;
    }
    else
        return std::make_shared<BVH>( *this );
}

float BVH::getMinDistance(const glm::vec3& _point) const {
    float minDist = 3.0e+038;
    if (leaf) {
        for (size_t i = 0; i < elements.size(); i++) {
            float d = elements[i].unsignedDistance(_point);
            if (d < minDist)
                minDist = d;
        }
    }
    else if (right != nullptr && left != nullptr) {
        float left_dist = left->getMinDistance(_point);
        float right_dist = right->getMinDistance(_point);
        if (left_dist <= right_dist)
            return left_dist;
        else 
            return right_dist;
    }
    return minDist;
}

float BVH::getMinSignedDistance(const glm::vec3& _point) const {
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
        float left_dist  = abs( _point[axis] - left->getClosestOn(_point[axis], axis) );
        float right_dist = abs( _point[axis] - right->getClosestOn(_point[axis], axis) );

        if ( abs(left_dist - right_dist) <= left_dist * 0.1 ) {
            left_dist = left->getMinSignedDistance(_point);
            right_dist = right->getMinSignedDistance(_point);
            if (abs(left_dist) < abs(right_dist))
                return left_dist;
            else 
                return right_dist;
        }
        else if (left_dist <= right_dist)
            return left->getMinSignedDistance(_point);
        else 
            return right->getMinSignedDistance(_point);

    }
    return minDist;
}

}