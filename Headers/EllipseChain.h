#pragma once
#include <iostream>
#include <cmath>
#include <numbers>
#include <unordered_set>
#include "V2.h"
#include "Defines.h"
#include "Utilities.h"
#include "Circle.h"


/*This file contains the framework for a group of circles being linked together in a chain. The 
adding of circles to the chain works through the circleadd function / adding circles to the "group". 
The chain holds positions, which circles are in it, and voltage, while allowing for you to move 
the whole chain of circles at once.*/

// Class to hold a group of N ellipses by modifying the CircleContainer group
struct Chain {
    // group is list of N ellipses, positions defines relative location of each ellipse (first ellipse has rel pos of {0,0})
    Chain(const V2<int> position,const CircleContainer& group,const std::vector<V2<int>>& rel_positions, Value value = 0) : position{ position }, group{ group }, rel_positions{ rel_positions }, value{ value }, id{ ChainCount() } {}
    // Initialize core vars
    V2<int> position;   // position defined by first ellipse in group, this position defines the base of the chain and modifies the position of the rest of the chain when it is moved
    CircleContainer group;
    std::vector<V2<int>> rel_positions;
    Value value;
    
    // define other core vars
    V2<int> pos_old;     // defined by first ellipse
    V2<double> pos_leftover;
    V2<Value> velocity = { 0.0f, 0.0f };
    V2<Value> acceleration = { 0.0f, 0.0f };
    int id;


    // Redefine == and != for chains
    bool operator==(const Chain& other) const {
        return id == other.id;
    }

    bool operator!=(const Chain& other) const {
        return !(*this == other);
    }
    // Give each chain a unique id
    static int ChainCount() {
        static int id{ 0 };
        return id++;
    }
    // Find all points on the perimeter of the chain
    std::vector<V2<int>> GetPerimeterPoints() const{
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

    // "moves" the entire chain by updating the position of each circle relative to the main one
    void UpdateChain() {
        // updates member positions and values with respect to grid
        for (std::size_t i = 0; i < group.size(); ++i) {
            group[i].position = position + rel_positions[i];
            group[i].value = value;
        }
    }
    // Adds a new cirlce to the chain group
    void AddCircle(const Circle& circle) {
    group.emplace_back(circle);
    rel_positions.emplace_back(circle.position - position);
    UpdateChain();
    }
    
};