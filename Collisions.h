#pragma once
#include "V2.h"
#include "Circle.h"
#include "Rectangle.h"
#include "Triangle.h"
#include "Needle.h"

/* This file includes lots of functions to detect if a collision has occured. The intersect
functions are primed for optimization but look like legacy code*/

// Determines if a point is in contact with a rectangle (not all)
inline bool PointVsRectangle(const V2<int>& point, const Rectangle& rectangle) {
    if (point.x < rectangle.position.x || point.x > rectangle.position.x + rectangle.size.x) return false;
    if (point.y < rectangle.position.y || point.y > rectangle.position.y + rectangle.size.y) return false;
    return true;
}


// Checks whether a point lies inside or on the boundary of the ellipse defined by a and b
inline bool PointVsCircle(const V2<int>& point, const Circle& circle) {
    auto distance = point - circle.position;
    auto alpha = static_cast<double>(distance.x * distance.x) / static_cast<double>(circle.a * circle.a);
    auto beta = static_cast<double>(distance.y * distance.y) / static_cast<double>(circle.b * circle.b);
    return ((alpha + beta) <= 1);
}

// Determines if a point is ON a circle's edge
inline bool PointVsCirclePerimeter(const V2<int>& point, const Circle& circle) {
    auto distance = point - circle.position;
    auto alpha = static_cast<double>(distance.x * distance.x) / static_cast<double>(circle.a * circle.a);
    auto beta = static_cast<double>(distance.y * distance.y) / static_cast<double>(circle.b * circle.b);
    // return alpha + beta == 1; // Setting a floating point equal to an int? Asking for trouble
    double s = alpha + beta;
    double eps = 0.001;
    return std::abs(s - 1.0) < eps;
}

// Checks whether any perimeter point of ellipse A lies inside ellipse B
inline bool CircleVsCircle(const Circle& A, const Circle& B) {
    std::vector<V2<int>> perim = A.GetPerimeterPoints();
    for (auto p : perim) {
        if (PointVsCircle(p, B)) {
            return true;
        }
    }
    return false;
}

// Checks whether any cached points on a triangle are in a circle's radius 
inline bool CircleVsTriangle(const Circle& A, const Triangle& tri) {
    for (auto& p : tri.points) {
        if (PointVsCircle(p, A)) {
            return true;
        }
    }
    return false;
}

// Checks whether any cached points on a needle are in a circle's radius
inline bool CircleVsNeedle(const Circle& A, const Needle& needle) {
    for (auto& p : needle.points) {
        if (PointVsCircle(p, A)) {
            return true;
        }
    }
    return false;
}

// Determines which point on the edge of a triangle is closest to the center of a circle
inline V2<int> ClosestPointOnTriangle(const Circle& A, const Triangle& tri) {
    double sq_dist = 9999;
    V2<int> closest_point;
    for (auto& P : tri.edge_points) {
        auto dif = P - A.position;
        if ((dif.x * dif.x) + (dif.y * dif.y) < sq_dist) {
            sq_dist = (dif.x * dif.x) + (dif.y * dif.y);
            closest_point = P;
        }
    }
    return closest_point;
}

// Determines which point on the edge of a needle is closest to the center of a circle
inline V2<int> ClosestPointOnNeedle(const Circle& A, const Needle& needle) {
    double sq_dist = 10E5;
    V2<int> closest_point;
    for (auto& P : needle.edge_points) {
        auto dif = P - A.position;
        auto dif_mag = (dif.x * dif.x) + (dif.y * dif.y);
        if (dif_mag < sq_dist) {
            sq_dist = (dif.x * dif.x) + (dif.y * dif.y);
            closest_point = P;
        }
    }
    return closest_point;
}

// returns True if at least one point in A meets one point in B
inline bool EdgeIntersect(std::vector<V2<int>>& A, std::vector<V2<int>>& B) {
    for (auto& P : A) {
        for (auto& Q : B) {
            if (P == Q) {
                //LOG(P);
                return true;
            }
        }
    }
    return false;
}

// Computes the square distance between a point p and a rectangle define as AABB b
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

// Checks circle has intersected the circle's collision radius
inline bool CircleVsRectangle(const Circle& circle, const Rectangle& b) {
    // If rectangles ever used again, this should be updated to accomodate ellipse definitions (with a, b instead of radius)
    // Compute squared distance between sphere center and AABB
        float sqDist = SqDistPointAABB(circle.position, b);
        // Sphere and AABB intersect if the (squared) distance
        // between them is less than the (squared) sphere radius
    return sqDist <= circle.radius * circle.radius;
}

// Finds the closest rectangle point relative to an input position
inline V2<int> ClosestPointOnRectangle(const V2<int>& position, const Rectangle& b) {
    // below does not work inside left electrode
    auto closest_point = position;
    closest_point.x = std::max(closest_point.x, b.position.x);
    closest_point.x = std::min(closest_point.x, b.position.x + b.size.x);

    closest_point.y = std::max(closest_point.y, b.position.y);
    closest_point.y = std::min(closest_point.y, b.position.y + b.size.y);

    return closest_point;
}




// deprecated 
// This code is not actively used but could be one day, pushed down here to keep things neater

// These next two functions appear to be inactive
// Determines if there is interframe collision (prevents circle hopping or 'teleporting')
inline V2<int> Intersect(const V2<int>& Pf, const V2<int>& Pi, std::vector<V2<int>>& boundary) {
    auto vect = Pf - Pi;
    int ticks = 1000;
    for (int i = 0; i <= ticks; i++) {
        V2<int> test_coord = Round(i * vect / ticks) + Pi;
        for (auto& P : boundary) {
            if (test_coord == P) {
                return(test_coord);
            }
        }
    }
    return(Pf);
}
// approximates an interframe collision location for a moving ellipse against a boundary using sampled motion steps and perimeter matching
inline V2<int> ChainIntersect(V2<int>& old_pos, const Circle& A, const std::vector<V2<int>>& boundary) {
    // returns list containing position of A, position of B, and intersection point
    auto vect = A.position - old_pos;
    int ticks = 1000;
    for (int i = 0; i <= ticks; i++) {
        V2<int> test_coord = Round(i * vect / ticks) + old_pos;
        auto perimA = A.GetPerimeterPoints();
        for (auto& P : perimA) {
            for (auto& Q : boundary) {
                if (P == Q) {
                    return test_coord;
                }
            }
        }
    }
    LOG("No suitable intersection found");
    return { 0,0 };
}
