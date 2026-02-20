#include "Simulation.h"

//#include "matplotlibcpp.h"
//namespace plt = matplotlibcpp;

// TO DO
// Make data file smaller

int main() {

    Simulation sim(V2<int>{ 800, 600 });

    sim.AddRectangle(Rectangle({ 0, 200 }, { 300, 200 }, 50));
    sim.AddRectangle(Rectangle({ 500, 200 }, { 300, 200 }, -50));

    //sim.AddCircle(Circle({ 565, 300 }, 4));
    //auto test = sim.ComputeForceTest();
    //LOG(test[0]);

    //sim.AddCircle(Circle({ 575, 300 }, 16));
    //sim.AddCircle(Circle({ 350, 275 }, 25));

    sim.AddCircle(Circle({ 340, 360 }, 5));
    sim.AddCircle(Circle({ 410, 350 }, 5));
    sim.AddCircle(Circle({ 350, 290 }, 5));
    sim.AddCircle(Circle({ 400, 295 }, 5));
    sim.AddCircle(Circle({ 405, 220 }, 10));
    sim.AddCircle(Circle({ 430, 250 }, 10));
    sim.AddCircle(Circle({ 440, 255 }, 5));
    sim.AddCircle(Circle({ 315, 390 }, 5));
    sim.AddCircle(Circle({ 405, 300 }, 10));
    sim.AddCircle(Circle({ 480, 215 }, 10));

    //sim.AddCircle(Circle({ 250, 300 }, 45));
    //sim.AddCircle(Circle({ 550, 300 }, 45));
    //sim.AddCircle(Circle({ 650, 300 }, 35));
    //sim.AddCircle(Circle({ 530, 300 }, 35));
    //sim.AddCircle(Circle({ 560, 320 }, 35));
    //sim.AddCircle(Circle({ 540, 250 }, 35));
    //sim.AddCircle(Circle({ 230, 350 }, 35));
    //sim.AddCircle(Circle({ 380, 370 }, 35));
    //sim.AddCircle(Circle({ 425, 200 }, 35));
    //sim.AddCircle(Circle({ 400, 210 }, 35));
    //sim.AddCircle(Circle({ 350, 210 }, 35));

    sim.Start(5000);


    auto frames = 30;
    for (auto i = 0; i < frames; i++) {
        LOG("Computing frame " << i);
        sim.Update();
    }

    sim.Save("data.json");

    LOG("Done!");

    std::cin.get();
}