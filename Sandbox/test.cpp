// The main purpose of this file is to test specific aspects of the simulation

#include <iostream>
#include <numbers>
#include <cmath>
#include <iomanip> // Optional: for setting precision
using namespace std;
#include <cstdlib> // for rand() and srand()
#include "functiontests.h"
#include "../Grid.h"
#include "../Headers/V2.h"
#include "omp.h"
#include "../Headers/Defines.h"
#include "../Headers/Utilities.h"
#include "../Headers/Collisions.h"
#include "../Headers/DataHandler.h"
#include "../Grid.h"
#include <chrono>


int main() {
    // Record start time
    auto start = std::chrono::steady_clock::now();
    int test_var;
    LOG("Hello! Here we would like an input of either 1 2 or 3");
    std::cin >> test_var;
    LOG("Thanks! The variable you listed is " << test_var);
    auto time1 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(time1 - start);
    auto timepassed = time1 - start;
    LOG("Total time passed is: " << timepassed << " or in us: " << duration);
    LOG_("Now try typing in a different number ");
    std::cin >> test_var;
    LOG("Thanks! The variable you listed is " << test_var);

    time1 = std::chrono::steady_clock::now();
    timepassed = time1 - start;
    auto sec_int = std::chrono::duration_cast<std::chrono::seconds>(timepassed);
    LOG("Total time passed is: " << timepassed << " or in s: " << sec_int);
    std::cout << "Ran with no errors, press any key to quit: ";
    std::cin.ignore();
    std::cin.get();
    


    return 0;
}

