#include "Simulation.h"
#include "Grid.h"

//#include "matplotlibcpp.h"
//namespace plt = matplotlibcpp;


int main() {
    int loops;
    int electrode_type;
    int frames; 
    double error;
    int min_loops;
    int check_every = 20;
    int mode = 0; 
    int numer_of_circles;
    int circle_a = 15;
    int circle_b = 15;
    unsigned int seed = 0;
    int use_gaussian = 0;


    // Short dialogue tree to get simulation inputs from the user
    LOG("Hello! Welcome to the Amlan Biswas Lab's Dielectrophoresis simulation.");
    LOG("First, please input some parameters regarding the simulation.");
    LOG("How many frames would you like to compute?");
    LOG_("Please input an integer: ");
    std::cin >> frames;
    LOG_("Input the max number of relaxation loops: "); 
    std::cin >> loops;
    LOG("What shape Electrodes would you like to use? Please input an integer. Note: Currently only the needle electrode works."); 
    LOG_("(0) for triangle, (1) for needle, (2) for rectangle: ");
    std::cin >> electrode_type;
    if (electrode_type == 0) {
        LOG("Confirmation: You have selected triangle electrodes.");
    }
    else if (electrode_type == 1) {
        LOG("Confirmation: You have selected needle electrodes.");
    }
    else if (electrode_type == 2) {
        LOG("Confirmation: You have selected rectangle electrodes.");
    }
    else {
        LOG("Invalid input, defaulting to triangle electrodes.");
        electrode_type = 0;
    }
    LOG_("What should the min loops per relaxation run be? ");
    std::cin >> min_loops;
    LOG_("What should the relative error we should check be? Recommended is 0.00001. Input a double: ");
    std::cin >> error;

    // Predefine simulation so that the intellisense is happy
    Simulation sim(V2<int>{ 800, 600 }, electrode_type);

    // Further note, electrodes must be placed first before the needle

    // WARNING: IF DIFFERENT SHAPE IS USED (I.E. NEEDLE VS TRIANGLE VS RECTANGLE), CODE WILL HAVE TO BE ADAPTED IN Simulation.h (and other
    // header files.
    // Examples: where left and right electrodes defined, use desired shape to instantiate new objects
    // If shape is new: copy paste functions and code blocks (such as InTriangle) and change the name (ik this is disgusting programming)

    // --------------------> note that first two points form straight edge of needle! <--------------
    
    // Add needles (electrodes) to the simulation with specified vertices and values
    sim.AddNeedle(Needle({ 0, 74 }, { 0, 524 }, { 299, 299 }, 50));
    sim.AddNeedle(Needle({ 799, 74 }, { 799, 524 }, { 499, 299 }, -50));
    
    //sim.AddTriangle(Triangle({ 0, 75 }, { 0, 525 }, { 325, 300 }, 50));
    //sim.AddTriangle(Triangle({ 475, 300 }, { 799, 75 }, { 799, 525 }, -50));

    //sim.AddRectangle(Rectangle({ 0, 150 }, { 250, 300 }, 50));
    //sim.AddRectangle(Rectangle({ 550, 150 }, { 250, 300 }, -50));



    // At this point we'll split between express mode simulation and custom imputs.
    LOG("would you like to run this simulation in express mode with preset parameters (0) or input your own parameters? (1)");
    LOG_("Please input 0 for express mode or 1 for custom parameters: ");
    std::cin >> mode;
    if (mode == 0) {
        LOG("You have selected express mode.");
    }
    else if (mode == 1) {
        LOG("You have selected custom parameters.");
        LOG("How many circles would you like to simulate?");
        LOG_("Input the number of circles to be simulated: ");
        std::cin >> numer_of_circles;
        LOG("Next, what should the a and b parameters of the circles be? (Input two integers)");
        LOG("Note: a is the horizontal radius and b is the vertical radius. Recommended is 15 for both.");
        LOG_("Input a and b: ");
        std::cin >> circle_a >> circle_b;
        LOG("What seed would you like to use for the random number generator for circle placement? Input an unsigned integer. Recommended is 0.");
        LOG_("Input seed: ");
        std::cin >> seed;
        LOG("Would you like to use a Gaussian distribution for circle placement instead of a uniform distribution? (0 for no, 1 for yes)");
        LOG_("Input 0 for no, 1 for yes: ");
        std::cin >> use_gaussian;
        sim.AddRandomCirclesSeeded(numer_of_circles, circle_a, circle_b, seed, use_gaussian);
        LOG("How often should the simulaiton check the relative error during relaxation?");
        LOG_("Input the number of loops between error checks: ");
        std::cin >> check_every;
    } 


    if (mode == 0) {
        sim.AddCircle(Circle({ 349, 309 }, 15, 15));
        sim.AddCircle(Circle({ 449, 294 }, 15, 15));
        sim.AddCircle(Circle({ 439, 334 }, 15, 15));
        sim.AddCircle(Circle({ 324, 359 }, 15, 15));
        sim.AddCircle(Circle({ 419, 269 }, 15, 15));
        sim.AddCircle(Circle({ 374, 374 }, 15, 15));
        sim.AddCircle(Circle({ 479, 259 }, 15, 15));
        sim.AddCircle(Circle({ 399, 314 }, 15, 15));
        sim.AddCircle(Circle({ 474, 339 }, 15, 15));
        sim.AddCircle(Circle({ 329, 269 }, 15, 15));
    }

    sim.Start(loops, error, min_loops, check_every);
    
    
    for (auto i = 0; i < frames; i++) {
        LOG("Computing frame " << i);
        sim.Update(0.1, error, min_loops, check_every);
        // check if junction closed (static circles on opposite electrodes collided; switch to static frame)
        if (sim.sim_stopped) {
            break;
        }
    }

    // auto static_frames = 1;  
    // sim.StaticUpdate(static_frames);

    sim.Save("output/data.json");

    LOG("Done!");
    std::cin.ignore();
    std::cin.get();
}