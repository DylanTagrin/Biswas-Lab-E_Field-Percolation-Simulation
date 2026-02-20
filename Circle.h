#pragma once
#include <unordered_set>
#include "V2.h"
#include "Defines.h"
#include "Utilities.h"

struct Circle {
    Circle(const V2<int>& position, int radius, Value value = 0) : position{ position }, radius{ radius }, value{ value }, id{ CircleCount() } {}
    V2<int> position;
    V2<Value> velocity;
    V2<Value> acceleration;
    int radius{ 0 };
    Value value{ 0 };
    int id;

    bool operator==(const Circle& other) const {
        return id == other.id;
    }

    bool operator!=(const Circle& other) const {
        return !(*this == other);
    }

    static int CircleCount() {
        static int id{ 0 };
        return id++;
    }

    std::unordered_set<V2<int>> GetPerimeterPoints() const {
        std::unordered_set<V2<int>> perimeter_points;
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
        return perimeter_points;
    }

private:
    void AddPerimeterPoints(std::unordered_set<V2<int>>& v, V2<int> point ) const {
        v.emplace(position.x + point.x, position.y + point.y);
        v.emplace(position.x - point.x, position.y + point.y);
        v.emplace(position.x + point.x, position.y - point.y);
        v.emplace(position.x - point.x, position.y - point.y);
        v.emplace(position.x - point.y, position.y + point.x);
        v.emplace(position.x + point.y, position.y - point.x);
        v.emplace(position.x - point.y, position.y - point.x);
        v.emplace(position.x + point.y, position.y + point.x);
    }
};