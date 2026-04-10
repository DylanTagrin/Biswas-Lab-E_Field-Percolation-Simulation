#include "Simulation.h"
#include "Grid.h"

//#include "matplotlibcpp.h"
//namespace plt = matplotlibcpp;


int main() {
    int loops;
    int electrode_type;
    int frames; 

    // Short dialogue tree to get simulation inputs from the user
    LOG("Hello! Welcome to the Amlan Biswas Lab's Dielectrophoresis simulation.");
    LOG("First, please input some parameters regarding the simulation.");
    LOG("How many frames would you like to compute?");
    LOG_("Please input an integer: ");
    std::cin >> frames;
    LOG("Confirmation: You have input " << frames << " frames to compute.");
    LOG_("Input the max number of relaxation loops: "); 
    std::cin >> loops;
    LOG("Confirmation: You have input " << loops << " as the maximum amount of loops.");
    LOG("What shape Electrodes would you like to use? Please input an integer."); 
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

    // Initialize Simulation with grid size (800, 600)
    Simulation sim(V2<int>{ 800, 600 }, electrode_type);

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

    //sim.AddCircle(Circle({ 565, 300 }, 4));
    //auto test = sim.ComputeForceTest();
    //LOG(test[0]);

    //sim stability testing and repetition
    //also long runs
    // long runs 1-7
    /*
    sim.AddCircle(Circle({ 400, 460 }, 30));
    sim.AddCircle(Circle({ 400, 141 }, 30));
    sim.AddCircle(Circle({ 340, 300 }, 15));
    sim.AddCircle(Circle({ 399, 300 }, 15));
    sim.AddCircle(Circle({ 455, 301 }, 15));
    sim.AddCircle(Circle({ 360, 364 }, 25));
    sim.AddCircle(Circle({ 440, 366 }, 30));
    sim.AddCircle(Circle({ 359, 235 }, 30));
    sim.AddCircle(Circle({ 443, 234 }, 25));
    sim.AddCircle(Circle({ 320, 430 }, 30));
    sim.AddCircle(Circle({ 475, 430 }, 25));
    sim.AddCircle(Circle({ 320, 160 }, 25));
    sim.AddCircle(Circle({ 475, 160 }, 30));
    */
    // long run 8-13
    /*
    sim.AddCircle(Circle({ 358, 380 }, 29));
    sim.AddCircle(Circle({ 440, 390 }, 30));
    sim.AddCircle(Circle({ 360, 230 }, 30));
    sim.AddCircle(Circle({ 440, 210 }, 31));

    sim.AddCircle(Circle({ 330, 300 }, 15));
    sim.AddCircle(Circle({ 395, 300 }, 15));
    sim.AddCircle(Circle({ 455, 301 }, 15));

    sim.AddCircle(Circle({ 405, 460 }, 25));
    sim.AddCircle(Circle({ 395, 140 }, 25));

    sim.AddCircle(Circle({ 310, 160 }, 35));
    sim.AddCircle(Circle({ 500, 450 }, 35));

    sim.AddCircle(Circle({ 500, 150 }, 25));
    sim.AddCircle(Circle({ 300, 450 }, 25));
    */
    // long run 14-21

    /*sim.AddCircle(Circle({ 360, 300 }, 30));
    sim.AddCircle(Circle({ 440, 300 }, 31));

    sim.AddCircle(Circle({ 320, 385 }, 25));
    sim.AddCircle(Circle({ 400, 375 }, 30));
    sim.AddCircle(Circle({ 480, 385 }, 25));

    sim.AddCircle(Circle({ 320, 215 }, 25));
    sim.AddCircle(Circle({ 400, 225 }, 30));
    sim.AddCircle(Circle({ 480, 215 }, 25));

    sim.AddCircle(Circle({ 350, 450 }, 20));
    sim.AddCircle(Circle({ 450, 450 }, 20));
    sim.AddCircle(Circle({ 350, 150 }, 20));
    sim.AddCircle(Circle({ 450, 150 }, 20));

    sim.AddCircle(Circle({ 350, 500 }, 20));
    sim.AddCircle(Circle({ 450, 500 }, 20));
    sim.AddCircle(Circle({ 350, 100 }, 20));
    sim.AddCircle(Circle({ 450, 100 }, 20));

    sim.AddCircle(Circle({ 400, 470 }, 25));
    sim.AddCircle(Circle({ 400, 130 }, 25));*/

    //triangle testing
    /*
    sim.AddCircle(Circle({ 310, 400 }, 40));
    sim.AddCircle(Circle({ 380, 230 }, 30));
    sim.AddCircle(Circle({ 480, 180 }, 30));
    sim.AddCircle(Circle({ 480, 250 }, 20));
    sim.AddCircle(Circle({ 450, 410 }, 20));
    sim.AddCircle(Circle({ 370, 330 }, 40));
    sim.AddCircle(Circle({ 310, 180 }, 30));
    sim.AddCircle(Circle({ 280, 130 }, 20));
    sim.AddCircle(Circle({ 390, 450 }, 30));
    sim.AddCircle(Circle({ 385, 120 }, 30));
    sim.AddCircle(Circle({ 450, 290 }, 20));
    sim.AddCircle(Circle({ 290, 250 }, 20));
    */

    //tiny circles
    //sim.AddCircle(Circle({ 340, 360 }, 5));
    //sim.AddCircle(Circle({ 410, 350 }, 5));
    //sim.AddCircle(Circle({ 350, 290 }, 5));
    //sim.AddCircle(Circle({ 400, 295 }, 5));
    //sim.AddCircle(Circle({ 405, 220 }, 10));
    //sim.AddCircle(Circle({ 430, 250 }, 10));
    //sim.AddCircle(Circle({ 450, 255 }, 5));
    //sim.AddCircle(Circle({ 315, 390 }, 5));
    //sim.AddCircle(Circle({ 405, 300 }, 5));
    //sim.AddCircle(Circle({ 480, 215 }, 10));
    //sim.AddCircle(Circle({ 480, 265 }, 5));
    //sim.AddCircle(Circle({ 490, 300 }, 5));
    //sim.AddCircle(Circle({ 340, 215 }, 5));
    //sim.AddCircle(Circle({ 325, 390 }, 10));
    //sim.AddCircle(Circle({ 375, 345 }, 5));
    //sim.AddCircle(Circle({ 310, 210 }, 5));
    //sim.AddCircle(Circle({ 450, 240 }, 10));
    //sim.AddCircle(Circle({ 435, 385 }, 10));
    //sim.AddCircle(Circle({ 445, 330 }, 5));
    //sim.AddCircle(Circle({ 360, 260 }, 5));
    //end tiny circles

    // ellipse testing

    // usp poster sims
    // horizontal ellipse size: 32x24; vertical: 24x32; circle: 28x28
    
    /*sim.AddCircle(Circle({ 349, 299 }, 32, 24));
    sim.AddCircle(Circle({ 449, 299 }, 32, 24));
    
    sim.AddCircle(Circle({ 349, 374 }, 32, 24));
    sim.AddCircle(Circle({ 449, 224 }, 32, 24));
    sim.AddCircle(Circle({ 449, 374 }, 32, 24));
    sim.AddCircle(Circle({ 349, 224 }, 32, 24));*/

    /*sim.AddCircle(Circle({ 349, 299 }, 20, 28));
    sim.AddCircle(Circle({ 449, 299 }, 20, 28));
    sim.AddCircle(Circle({ 349, 369 }, 20, 28));
    sim.AddCircle(Circle({ 449, 369 }, 20, 28));
    sim.AddCircle(Circle({ 349, 229 }, 20, 28));
    sim.AddCircle(Circle({ 449, 229 }, 20, 28));
    sim.AddCircle(Circle({ 389, 199 }, 20, 28));
    sim.AddCircle(Circle({ 409, 399 }, 20, 28));
    sim.AddCircle(Circle({349, 444}, 20, 28));
    sim.AddCircle(Circle({ 449, 154 }, 20, 28));*/
/*
    sim.AddCircle(Circle({ 349, 299 }, 28, 28));
    sim.AddCircle(Circle({ 449, 299 }, 28, 28));
    sim.AddCircle(Circle({ 349, 369 }, 28, 28));
    sim.AddCircle(Circle({ 449, 369 }, 28, 28));
    sim.AddCircle(Circle({ 349, 229 }, 28, 28));
    sim.AddCircle(Circle({ 449, 229 }, 28, 28));
   */
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


    sim.Start(loops);


    
    auto static_frames = 1;
    for (auto i = 0; i < frames; i++) {
        LOG("Computing frame " << i);
        sim.Update(0.1);
        // check if junction closed (static circles on opposite electrodes collided; switch to static frame)
        if (sim.sim_stopped) {
            break;
        }
    }
        
    sim.StaticUpdate(static_frames);

    sim.Save("output/data.json");

    LOG("Done!");
    std::cin.ignore();
    std::cin.get();
}