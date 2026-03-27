#pragma once
#include <type_traits>

// Type traits for compile-time checks, used extensively in V2.h for operator overloading and ensuring type safety
// Enable if T is an arithmetic type (integral or floating-point)
template <typename T>
using is_number = std::enable_if_t<std::is_arithmetic_v<T>, bool>;
// Enable if From is convertible to To
template <typename From, typename To>
using convertible = std::enable_if_t<std::is_convertible_v<From, To>, bool>;