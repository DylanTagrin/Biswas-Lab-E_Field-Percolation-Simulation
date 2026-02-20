#pragma once
#include <type_traits>

template <typename T>
using is_number = std::enable_if_t<std::is_arithmetic_v<T>, bool>;
template <typename From, typename To>
using convertible = std::enable_if_t<std::is_convertible_v<From, To>, bool>;