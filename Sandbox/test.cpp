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



int main() {
    int test_var;
    LOG("Hello! Here we would like an input of either 1 2 or 3");
    std::cin >> test_var;
    LOG("Thanks! The variable you listed is " << test_var);
    LOG_("Now try typing in a different number ");
    std::cin >> test_var;
    LOG("Thanks! The variable you listed is " << test_var);


    std::cout << "Ran with no errors";

    return 0;
}

