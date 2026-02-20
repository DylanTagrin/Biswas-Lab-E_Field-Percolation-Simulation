#pragma once
#include "V2.h"
#include "Circle.h"
#include "Rectangle.h"
#include "TypeTraits.h"

template <typename T>
class Grid {
public:
    Grid() = default;
    Grid(const V2<int>& size) : size{ size }, length{ size.x * size.y } { }

    T& operator[](std::size_t idx) {
        return grid[idx];
    }

    const T& operator[](std::size_t idx) const {
        return grid[idx];
    }

    template <typename T, is_number<T> = true>
    T operator[](std::size_t idx) const {
        return grid[idx];
    }

    T& operator[](const V2<int>& position) {
        return grid[size.x * position.y + position.x];
    }

    const T& operator[](const V2<int>& position) const {
        return grid[size.x * position.y + position.x];
    }

    template <typename T, is_number<T> = true>
    T operator[](const V2<int>& position) const {
        return grid[size.x * position.y + position.x];
    }

    void InitValue() {
        grid.resize(length, 0);
    }

    void InitVector() {
        grid.resize(length, {});
    }

    void SetRectangle(const Rectangle& rectangle) {
        for (auto i = rectangle.position.x; i < rectangle.position.x + rectangle.size.x; ++i) {
            for (auto j = rectangle.position.y; j < rectangle.position.y + rectangle.position.y; ++j) {
                grid[size.x * j + i] = rectangle.value;
            }
        }
    }

    void SetCircle(const Circle& circle) {
        for (auto i = circle.position.x - circle.radius; i < circle.position.x + circle.radius; ++i) {
            for (auto j = circle.position.y - circle.radius; j < circle.position.y + circle.radius; ++j) {
                V2<int> point{ i, j };
                if (PointVsCircle(point, circle)) {
                    grid[size.x * j + i] = circle.value;
                }
            }
        }
    }

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

    Grid<V2<Value>> GetGradient() const {
        Grid<V2<Value>> gradient{ size };
        gradient.InitVector();
        auto gradient_size = gradient.GetSize();
        for (auto x = 0; x < gradient_size.x; ++x) {

            auto next_x = x + 1;

            for (auto y = 0; y < gradient_size.y; ++y) {
                V2<Value> gradient_value;

                auto next_y = y + 1;
                auto index = y * size.x + x;
                auto next_x_index = index + 1;
                auto last_x_index = index - 1;
                auto next_y_index = index + size.x;
                auto last_y_index = index - size.x;

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
                } else if (next_y == gradient_size.y) {
                    gradient_value.y = grid[index] - grid[last_y_index];
                }
                gradient[index] = gradient_value;
            }
        }
        return gradient;
    }

    const V2<int>& GetSize() const {
        return size;
    }

    const int GetLength() const {
        return length;
    }

    const std::vector<T>& GetVector() const {
        return grid;
    }

private:
    const int length{ 0 };
    const V2<int> size;
    std::vector<T> grid;
};