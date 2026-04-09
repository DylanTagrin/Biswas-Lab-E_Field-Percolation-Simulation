#pragma once
#include "V2.h"
#include "Defines.h"

/* This file defines a class structure for a Triangle shaped object that can be drawn in the grid.
The triangle struct holds its 3 vertex points, along with edge lengths. After there's a series
of functions for calculating the triangle's area, seeing if a point is IN the triangle, and seeing
if a point is ON the triange edge.*/

/* Constructs class object of a triangle based on 3 V2<int> verticies. 
Holds vertex points, side lengths, and perimeter. Also contains a mutable
points and edge_points vector that can be stored for later use in grid.h*/
struct Triangle {
	Triangle(const V2<int>& v1, const V2<int>& v2, const V2<int>& v3, Value value = 0) : v1{ v1 }, v2{ v2 }, v3{ v3 }, value{ value } {}
	V2<int> v1;
	V2<int> v2;
	V2<int> v3;
	Value value{ 0 };
	double L12 = std::sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));
	double L23 = std::sqrt((v2.x - v3.x) * (v2.x - v3.x) + (v2.y - v3.y) * (v2.y - v3.y));
	double L31 = std::sqrt((v1.x - v3.x) * (v1.x - v3.x) + (v1.y - v3.y) * (v1.y - v3.y));
	double perimeter = L12 + L23 + L31;
public:
	mutable std::vector<V2<int>> points;
	mutable std::vector<V2<int>> edge_points;
};

// Function that returns the area of a Triangle class object 
double TriangleArea(const Triangle& tri) {
	float s = tri.perimeter / 2;
	double A = std::sqrt(s * (s - tri.L12) * (s - tri.L23) * (s - tri.L31));
	return A;
}
// Function that returns a Bool if a point V2<int> is in a Triangle
bool InTriangle(const Triangle& tri, V2<int> P) {
	// determines if point P in triangle tri by adding up 3 triangles made with point p and seeing if the sum ~= tri area
	auto A1 = TriangleArea(Triangle(tri.v1, tri.v2, P, 0));
	auto A2 = TriangleArea(Triangle(tri.v2, tri.v3, P, 0));
	auto A3 = TriangleArea(Triangle(tri.v1, tri.v3, P, 0));
	float area_dif = A1 + A2 + A3 - TriangleArea(tri);
	if (abs(area_dif) / TriangleArea(tri) < 0.001) {
		return true;
	}
	else {
		return false;
	}
}
// Function that returns a Bool if a point V2<int> is on a Triangle edge
bool OnTriangleEdge(const Triangle& tri, V2<int> P) {
	// determines if point P on edge of triange tri
	auto A1 = TriangleArea(Triangle(tri.v1, tri.v2, P, 0));
	auto A2 = TriangleArea(Triangle(tri.v2, tri.v3, P, 0));
	auto A3 = TriangleArea(Triangle(tri.v1, tri.v3, P, 0));
	auto tri_area = TriangleArea(tri);
	if (A1 / tri_area < 0.001 || A2 / tri_area < 0.001 || A3 / tri_area < 0.001) return true;
	return false;
}