#pragma once
#include <vector>
#include <tuple>
#include <iostream>



#define LOG(x) { std::cout << x << std::endl; }
#define LOG_(x) { std::cout << x; }

struct Circle;
struct Chain;
struct circle_comparator;
struct circle_pair_comparator;

using Value = float;
using CircleContainer = std::vector<Circle>;
using ChainContainer = std::vector<Chain>;
using CollisionContainer = std::vector<std::pair<Circle, Circle>>;

template <typename T>
using Data = std::pair<const char*, T>;