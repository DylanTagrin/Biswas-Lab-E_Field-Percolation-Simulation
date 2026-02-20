#pragma once
#include "V2.h"
#include "Circle.h"
#include "Rectangle.h"

inline bool PointVsRectangle(const V2<int>& point, const Rectangle& rectangle) {
    if (point.x < rectangle.position.x || point.x > rectangle.position.x + rectangle.size.x) return false;
    if (point.y < rectangle.position.y || point.y > rectangle.position.y + rectangle.size.y) return false;
    return true;
}

inline bool PointVsCircle(const V2<int>& point, const Circle& circle) {
    auto distance = point - circle.position;
    return distance.x * distance.x + distance.y * distance.y <= circle.radius * circle.radius;
}

inline bool PointVsCirclePerimeter(const V2<int>& point, const Circle& circle) {
    auto distance = point - circle.position;
    return distance.x * distance.x + distance.y * distance.y == circle.radius * circle.radius;
}

inline bool CircleVsCircle(const Circle& A, const Circle& B) {
    double r = A.radius + B.radius;
    r *= r;
    return r >= (A.position - B.position).MagnitudeSquared();
}

// Computes the square distance between a point p and an AABB b
inline float SqDistPointAABB(const V2<int>& p, const Rectangle& b) {
    float sqDist = 0.0f;
    // For each axis count any excess distance outside box extents
    if (p.x < b.position.x) sqDist += (b.position.x - p.x) * (b.position.x - p.x);
    if (p.x > b.position.x + b.size.x) sqDist += (p.x - b.position.x - b.size.x) * (p.x - b.position.x - b.size.x);

    //if (p.y < b.position.y - b.size.y) sqDist += (b.position.y - b.size.y - p.y) * (b.position.y - b.size.y - p.y);
    //if (p.y > b.position.y) sqDist += (p.y - b.position.y) * (p.y - b.position.y);

    if (p.y < b.position.y) sqDist += (b.position.y - p.y) * (b.position.y - p.y);
    if (p.y > b.position.y + b.size.y) sqDist += (p.y - b.position.y - b.size.y) * (p.y - b.position.y - b.size.y);

    return sqDist;
}

inline bool CircleVsRectangle(const Circle& circle, const Rectangle& b) {
// Compute squared distance between sphere center and AABB
        float sqDist = SqDistPointAABB(circle.position, b);
        // Sphere and AABB intersect if the (squared) distance
        // between them is less than the (squared) sphere radius
    return sqDist <= circle.radius * circle.radius;
}

inline V2<int> ClosestPointOnRectangle(const V2<int>& position, const Rectangle& b) {
    auto closest_point = position;
    closest_point.x = std::max(closest_point.x, b.position.x);
    closest_point.x = std::min(closest_point.x, b.position.x + b.size.x);

    closest_point.y = std::max(closest_point.y, b.position.y);
    closest_point.y = std::min(closest_point.y, b.position.y + b.size.y);

    return closest_point;
}