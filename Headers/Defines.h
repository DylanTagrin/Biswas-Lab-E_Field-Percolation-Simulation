#pragma once
#include <vector>
#include <tuple>
#include <iostream>

/* This file contains a handful of definitions that are reused throughout the system.*/

// Defines LOG to do a quick cout, kind of strange, but whatever
#define LOG(x) { std::cout << x << std::endl; }
#define LOG_(x) { std::cout << x; }

// Predeclares some essential structs to reduce compiler errors and allow for the "using" defs
struct Circle;
struct Chain;
// Note: These two might not be used and can be slated for removal
struct circle_comparator;
struct circle_pair_comparator;

// provides alternative naming for a handful of reused values and conveniences
using Value = float;
using CircleContainer = std::vector<Circle>;
using ChainContainer = std::vector<Chain>;
using CollisionContainer = std::vector<std::pair<Circle, Circle>>;

// Provides a template for events such as Data<int>{"width", grid_size.x} (keeps things short)
template <typename T>
using Data = std::pair<const char*, T>;