#pragma once
#include "V2.h"
#include "Defines.h"

/* This one is a bit of a mess, in short the needle is split partially into Grid.h, shape 
rasterization should be moved into here eventually.
This file contains the Needle struct and the functions that determine if a point p is inside or 
on the edge of the needle. The Needle class holds its 3 verticies with V3 being the 'point', 
voltage, points inside and on the edge, and the control point for defining the dezier curve.*/


/* Needle class initialized with V2<int> for the 3 verticies of a needle with
v3 being the far point.*/
struct Needle {
	Needle(const V2<int>& v1, const V2<int>& v2, const V2<int>& v3, \
		Value value = 0) : v1{ v1 }, v2{ v2 }, v3{ v3 }, value {value} {}
	// v1, v2 form straight edge; midpoint of straight edge is control point for Bezier curves
	V2<int> v1;
	V2<int> v2;
	V2<int> v3;
	Value value{ 0 };

	V2<int> cp = V2<int>{ v1.x, v3.y };
public:
	mutable std::vector<V2<int>> points;
	mutable std::vector<V2<int>> edge_points;
};
// Determines if point P is within the points inside the needle
bool InNeedle(const Needle& needle, V2<int> P) {
	// note: in current version, edge points belong to this point list as well
	if (std::find(needle.points.begin(), needle.points.end(), P) != needle.points.end()) {
		return true;
	}
	else {
		return false;
	}
}
// Determines if point P is the same as any of the points on the needle's edges
bool OnNeedleEdge(const Needle& needle, V2<int> P) {
	if (std::find(needle.edge_points.begin(), needle.edge_points.end(), P) != needle.edge_points.end()) {
		return true;
	}
	else {
		return false;
	}
}