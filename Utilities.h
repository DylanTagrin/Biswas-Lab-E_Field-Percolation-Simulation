#pragma once
#include <iostream>
#include <vector>
#include <unordered_set>
#include <utility>
#include "V2.h"


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

// Hash function from https://ideone.com/tieHbd

//namespace std {
//
//// Custom hashing function for ecs::Entity class.
//// This allows for use of unordered maps and sets with entities as keys.
//	template <>
//	struct hash<V2<int>> {
//		std::size_t operator()(const V2<int>& k) const {
//			// Hashing combination algorithm from:
//			// https://stackoverflow.com/a/17017281
//			std::size_t h = 17;
//			h = h * 31 + std::hash<int>()(k.x);
//			h = h * 31 + std::hash<int>()(k.y);
//			return h;
//		}
//	};
//
//} // namespace std