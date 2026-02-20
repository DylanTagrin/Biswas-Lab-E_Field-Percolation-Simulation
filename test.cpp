#include <iostream>
#include <numbers>
#include <iomanip> // Optional: for setting precision

int main() {
    // Use std::numbers::pi for a double precision value
    std::cout << std::setprecision(20) << "Pi (double): " << std::numbers::pi << std::endl;

    // Use a template for specific floating-point types, e.g., long double
    std::cout << std::setprecision(20) << "Pi (long double): " << std::numbers::pi_v<long double> << std::endl;

    return 0;
}