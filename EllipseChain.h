#pragma once
#include <iostream>
#include <cmath>
#include <numbers>
#include <unordered_set>
#include "V2.h"
#include "Defines.h"
#include "Utilities.h"
#include "Circle.h"


struct Chain {
    // group is list of N ellipses, positions defines relative location of each ellipse (first ellipse has rel pos of {0,0})
    Chain(V2<int> position, CircleContainer& group, std::vector<V2<int>>& rel_positions, Value value = 0) : position{ position }, group{ group }, rel_positions{ rel_positions }, value{ value }, id{ ChainCount() } {}
    V2<int> position;   // position defined by first ellipse in group
    CircleContainer group;
    std::vector<V2<int>> rel_positions;
    Value value;
    

    V2<int> pos_old;     // defined by first ellipse
    V2<double> pos_leftover;
    V2<Value> velocity = { 0.0f, 0.0f };
    V2<Value> acceleration = { 0.0f, 0.0f };
    int id;



    bool operator==(const Chain& other) const {
        return id == other.id;
    }

    bool operator!=(const Chain& other) const {
        return !(*this == other);
    }
    
    static int ChainCount() {
        static int id{ 0 };
        return id++;
    }

    std::vector<V2<int>> GetPerimeterPoints() {
        std::vector<V2<int>> perimeter_points;
        for (const Circle& A : group) {
            auto perim = A.GetPerimeterPoints();
            for (auto& i : perim) {
                // issue if points binding two ellipses experiences significant forces
                if (std::find(perimeter_points.begin(), perimeter_points.end(), i) == perimeter_points.end()) {
                    perimeter_points.emplace_back(i);
                }
            }
        }
        return perimeter_points;
    }

    
    void UpdateChain() {
        // updates member positions and values with respect to grid
        for (std::size_t i = 0; i < group.size(); ++i) {
            group[i].position = position + rel_positions[i];
            group[i].value = value;
        }
    }
    
};