#pragma once
#include <iostream>
#include <vector>
#include <unordered_set>
#include <utility>
#include "V2.h"

/* This file contrains some utlility functions that can print a container of V2s
This is not in V2 since its only needed in very specific circumstances and is kinda heavy.*/

inline std::ostream& operator<<(std::ostream& os, const std::vector<V2<int>>& v) {
	os << "[";
	for (std::size_t i = 0; i < v.size(); ++i) {
		os << v[i];
		if (i != v.size() - 1) {
			os << ", ";
		}
	}
	os << "]";
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::unordered_set<V2<int>>& v) {
	os << "[";
	for (auto it = v.begin(); it != v.end(); ++it) {
		os << *it;
		if (std::next(it) != v.end()) {
			os << ", ";
		}
	}
	os << "]";
	return os;
}

