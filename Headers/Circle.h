#pragma once
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numbers>
#include <unordered_set>
#include "V2.h"
#include "Defines.h"
#include "Utilities.h"

/* This file contains the cicle class and some key functions. One function of note is the 
 GetPerimeterPoints() function which uses 1000 iterations, can probably be reduced to something
 like 200 seeing as most of the circles have a total circumference of ~100 points */


/* Class that creates ellipses based on position, a = horizontal radius, b = vertical radius, 
radius = collision interaction radius, and value = voltage. This class contains positions, velocity,
accelerations, and other core parameters for collision and transit calcuations.*/
struct Circle {
    // a is semi-major (horizontal) axis, b is semi-minor (vertical) axis
    Circle(const V2<int>& position, int a, int b, int radius = 0, Value value = 0) : position{ position }, a{ a }, b{ b }, radius{ radius }, value{ value }, id{ CircleCount() } {}
    V2<int> position;
    V2<int> pos_old;    // position of ellipse in previous frame
    V2<double> pos_leftover;
    V2<Value> velocity = { 0.0f, 0.0f };
    V2<Value> acceleration = { 0.0f, 0.0f };
    int a;
    int b;
    int radius{ 0 };
    Value value;
    int id;
    // Redefine == and != operators to check if two circles are the same
    bool operator==(const Circle& other) const {
        return id == other.id;
    }

    bool operator!=(const Circle& other) const {
        return !(*this == other);
    }
    // Creates a unique ID for each circle
    static int CircleCount() {
        static int id{ 0 };
        return id++;
    }
    // Creates a V2 vector for the points on the perimeter of the circle using a simple rounding method
    std::vector<V2<int>> GetPerimeterPoints() const {
        std::vector<V2<int>> perimeter_points;

        int num_points = 1000;
        // Loop to calculate points at different angles
        for (int i = 0; i < num_points; ++i) {
            double theta = 2 * std::numbers::pi * i / num_points;  // Angle between 0 and 2pi
            double x = a * cos(theta) + position.x;  // x coordinate
            double y = b * sin(theta) + position.y;  // y coordinate
            V2<int> point = Round(V2<double>{ x, y });
            if (std::find(perimeter_points.begin(), perimeter_points.end(), point) == perimeter_points.end()) {
                perimeter_points.push_back(point);  // Add point to the vector
            }
        }

        // These commented out sections are alternative methods for getting the points, can be deleted later if you want
        /*
        double arc = 0.5;
        double phi = 0;
        while (phi < 2 * M_PI) {
            V2<int> point = Round(V2<double>(position.x + radius * cos(phi), position.y + radius * sin(phi)));
            if (std::find(perimeter_points.begin(), perimeter_points.end(), point) != perimeter_points.end()) {
                phi += arc / radius;
                continue;
            }
            else {
                perimeter_points.emplace(point);
                phi += arc / radius;
            }
        }*/

        /*
        V2<int> point{ 0,radius };
        int d = 3 - 2 * radius;
        AddPerimeterPoints(perimeter_points, point);
        while (point.y >= point.x) {
            point.x++;
            if (d > 0) {
                point.y--;
                d = d + 4 * (point.x - point.y) + 10;
            } else {
                d = d + 4 * point.x + 6;
            }
            AddPerimeterPoints(perimeter_points, point);
        }
        */
        /*
        for (auto i : perimeter_points) {
            bool i_j = false;
            for (auto j : perimeter_points) {
                if (i == j && !i_j) {
                    i_j = true;
                }
                else if (i == j && i_j) {

                }
            }
        }
        */
        return perimeter_points;
    }
// Older, now defunct function that can be used to generate perimeter points based on known boundry conditions
// private:
//     void AddPerimeterPoints(std::unordered_set<V2<int>>& v, V2<int> point ) const {
//         v.emplace(position.x + point.x, position.y + point.y);
//         v.emplace(position.x - point.x, position.y + point.y);
//         v.emplace(position.x + point.x, position.y - point.y);
//         v.emplace(position.x - point.x, position.y - point.y);
//         v.emplace(position.x - point.y, position.y + point.x);
//         v.emplace(position.x + point.y, position.y - point.x);
//         v.emplace(position.x - point.y, position.y - point.x);
//         v.emplace(position.x + point.y, position.y + point.x);
//     }
};