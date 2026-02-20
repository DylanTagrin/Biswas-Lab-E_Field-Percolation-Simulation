#pragma once
#include <cassert>
#include <functional>
#include <limits>
#include <type_traits>

#include "Defines.h"
#include "Utilities.h"
#include "Collisions.h"
#include "Grid.h"
#include "DataHandler.h"

class Simulation {
public:
    Simulation(const V2<int>& grid_size) : 
        relaxed_grid{ grid_size },
        data_handler{ 
            Data<int>{ "width", grid_size.x },
            Data<int>{ "height", grid_size.y },
            Data<int>{ "timestep", 1 },
            Data<std::vector<std::vector<Value>>>{ "data", std::vector<std::vector<Value>>{} } 
        } {
        relaxed_grid.InitValue();
    }

    void Start(int relaxation_loops) {
        loops = relaxation_loops;
        UpdateRectangles(relaxed_grid);
        Relax(relaxed_grid, loops, false);
    }

    void Save(const char* file_path) {
        data_handler.SaveToFile(file_path);
    }

    void Update() {
        auto circle_grid = relaxed_grid;
        for (auto& circle : circles) {
            circle.value = circle_grid[circle.position];
        }
        UpdateCircles(circle_grid, circles);
        for (auto& static_circle : static_circles) {
            LOG("Static Circle Value:" << static_circle.value);
        }
        UpdateCircles(circle_grid, static_circles);
        Relax(circle_grid, loops, true);
        auto electric_fields = ComputeElectricField(circle_grid);
        data_handler.Add("data", electric_fields.first.GetVector());
        UpdatePhysics(electric_fields.second);
        UpdateCollisions();
    }

    void UpdatePhysics(const Grid<Value>& electric_field_squared) {
        for (auto& circle : circles) {
            auto force = ComputeForceOnCircle(electric_field_squared, circle);
            circle.acceleration = { 1.0f * force.x, 1.0f * force.y };
            LOG(force.x << "," << force.y);
            circle.velocity += { 1.0f * force.x, 1.0f * force.y };
            circle.position += Round(circle.velocity);
        }
    }

    int GetCombinedRadius(const Circle& a, const Circle& b) {
        return static_cast<int>(std::round(std::sqrt(a.radius * a.radius + b.radius * b.radius)));
    }

    CircleContainer CollisionRecursion(CircleContainer& all_circles, CollisionContainer& out_collisions, CircleContainer& out_remainder) {
        CollisionSubroutine(all_circles, out_collisions, out_remainder);
        if (out_collisions.size() > 0) {
            LOG("Collision found!");
            // in for-loop arguments, colon replaced with semicolon
            // in for-loop, I iterate over every object in 'out_collisions'; I also def a and b as elements of each pair in 'out_collisions';
            int size = sizeof(out_collisions) / sizeof(out_collisions[0]);
            for (int i = 0; i < size; i++) {
                auto a = out_collisions[i].first;
                auto b = out_collisions[i].second;
                auto new_position = Round((a.position + b.position) / 2.0);
                auto new_radius = GetCombinedRadius(a, b);
                LOG("New Position: " << new_position);
                LOG("New Radius: " << new_radius);
                out_remainder.emplace_back(new_position, new_radius);
            }
            all_circles = out_remainder;
            out_collisions.clear();
            out_remainder.clear();
            return CollisionRecursion(all_circles, out_collisions, out_remainder);
        } else {
            return out_remainder;
        }
    }

    void UpdateCollisions() {
        CircleContainer all_circles{ circles };
        CollisionContainer out_collisions;
        CircleContainer out_remainder;
        //out_remainder = CollisionRecursion(all_circles, out_collisions, out_remainder);
        //circles = out_remainder;
        CircleContainer collided;
        CircleContainer new_circles;
        for (auto& a : circles) {
            for (auto& b : circles) {
                if (a != b && CircleVsCircle(a, b) && !HasCircle(a, collided) && !HasCircle(b, collided)) {
                    auto new_position = Round((a.position + b.position) / 2.0);
                    auto new_radius = GetCombinedRadius(a, b);
                    LOG("New Position: " << new_position);
                    LOG("New Radius: " << new_radius);
                    new_circles.emplace_back(new_position, new_radius);
                    collided.emplace_back(a);
                    collided.emplace_back(b);
                    break;
                }
            }
        }
        for (auto& circle : circles) {
            if (!HasCircle(circle, collided)) {
                new_circles.emplace_back(circle);
            }
        }
        circles = new_circles;
        CircleContainer new_static;
        out_remainder.clear();
        for (auto& circle : circles) {
            bool colliding = false;
            for (auto& static_circle : static_circles) {
                if (!colliding && !HasCircle(circle, static_circles) && CircleVsCircle(circle, static_circle)) {
                    LOG("Static Circle collision!");
                    colliding = true;
                    new_static.emplace_back(static_circle.position, GetCombinedRadius(circle, static_circle), static_circle.value);
                }
            }
            for (const auto& rect : rectangles) {
                if (!colliding && !HasCircle(circle, static_circles) && CircleVsRectangle(circle, rect)) {
                    LOG("Electrode collision!");
                    colliding = true;
                    circle.position = ClosestPointOnRectangle(circle.position, rect);
                    circle.value = rect.value;
                    new_static.emplace_back(circle);
                }
            }
            if (!colliding) {
                out_remainder.emplace_back(circle);
            }
        }
        circles = out_remainder;
        for (const auto& static_circle : static_circles) {
            if (!HasCircle(static_circle, new_static)) {
                new_static.emplace_back(static_circle);
            }
        }
        static_circles = new_static;



        CircleContainer static_collided;
        CircleContainer static_new_circles;
        for (auto& a : static_circles) {
            for (auto& b : static_circles) {
                if (a != b && CircleVsCircle(a, b) && !HasCircle(a, static_collided) && !HasCircle(b, static_collided)) {
                    auto new_position = Round((a.position + b.position) / 2.0);
                    auto new_radius = GetCombinedRadius(a, b);
                    LOG("New Position: " << new_position);
                    LOG("New Radius: " << new_radius);
                    static_new_circles.emplace_back(new_position, new_radius, a.value);
                    static_collided.emplace_back(a);
                    static_collided.emplace_back(b);
                    break;
                }
            }
        }
        for (auto& circle : static_circles) {
            if (!HasCircle(circle, static_collided)) {
                static_new_circles.emplace_back(circle);
            }
        }
        static_circles = static_new_circles;


    }

    bool HasCircle(const Circle& circle, const CircleContainer& container) {
        for (const auto& c : container) {
            if (circle == c) return true;
        }
        return false;
    }

    void Relax(Grid<Value>& grid, int relaxation_loops, bool force_circles) {
        auto size = grid.GetSize();
        for (auto i = 0; i < relaxation_loops; ++i) {
            RelaxGrid(size, grid, force_circles);
            //LOG(i);
        }
    }

    //@return electric_field, electric_field_squared
    std::pair<Grid<Value>, Grid<Value>> ComputeElectricField(const Grid<Value>& potentials) const {
        Grid<Value> electric_field{ potentials.GetSize() };
        electric_field.InitValue();
        auto electric_field_squared = electric_field;
        auto gradient_grid = potentials.GetGradient();
        for (auto i = 0; i < electric_field.GetLength(); i++) {
            auto magnitude_squared = static_cast<Value>(gradient_grid[i].MagnitudeSquared());
            electric_field_squared[i] = magnitude_squared;
            electric_field[i] = std::sqrt(magnitude_squared);
        }
        return { electric_field, electric_field_squared };
    }

    void RelaxGrid(const V2<int>& size, Grid<Value>& grid, bool forces_circles) {
        for (auto i = 1; i < size.x - 1; ++i) {
            auto x_last = i - 1;
            auto x_next = i + 1;
            for (auto j = 1; j < size.y - 1; ++j) {
                auto y_part = size.x * j;
                auto y_index_minus = y_part - size.x;
                auto y_index_plus = y_part + size.x;
                grid[y_part + i] = (
                      grid[y_index_minus + x_last]
                    + grid[y_index_minus + i]
                    + grid[y_index_minus + x_next]
                    + grid[y_part + x_last]
                    + grid[y_part + x_next]
                    + grid[y_index_plus + x_last]
                    + grid[y_index_plus + i]
                    + grid[y_index_plus + x_next]
                ) / static_cast<Value>(8.0);
            }
        }
        UpdateRectangles(grid);
        if (forces_circles) {
            UpdateCircles(grid, circles);
            UpdateCircles(grid, static_circles);
        }
    }

    //V2<Value> ComputeForceOnCircle(const Grid<Value>& electric_field_squared, const Circle& circle) const {
    //    V2<Value> force;
    //    auto gradient_grid = electric_field_squared.GetGradient();
    //    for (auto i = circle.position.x - circle.radius; i < circle.position.x + circle.radius; ++i) {
    //        for (auto j = circle.position.y - circle.radius; j < circle.position.y + circle.radius; ++j) {
    //            V2<int> point = { i, j };
    //            if (PointVsCirclePerimeter(point, circle)) {
    //                force += gradient_grid[point];
    //            }
    //        }
    //    }
    //    return force;
    //}

    V2<Value> ComputeForceOnCircle(const Grid<Value>& electric_field_squared, const Circle& circle) const {
        V2<Value> force;
        auto gradient_grid = electric_field_squared.GetGradient();
        auto perimeter_points = circle.GetPerimeterPoints();
        for (const auto& point : perimeter_points) {
            force += gradient_grid[point];
        }
        return force;
    }

    std::vector<int> ComputeForceTest() {
        std::vector<int> forces;
        for (const auto& circle : circles) {
            LOG("Looking at next circle.")
            int force = 0;
            auto perimeter_points = circle.GetPerimeterPoints();
            LOG(perimeter_points);
            force += perimeter_points.size();
            forces.push_back(force);
        }
        return forces;
    }

    void CollisionSubroutine(const CircleContainer& all_circles, CollisionContainer& out_collisions, CircleContainer& out_remainder) {
        for (const auto& a : all_circles) {
            bool collided = false;
            for (const auto& b : all_circles) {
                if (a != b && CircleVsCircle(a, b)) {
                    auto it = std::find_if(std::begin(out_collisions), std::end(out_collisions),
                        [&](std::pair<Circle, Circle> pair) {
                        return (a == pair.first && b == pair.second) || (a == pair.second && b == pair.first);
                    });
                    if (it == std::end(out_collisions)) {
                        out_collisions.emplace_back(a, b);
                    }
                    collided = true;
                }
            }
            if (!collided) {
                out_remainder.emplace_back(a);
            }
        }
    }

    void UpdateCircles(Grid<Value>& grid, const CircleContainer& container) {
        for (const auto& circle : container) {
            grid.SetCircle(circle);
        }
    }

    void UpdateRectangles(Grid<Value>& grid) {
        for (const auto& rectangle : rectangles) {
            grid.SetRectangle(rectangle);
        }
    }

    void AddCircle(const Circle& circle) {
        circles.emplace_back(circle);
    }

    void AddRectangle(const Rectangle& rectangle) {
        rectangles.emplace_back(rectangle);
    }

private:
    Grid<Value> relaxed_grid;
    CircleContainer circles;
    CircleContainer static_circles;
    std::vector<Rectangle> rectangles;
    int loops{ 0 };
    DataHandler data_handler;
};
