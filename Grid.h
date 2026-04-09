#pragma once
#include "V2.h"
#include "Collisions.h"
#include "Circle.h"
#include "EllipseChain.h"
#include "Rectangle.h"
#include "Triangle.h"
#include "Needle.h"
#include "TypeTraits.h"

/*This file contains the framework for the grid class, a 2D voltage map with functions to paint
shapes onto it. Note: No unit implimentation*/

/* Grid class for a 2D electric potential map, technically a 1D array where access to the index for [x,y] is
grid[size.x * y + x].*/
template <typename T>
class Grid {
public:
    // Construct empty grid
    Grid() = default;
    // Construct grid with a size and split it between length's x and y with grid[size.x * y + x] for 1D indexing
    Grid(const V2<int>& size) : size{ size }, length{ size.x * size.y } { }

    // Redefines the [] operator for different object types and inputs (index and position)
    T& operator[](std::size_t idx) {
        return grid[idx];
    }
    const T& operator[](std::size_t idx) const {
        return grid[idx];
    }
    template <typename U, is_number<U> = true>
    U operator[](std::size_t idx) const {
        return grid[idx];
    }
    T& operator[](const V2<int>& position) {
        return grid[size.x * position.y + position.x];
    }
    const T& operator[](const V2<int>& position) const {
        return grid[size.x * position.y + position.x];
    }
    template <typename U, is_number<U> = true>
    U operator[](const V2<int>& position) const {
        return grid[size.x * position.y + position.x];
    }
    // Initiates either a type value to 0
    void InitValue() {
        grid.resize(length, 0);
    }
    // Initiates either a vector to empty
    void InitVector() {
        grid.resize(length, {});
    }
    // Sets grid values equal to rectangle.value 
    void SetRectangle(const Rectangle& rectangle) {
        for (auto i = rectangle.position.x; i < rectangle.position.x + rectangle.size.x; ++i) {
            for (auto j = rectangle.position.y; j < rectangle.position.y + rectangle.size.y; ++j) {
                grid[size.x * j + i] = rectangle.value;
            }
        }
    }
    // Sets grid values equal to triangle.value and fills triangle.points if not yet created
    void SetTriangle(const Triangle& triangle) {
        if(triangle.points.empty()) {
            for (int i = 0; i < size.x; ++i) {
                for (int j = 0; j < size.y; ++j) {
                    if (V2<int>Q = { i, j }; InTriangle(triangle, Q)) {
                        grid[size.x * j + i] = triangle.value;
                        triangle.points.push_back(Q);
                        if (OnTriangleEdge(triangle, Q)) {
                            triangle.edge_points.push_back(Q);
                        }
                        
                    }
                    
                }
            }
        }
        else {
            for (auto P : triangle.points) {
                grid[size.x * P.y + P.x] = triangle.value;
            }
        }
    }
    // Sets grid values equal to needle.value and fills needle.points if not yet created
    void SetNeedle(const Needle& needle) {
        auto v1 = needle.v1; auto v2 = needle.v2; auto v3 = needle.v3; auto cp = needle.cp;
        // define boundary
        if (needle.edge_points.empty()) {
            int steps = 2000;
            // #pragma omp parallel for
            for (int t_ = 0; t_ <= steps; t_++) {
                double t = static_cast<double>(t_) / steps;
                std::vector<V2<int>> verts = { v1, v2 };
                for (auto v : verts) {
                    V2<int> f1 = (1 - t) * (1 - t) * (v - cp);
                    V2<int> f2 = t * t * (v3 - cp);
                    V2<int> P = Round(cp + f1 + f2);
                    if (std::find(needle.edge_points.begin(), needle.edge_points.end(), P) == needle.edge_points.end()) {
                        needle.edge_points.emplace_back(P);
                        needle.points.emplace_back(P);
                        grid[size.x * P.y + P.x] = needle.value;
                    }
                }
                V2<int> P2 = Round(v1 + V2<int>(t * (v2 - v1)));
                if (std::find(needle.edge_points.begin(), needle.edge_points.end(), P2) == needle.edge_points.end()) {
                    needle.edge_points.emplace_back(P2);
                    needle.points.emplace_back(P2);
                    grid[size.x * P2.y + P2.x] = needle.value;
                }
            }
            //LOG("GAHOOK WE HAVE BOUNDARIES");
            /*for (auto it = needle.edge_points.begin(); it != needle.edge_points.end(); ++it) {
                std::cout << *it << " ";
            }*/
            //LOG(needle.edge_points);
        
            // define inside with flood fill
            //if (needle.points.empty()) {
            std::vector<V2<int>> queue = {};
            std::vector<V2<int>> queue_temp = {};
            V2<int> seed = Round(V2<double> ( (cp.x + v3.x) / 2, cp.y ));
            queue.emplace_back(seed);
            needle.points.emplace_back(seed);
            while (!queue.empty()) {
                for (auto& P : queue) {
                    // define new nodes and destroy old ones
                    std::vector<V2<int>> new_points = { P + V2<int>(0, 1), P + V2<int>(1, 0), P + V2<int>(-1, 0), P + V2<int>(0, -1) };
                    // add new nodes to array of interior points and the temporary queue
                    for (auto& Q : new_points) {
                        if (std::find(needle.points.begin(), needle.points.end(), Q) == needle.points.end()) {
                            needle.points.emplace_back(Q);
                            queue_temp.emplace_back(Q);
                            grid[size.x * Q.y + Q.x] = needle.value;
                        }
                    }
                }
                queue.clear();
                queue = queue_temp;
                //LOG(queue_temp);
                queue_temp.clear();
            }
            
            //LOG("gah00k");
            
            // start with node near center and add to array
            // node grows by 4-fold at max rate; add new nodes to array if they are not already there
            // 
        }
        else {
            for (auto P : needle.points) {
                grid[size.x * P.y + P.x] = needle.value;
            }
        }
    }
    // Create a rectangle area ab and if point in circle then set gridvalue = circle.value
    void SetCircle(const Circle& circle) {
        // essentially set up box of points around ellipse and see which ones inside (instead of iterating over entire grid)
        for (auto i = 0; i <= 2*circle.a; ++i) {
            auto p = i + circle.position.x - circle.a;
            for (auto j = 0; j <= 2*circle.b; ++j) {
                auto q = j + circle.position.y - circle.b;
                V2<int> point = { p, q };
                if (PointVsCircle(point, circle)) {
                    grid[size.x * q + p] = circle.value;
                }
            }
        }
    }
    // Lookup chain and set every circle point within the chain = chain.value
    void SetChain(const Chain& chain) {
        for (const Circle& circle : chain.group) {
            for (auto i = 0; i <= 2 * circle.a; ++i) {
                auto p = i + circle.position.x - circle.a;
                for (auto j = 0; j <= 2 * circle.b; ++j) {
                    auto q = j + circle.position.y - circle.b;
                    V2<int> point = { p, q };
                    if (PointVsCircle(point, circle)) {
                        grid[size.x * q + p] = chain.value;
                    }
                }
            }
        }
    }
    // Prints out the grid into terminal 
    void PrintGrid() const {
        LOG("------------------------");
        for (auto i = 0; i < size.x; ++i) {
            for (auto j = 0; j < size.y; ++j) {
                LOG_(grid[size.x * j + i]);
                if (j != size.y) {
                    LOG_(",");
                }
            }
            LOG("");
        }
        LOG("------------------------");
    }
    // Converts potential from grid[x,y] = number into gradient gradient[x,y] = (dx, dy)
    Grid<V2<Value>> GetGradient() const {
        // Create output
        Grid<V2<Value>> gradient{ size };
        gradient.InitVector();
        auto gradient_size = gradient.GetSize();
        // Loop through every grid point
        for (auto x = 0; x < gradient_size.x; ++x) {
            auto next_x = x + 1;
            for (auto y = 0; y < gradient_size.y; ++y) {
                V2<Value> gradient_value;
                auto next_y = y + 1;

                // Grab index of all adjacent points
                auto index = y * size.x + x;
                auto next_x_index = index + 1;
                auto last_x_index = index - 1;
                auto next_y_index = index + size.x;
                auto last_y_index = index - size.x;
                // Use central difference method to calc the gradient (if statements for boundry)
                if (x > 0 && next_x < gradient_size.x) {
                    gradient_value.x = (grid[next_x_index] - grid[last_x_index]) / static_cast<Value>(2.0);
                } else if (x == 0) {
                    gradient_value.x = grid[next_x_index] - grid[index];
                } else if (next_x == gradient_size.x) {
                    gradient_value.x = grid[index] - grid[last_x_index];
                }
                if (y > 0 && next_y < gradient_size.y) {
                    gradient_value.y = (grid[next_y_index] - grid[last_y_index]) / static_cast<Value>(2.0);
                } else if (y == 0) {
                    gradient_value.y = grid[next_y_index] - grid[index];
                } else if (next_y == gradient_size.y - 1) {
                    gradient_value.y = grid[index] - grid[last_y_index];
                }
                gradient[index] = gradient_value;
            }
        }
        return gradient;
    }
    // returns the grid dimensions (width, height) as a V2<int>
    const V2<int>& GetSize() const {
        return size;
    }
    // Returns the total number of grid cells: length = size.x * size.y
    const int GetLength() const {
        return length;
    }
    // Returns the underlying flattened 1D vector storing the grid contents. 
    const std::vector<T>& GetVector() const {
        return grid;
    }

    /*struct Node {
        Node(const V2<int> pos, bool condition, )
    };*/

private:
    const int length{ 0 };
    const V2<int> size;
    std::vector<T> grid;
};