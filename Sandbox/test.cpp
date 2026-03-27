// The main purpose of this file is to test specific aspects of the simulation

#include <iostream>
#include <numbers>
#include <cmath>
#include <iomanip> // Optional: for setting precision
// #include <type_traits>
using namespace std;
#include <cstdlib> // for rand() and srand()
#include "functiontests.h"
#include "../TypeTraits.h" 



int main() {
   
    // Use a template for specific floating-point types, e.g., long double
    // std::cout << std::setprecision(20) << "Pi (long double): " << std::numbers::pi_v<long double> << std::endl;
    srand(20); // Set a fixed seed for reproducibility
    int random_value = rand() % 100; // Generate a random value between 0 and 99
    // double result = floor(2.1);
    auto pi = 3.14159;
    cout << "Random Value: " << random_value << endl;
    double latitude = 29.644203;
    double longitude = -82.34970;


    double test_var1 = 10;
    double new_var = squared(test_var1); 
    cout << "Test Var 1: " << test_var1 << endl;
    cout << "New Var: " << new_var << endl;
    
    is_number<decltype(test_var1)> statement = true;
    if (statement) {
        cout << "test_var1 is a number." << endl;
    } else {
        cout << "test_var1 is not a number." << endl;
    }



    return 0;
}

