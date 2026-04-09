#pragma once
#include <iostream>
#include <type_traits>
#include <cmath>
#include <limits>
#include <utility>
#include "TypeTraits.h"

/* The main purpose of this header is to define the V2 class, this class creates an X and Y object 
that can be referenced and used like a 2D vector. This structure allows for 2D objects to be
stored more efficiently while also providing necessary logic functions.
Constructed with V2<T> where T is the type of object you want to create.*/


// 2D Vector Class Template intialized with V2<T> a{x, y} for object type <T>; Class contains 2D math
template <typename T>
struct V2 {
    // Default values and constructor for when a new class object is created
    T x{ 0 };
    T y{ 0 };
    V2() = default;
    // Allows V2<int> a;   V2<int> b = a; reasigns =
    V2(const V2& copy) = default;
    V2& operator=(const V2 & copy) = default;
    // Allow temporary objects like V2<int> a = V2<int>(1,2);
    V2(V2 && move) = default;
    V2& operator=(V2 && move) = default;
    // Constructor allowing V2<T> a{1, 2} to initialize x and y values
    V2(T x, T y) : x{ x }, y{ y } {
    }
    // Define -= and += operators
    inline V2& operator-=(const V2& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    inline V2& operator+=(const V2& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    // Define magnitude operators
    double MagnitudeSquared() {
        return x * x + y * y;
    }

    double Magnitude() {
        return std::sqrt(MagnitudeSquared());
    }

    // Define the bool operators == and !=
    bool operator==(const V2& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const V2& other) const {
        return !(*this == other);
    }

    // Use template so that V2 *= number works; same with /=
    template <typename U, is_number<U> = true, convertible<U, T> = true>
    V2& operator*=(U rhs) {
        x *= static_cast<T>(rhs);
        y *= static_cast<T>(rhs);
        return *this;
    }

    template <typename U, is_number<U> = true, convertible<U, T> = true>
    V2& operator/=(U rhs) {
        if (rhs) {
            x /= static_cast<T>(rhs);
            y /= static_cast<T>(rhs);
        } else {
            x = std::numeric_limits<T>::infinity();
            y = std::numeric_limits<T>::infinity();
        }
        return *this;
    }

    // Conversion operators between int, double, float, and unsigned int (useful in previous functions)
    operator V2<int>() const {
        return V2<int>{ static_cast<int>(x), static_cast<int>(y) };
    }
    operator V2<double>() const { return V2<double>{ static_cast<double>(x), static_cast<double>(y) }; }
    operator V2<float>() const { return V2<float>{ static_cast<float>(x), static_cast<float>(y) }; }
    operator V2<unsigned int>() const { return V2<unsigned int>{ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }; }

};

// Redefine << for V2
template <typename T>
std::ostream& operator<<(std::ostream& os, const V2<T>& obj) {
    os << "(" << obj.x << "," << obj.y << ")";
    return os;
}

// Define how rounding will work
template <typename T>
inline V2<int> Round(const V2<T>& v) {
    return { static_cast<int>(std::round(v.x)), static_cast<int>(std::round(v.y)) };
}

// Overwrite the + - * / operators; * and / there twice for lhs vs rhs
template <typename T>
inline V2<T> operator+(V2<T> lhs, const V2<T>& rhs) {
    lhs += rhs;
    return lhs;
}

template <typename T>
inline V2<T> operator-(V2<T> lhs, const V2<T>& rhs) {
    lhs -= rhs;
    return lhs;
}

template <typename A, typename B, is_number<A> = true>
V2<typename std::common_type<A, B>::type> operator*(A lhs, const V2<B>& rhs) {
    return { lhs * rhs.x, lhs * rhs.y };
}
template <typename A, typename B, is_number<A> = true, typename S = typename std::common_type<A, B>::type>
V2<S> operator/(A lhs, const V2<B>& rhs) {
    V2<S> vector;
    if (rhs.x) {
        vector.x = lhs / rhs.x;
    } else {
        vector.x = std::numeric_limits<S>::infinity();
    }
    if (rhs.y) {
        vector.y = lhs / rhs.y;
    } else {
        vector.y = std::numeric_limits<S>::infinity();
    }
    return vector;
}

template <typename A, typename B, is_number<B> = true>
V2<typename std::common_type<A, B>::type> operator*(const V2<A>& lhs, B rhs) {
    return { lhs.x * rhs, lhs.y * rhs };
}
template <typename A, typename B, is_number<B> = true, typename S = typename std::common_type<A, B>::type>
V2<S> operator/(const V2<A>& lhs, B rhs) {
    V2<S> vector;
    if (rhs) {
        vector.x = lhs.x / rhs;
        vector.y = lhs.y / rhs;
    } else {
        vector.x = std::numeric_limits<S>::infinity();
        vector.y = std::numeric_limits<S>::infinity();
    }
    return vector;
}


// Function that allows for V2 data to be hashed for an unordered list
namespace std {
// Hash function from https://ideone.com/tieHbd
// Custom hashing function for ecs::Entity class.
// This allows for use of unordered maps and sets with entities as keys.
    template <>
    struct hash<V2<int>> {
        std::size_t operator()(const V2<int>& k) const {
            // Hashing combination algorithm from:
            // https://stackoverflow.com/a/17017281
            std::size_t h = 17;
            h = h * 31 + std::hash<int>()(k.x);
            h = h * 31 + std::hash<int>()(k.y);
            return h;
        }
    };

} // namespace std