#pragma once
#include <cassert>
#include <cmath>
#include <functional>
#include <limits>
#include <type_traits>

//#include "gsl/gsl_integration.h"
//#include "gsl/gsl_sf.h"
//#include "gsl/gsl_math.h"
#include "omp.h"
#include "Headers/Defines.h"
#include "Headers/Utilities.h"
#include "Headers/Collisions.h"
#include "Headers/DataHandler.h"
#include "Grid.h"

/*This file is the main simulation engine and has a lot going on with it.*/

/* Simulation constructor with a V2 gridsize input. 
    Contains data_handler for width, height, timestep, data, and measurements. 
    Uses "relaxed grid" for holding the potential, initiated at 0.
    NOTE: Electrode = 0 for triangle, 1 for needle, 2 for rectangle*/
class Simulation {
public:
    V2<int> glob_grid_size;
    bool sim_stopped = false;
    std::vector<Value> lf_data;
    /* Simulation constructor with a V2 gridsize input. 
    Contains data_handler for width, height, timestep, data, and measurements. 
    Uses "relaxed grid" for holding the potential, initiated at 0.
    NOTE: Electrode = 0 for triangle, 1 for needle, 2 for rectangle*/
    Simulation(const V2<int>& grid_size, int electrode) : relaxed_grid{ grid_size }, electrode {electrode}, 
        data_handler{   // Define some parameters that will be stored in the json
            Data<int>{ "width", grid_size.x },
            Data<int>{ "height", grid_size.y },
            Data<int>{ "timestep", 1 },
            Data<std::vector<std::vector<Value>>>{ "data", std::vector<std::vector<Value>>{} },
            Data<std::vector<Value>>{ "measurements", std::vector<Value>{} }
        } {
        relaxed_grid.InitValue();
    }
    // Initiates the simulation, starting by painting in the electrodes and relaxes the grid for N relaxation_loops
    void Start(int relaxation_loops) {
        loops = relaxation_loops;   // Redefines loops just in case
        // Assigns stored voltages on the electrodes to the grid
        if (electrode == 0) {
            UpdateTriangles(relaxed_grid);
        } else if (electrode == 1) {
            UpdateNeedles(relaxed_grid);
        } else if (electrode == 2){
            UpdateRectangles(relaxed_grid);
        }
        Relax(relaxed_grid, loops, false);
        LOG("Total Circle Area:" << CheckAreaTest());
    }
    // Saves the data_handler data to file_path as a json
    void Save(const char* file_path) {
        data_handler.SaveToFile(file_path);
    }
    /* Main driver for frame step. Copies potential grid,  */
    void Update(float dt) {
        // Make copy of potential grid
        auto circle_grid = relaxed_grid;
        // (important for steps after the first) set the electric potential of the circle equal to the grid value at the enter
        for (auto& circle : circles) {
            circle.value = circle_grid[circle.position];
        }
        // Set the grid's potential values equal to the circle's at select points
        UpdateCircles(circle_grid, circles);
        for (auto& static_circle : static_circles) {
            LOG("Static Circle Value:" << static_circle.value);
        }
        // Sets the grid's potential values equal to the STATIC circle's circles, or electrodes
        UpdateCircles(circle_grid, static_circles);
        // Performs potential relaxation loops
        Relax(circle_grid, loops, true);
        // With new potential calculate the electric field
        auto electric_fields = ComputeElectricField(circle_grid);
        // Save electric field data for later
        lf_data = electric_fields.first.GetVector();
        data_handler.Add("data", lf_data);
        // // Redundant:Find out the range of supplied voltages and voltage difference, note only works for 2 electrodes
        // std::vector<Value> voltage_range;
        // if (electrode == 0) {
        //     voltage_range = { triangles[0].value, triangles[1].value };
        // }
        // else if (electrode == 1) {
        //     voltage_range = { needles[0].value, needles[1].value };
        // }
        // auto voltage = voltage_range[1] - voltage_range[0];
        glob_grid_size = electric_fields.second.GetSize();
        data_handler.Add("measurements", Measure(glob_grid_size, electric_fields.second));
        UpdatePhysics(electric_fields.second, dt);
        //UpdateCollisions();   DISABLED FOR CHAINS
        LOG("Total Circle Area:" << CheckAreaTest());
    }

    void StaticUpdate(int frames) {
        auto electric_fields = ComputeElectricField(relaxed_grid);
        if (frames == 0) {
            return;
        }
        std::vector<int> voltage_range;
        for (auto& tri : triangles) {
            voltage_range.emplace_back(tri.value);
        }
        for (auto& needle : needles) {
            voltage_range.emplace_back(needle.value);
        }
        auto voltage = voltage_range[1] - voltage_range[0];
        auto interval = voltage / frames;
        for (auto i = 0; i < frames; i++) {
            LOG("Computing static frame " << i);
            voltage_range[0] += interval;
            voltage_range[1] -= interval;
            voltage = voltage_range[1] - voltage_range[0];
            data_handler.Add("data", lf_data);
            data_handler.Add("measurements", Measure(glob_grid_size, electric_fields.second));
        }
    }

    void UpdatePhysics(const Grid<Value>& electric_field_squared, float dt) {
        auto grid_size = electric_field_squared.GetSize();
        Needle left_elec({ 0, 0 }, { 0, 0 }, { 0, 0 }, 0); Needle right_elec({ 0, 0 }, { 0, 0 }, { 0, 0 }, 0);
        /*
        for (auto& rect : rectangles) {
            if (OnLeftSide(rect.position, grid_size)) {
                left_elec = rect;
            }
            else {
                right_elec = rect;
            }
        }
        */
        /*if (!triangles.empty()) {
            Triangle left_elec({ 0, 0 }, { 0, 0 }, { 0, 0 }, 0); Triangle right_elec({ 0, 0 }, { 0, 0 }, { 0, 0 }, 0);
            for (auto& tri : triangles) {
                if (OnLeftSide(tri.v1, grid_size)) {
                    left_elec = tri;
                }
                else {
                    right_elec = tri;
                }
            }
        }
        if (!needles.empty()) {
            Needle left_elec({ 0, 0 }, { 0, 0 }, { 0, 0 }, 0); Needle right_elec({ 0, 0 }, { 0, 0 }, { 0, 0 }, 0);
            for (auto& needle : needles) {
                if (OnLeftSide(needle.v1, grid_size)) {
                    left_elec = needle;
                }
                else {
                    right_elec = needle;
                }
            }
        }*/
        
        // allow static circles to migrate on rectangular electrodes
        /*
        for (auto& static_circle : static_circles) {
            //identify if circle is on left side or right side of grid
            auto old_position = static_circle.position;
            bool ls = OnLeftSide(old_position, grid_size);
            auto force = ComputeForceOnCircle(electric_field_squared, static_circle);
            LOG("force on static circle: " << force);
            if (ls == true) {
                //determine if circle is on side, top, bottom, or corner of electrode
                auto LE = left_elec;
                //check vertices
                if (old_position == LE.position + V2<int>(LE.size.x, 0) || old_position == LE.position + LE.size) {
                    //if y component of force >= x component, constrain movement in x direction; otherwise, constrain in y direction
                    if ((force.y >= 0 && force.y >= force.x) || (force.y < 0 && -1 * force.y >= force.x)) {
                        static_circle.acceleration = { 0, 1.0f * force.y };
                        static_circle.velocity = { 0, 1.0f * force.y };
                        static_circle.position += Round(static_circle.velocity);
                        if (static_circle.position.y < LE.position.y) {
                            static_circle.position.y = LE.position.y;
                        }
                        else if (static_circle.position.y > LE.position.y + LE.size.y) {
                            static_circle.position.y = LE.position.y + LE.size.y;
                        }
                    }
                    else {
                        static_circle.acceleration = { 1.0f * force.x, 0 };
                        static_circle.velocity = { 1.0f * force.x, 0 };
                        static_circle.position += Round(static_circle.velocity);
                        if (static_circle.position.x <= LE.position.x) {
                            //ensure circle does not sit on grid border by adding 1 to x-coordinate
                            static_circle.position.x = LE.position.x + 1;
                        }
                        else if (static_circle.position.x > LE.position.x + LE.size.x) {
                            static_circle.position.x = LE.position.x + LE.size.x;
                        }
                    }
                }
                //check side
                else if (old_position.x == LE.position.x + LE.size.x) {
                    static_circle.acceleration = { 0, 1.0f * force.y };
                    static_circle.velocity = { 0, 1.0f * force.y };
                    static_circle.position += Round(static_circle.velocity);
                    if (static_circle.position.y < LE.position.y) {
                        static_circle.position.y = LE.position.y;
                    }
                    else if (static_circle.position.y > LE.position.y + LE.size.y) {
                        static_circle.position.y = LE.position.y + LE.size.y;
                    }
                }
                //only other possible condition is top or bottom
                else {
                    static_circle.acceleration = { 1.0f * force.x, 0 };
                    static_circle.velocity = { 1.0f * force.x, 0 };
                    static_circle.position += Round(static_circle.velocity);
                    if (static_circle.position.x <= LE.position.x) {
                        //ensure circle does not sit on grid border by adding 1 to x-coordinate
                        static_circle.position.x = LE.position.x + 1;
                    }
                    else if (static_circle.position.x > LE.position.x + LE.size.x) {
                        static_circle.position.x = LE.position.x + LE.size.x;
                    }
                }
            }
            //if static circle on right side
            else {
                //determine if circle is on side, top, bottom, or corner of electrode
                auto RE = right_elec;
                //check vertices
                if (old_position == RE.position || old_position == RE.position + V2<int>(0, RE.size.y)) {
                    //if y component of force >= x component, constrain movement in x direction; otherwise, constrain in y direction
                    if ((force.y >= 0 && force.y >= force.x) || (force.y < 0 && -1 * force.y >= force.x)) {
                        static_circle.acceleration = { 0, 1.0f * force.y };
                        static_circle.velocity = { 0, 1.0f * force.y };
                        static_circle.position += Round(static_circle.velocity);
                        if (static_circle.position.y < RE.position.y) {
                            static_circle.position.y = RE.position.y;
                        }
                        else if (static_circle.position.y > RE.position.y + RE.size.y) {
                            static_circle.position.y = RE.position.y + RE.size.y;
                        }
                    }
                    else {
                        static_circle.acceleration = { 1.0f * force.x, 0 };
                        static_circle.velocity = { 1.0f * force.x, 0 };
                        static_circle.position += Round(static_circle.velocity);
                        if (static_circle.position.x >= RE.position.x + RE.size.x) {
                            //ensure circle does not sit on grid border by subtracting 1 from x-coordinate
                            static_circle.position.x = RE.position.x + RE.size.x - 1;
                        }
                        else if (static_circle.position.x < RE.position.x) {
                            static_circle.position.x = RE.position.x;
                        }
                    }
                }
                //check side
                else if (old_position.x == RE.position.x) {
                    static_circle.acceleration = { 0, 1.0f * force.y };
                    static_circle.velocity = { 0, 1.0f * force.y };
                    static_circle.position += Round(static_circle.velocity);
                    if (static_circle.position.y < RE.position.y) {
                        static_circle.position.y = RE.position.y;
                    }
                    else if (static_circle.position.y > RE.position.y + RE.size.y) {
                        static_circle.position.y = RE.position.y + RE.size.y;
                    }
                }
                //only other possible condition is top or bottom
                else {
                    static_circle.acceleration = { 1.0f * force.x, 0 };
                    static_circle.velocity = { 1.0f * force.x, 0 };
                    static_circle.position += Round(static_circle.velocity);
                    if (static_circle.position.x >= RE.position.x + RE.size.x) {
                        //ensure circle does not sit on grid border by subtracting 1 from x-coordinate
                        static_circle.position.x = RE.position.x + RE.size.x - 1;
                    }
                    else if (static_circle.position.x < RE.position.x) {
                        static_circle.position.x = RE.position.x;
                    }
                }
            }
            LOG("Static circle position: " << static_circle.position);
        }
        */

        // allow static circles to migrate on triangular/needle electrodes {TEMPORARILY DISABLED TO TEST CHAINS}

        //for (auto& static_circle : static_circles) {
        //    auto old_position = static_circle.position;
        //    bool onLS = OnLeftSide(old_position, grid_size);
        //    auto force = ComputeForceOnCircle(electric_field_squared, static_circle);
        //    LOG("force on static circle: " << force);
        //    static_circle.acceleration = { 1.0f * force.x, 1.0f * force.y };
        //    static_circle.velocity = { 1.0f * force.x, 1.0f * force.y };
        //    static_circle.position += Round(dt * static_circle.velocity);
        //    // ensure that circles move even for small, persistent forces
        //    if (dt * abs(static_circle.velocity.x) < 5) {
        //        static_circle.pos_leftover.x += (dt * static_circle.velocity.x) - round(dt * static_circle.velocity.x);
        //    }
        //    if (dt * abs(static_circle.velocity.y) < 5) {
        //        static_circle.pos_leftover.y += (dt * static_circle.velocity.y) - round(dt * static_circle.velocity.y);
        //    }
        //    // prevent circle from exiting grid margin by putting it back where it was >:D (or let it move a little bit)
        //    auto origin = V2<int>{ 0, 0 };
        //    if (!InRectangle(static_circle.position, origin, grid_size)) {
        //        auto m = Slope(old_position, static_circle.position);
        //        if (force.x <= 0) {
        //            static_circle.position = Round(old_position + V2<int>(-10 * cos(atan(m)), -10 * sin(atan(m))));
        //        }
        //        else if (force.x > 0) {
        //            static_circle.position = Round(old_position + V2<int>(10 * cos(atan(m)), 10 * sin(atan(m))));
        //        }
        //        else {
        //            static_circle.position = old_position;
        //        }
        //    }

        //    if (abs(static_circle.pos_leftover.x) >= 1 && abs(static_circle.pos_leftover.y) >= 1) {
        //        // time scale dt implicitly included in pos_leftover so it is not needed here
        //        static_circle.position += Round(static_circle.pos_leftover);
        //        static_circle.pos_leftover -= Round(static_circle.pos_leftover);
        //    }
        //    // prevent electrode hopping by keeping static circle on left or right electrode
        //    /*if (onLS) {
        //        static_circle.position = ClosestPointOnTriangle(static_circle, left_elec);
        //    }
        //    else {
        //        static_circle.position = ClosestPointOnTriangle(static_circle, right_elec);
        //    }*/

        //    // needle electrodes
        //    for (auto& needle : needles) {
        //        if (static_circle.value == needle.value) {
        //            static_circle.position = ClosestPointOnNeedle(static_circle, needle);
        //        }
        //        else {
        //            continue;
        //        }
        //    }
        //    /*
        //    if (onLS) {
        //        static_circle.position = ClosestPointOnNeedle(static_circle, left_elec);
        //    }
        //    else {
        //        static_circle.position = ClosestPointOnNeedle(static_circle, right_elec);
        //    }
        //    */
        //    // in case circle tries slipping off grid after being moved back to electrode border
        //    if (!InRectangle(static_circle.position, origin, grid_size)) {
        //        static_circle.position = old_position;
        //    }
        //    LOG("Old static circle position" << old_position << " ----> New: " << static_circle.position);
        //}
       
        //CircleContainer moving_circles;
        //ChainContainer moving_chains;
        for (auto& circle : circles) {
            circle.pos_old = circle.position;
            auto force = ComputeForceOnCircle(electric_field_squared, circle);
            circle.acceleration = { 1.0f * force.x, 1.0f * force.y };
            LOG("Force on circle: " << force.x << "," << force.y);
            circle.velocity = { 1.0f * force.x, 1.0f * force.y };
            // ensure that circles move even for small, persistent forces and improve accuracy for small forces
            if (dt * abs(circle.velocity.x) < 5) {
                circle.pos_leftover.x += (dt * circle.velocity.x) - trunc(dt * circle.velocity.x);
            }
            if (dt * abs(circle.velocity.y) < 5) {
                circle.pos_leftover.y += (dt * circle.velocity.y) - trunc(dt * circle.velocity.y);
            }
            //moving_circles.emplace_back(circle);
        }

        for (auto& chain : chains) {
            chain.pos_old = chain.position;
            V2<float> net_force = { 0,0 };
            for (auto& member : chain.group) {
                net_force += ComputeForceOnCircle(electric_field_squared, member);
            }
            LOG("Force on chain: " << net_force.x << "," << net_force.y);
            chain.velocity = { 1.0f * net_force.x, 1.0f * net_force.y };
            // ensure that chains move even for small, persistent forces and improve accuracy for small forces
            if (dt * abs(chain.velocity.x) < 5) {
                chain.pos_leftover.x += (dt * chain.velocity.x) - trunc(dt * chain.velocity.x);
            }
            if (dt * abs(chain.velocity.y) < 5) {
                chain.pos_leftover.y += (dt * chain.velocity.y) - trunc(dt * chain.velocity.y);
            }
            //moving_chains.emplace_back(chain);
        }

        int ticks = 200;
        for (int i = 0; i <= ticks; i++) {
            CircleContainer circles_remove;
            CircleContainer static_circles_remove;
            ChainContainer chains_remove;
            ChainContainer static_chains_remove;

            auto i_ = static_cast<float>(i);    // ensures i/[number] will yield float always
            for (auto& circle : circles) {
                V2<float> it = dt * circle.velocity * i_ / ticks;

                // if circle tries to move outside grid margin (50 pixels from border), move only a little from its original position
                // (for large forces --> large gradient, circle probably about to collide with something)
                /*
                auto origin = V2<int>{ 0, 0 };
                if (!InRectangle(circle.position, origin, grid_size)) {
                    auto m = Slope(circle.pos_old, circle.position);
                    if (circle.velocity.x <= 0) {
                        circle.position = Round(circle.pos_old + V2<int>(-10 * cos(atan(m)), -10 * sin(atan(m))));
                    }
                    else if (circle.velocity.x > 0) {
                        circle.position = Round(circle.pos_old + V2<int>(10 * cos(atan(m)), 10 * sin(atan(m))));
                    }
                    else {
                        circle.position = circle.pos_old;
                    }
                }*/

                //// check if circle in grid; if not, move to near border
                //if (!InRectangle(circle.position, origin, grid_size)) {
                //    float slope = Slope(old_position, circle.position);
                //    // determine if going to the left or right half of grid
                //    if (OnLeftSide(circle.position, grid_size)) {
                //        int dy = slope * (1 - old_position.x);
                //        int y_new = dy + old_position.y;
                //        if (y_new < 1) {
                //            int dx = (1 - old_position.y) / slope;
                //            circle.position = { dx + old_position.x, 1 };
                //        }
                //        else if (y_new > (grid_size.y - 1)) {
                //            int dx = (grid_size.y - 1 - old_position.y) / slope;
                //            circle.position = { dx + old_position.x, grid_size.y - 1 };
                //        }
                //        else {
                //            circle.position = { 1, y_new };
                //        }
                //    }
                //    else {
                //        int dy = slope * (grid_size.x - 1 - old_position.x);
                //        int y_new = dy + old_position.y;
                //        if (y_new <= 0) {
                //            int dx = (1 - old_position.y) / slope;
                //            circle.position = { dx + old_position.x, 1 };
                //        }
                //        else if (y_new >= grid_size.y) {
                //            int dx = (grid_size.y - 1 - old_position.y) / slope;
                //            circle.position = { dx + old_position.x, grid_size.y - 1 };
                //        }
                //        else {
                //            circle.position = { grid_size.x - 1, y_new };
                //        }
                //    }
                //}
                // check if new position inside electrode; if so, move to near electrode boundary so collision is registered
                /*
                for (auto& rect : rectangles) {
                    if (InRectangle(circle.position, rect.position, rect.size) ) {
                        // determine if left or right electrode
                        if (rect.position == left_elec.position) {
                            circle.position = FirstIntersection(old_position, circle.position, rect.position, rect.size);
                        }
                        else {
                            circle.position = FirstIntersection(old_position, circle.position, rect.position, rect.size);
                        }
                    }
                }

                // if circle in triangle, it was probably about to collide with either the electrode or another circle
                for (auto& tri : triangles) {
                    if (InTriangle(tri, circle.position) && !OnTriangleEdge(tri, circle.position)) {
                        auto m = Slope(old_position, circle.position);
                        if (force.x <= 0) {
                            circle.position = Round(old_position + V2<int>(-10 * cos(atan(m)), - 10 * sin(atan(m))));
                        }
                        else if (force.x > 0) {
                            circle.position = Round(old_position + V2<int>(10 * cos(atan(m)), 10 * sin(atan(m))));
                        }
                        else {
                            circle.position = old_position;
                        }
                    }
                }

                // if circle in needle, it was probably about to collide with either the electrode or another circle
                for (auto& needle : needles) {
                    if (InNeedle(needle, circle.position) && !OnNeedleEdge(needle, circle.position)) {
                        auto m = Slope(old_position, circle.position);
                        if (force.x <= 0) {
                            circle.position = Round(old_position + V2<int>(-10 * cos(atan(m)), -10 * sin(atan(m))));
                        }
                        else if (force.x > 0) {
                            circle.position = Round(old_position + V2<int>(10 * cos(atan(m)), 10 * sin(atan(m))));
                        }
                        else {
                            // condition to prevent crashing, but force should always be either <=0 or >0
                            circle.position = old_position;
                        }
                    }
                }
                */
                // check if leftover motion >= 1 at end of frame
                if (i_ == ticks) {
                    if (abs(circle.pos_leftover.x) >= 1) {
                        it.x += trunc(circle.pos_leftover.x);
                        circle.pos_leftover.x -= trunc(circle.pos_leftover.x);
                    }
                    if (abs(circle.pos_leftover.y) >= 1) {
                        it.y += trunc(circle.pos_leftover.y);
                        circle.pos_leftover.y -= trunc(circle.pos_leftover.y);
                    }
                }
                
                circle.position = Round(static_cast<V2<float>>(circle.pos_old) + it);
            }

            for (auto& chain : chains) {
                V2<float> it = dt * chain.velocity * i_ / ticks;
                // check if leftover motion >= 1 at end of frame
                if (i_ == ticks) {
                    if (abs(chain.pos_leftover.x) >= 1) {
                        it.x += trunc(chain.pos_leftover.x);
                        chain.pos_leftover.x -= trunc(chain.pos_leftover.x);
                    }
                    if (abs(chain.pos_leftover.y) >= 1) {
                        it.y += trunc(chain.pos_leftover.y);
                        chain.pos_leftover.y -= trunc(chain.pos_leftover.y);
                    }
                }

                chain.position = Round(static_cast<V2<float>>(chain.pos_old) + it);
                chain.UpdateChain();
                //LOG("Chain position: " << chain.position);
                //LOG("Chain velocity: " << chain.velocity);
            }

            // Check for interframe collisions
            for (auto& circle : circles) {
                auto perim_A = circle.GetPerimeterPoints();
                // chain method
                for (auto& circle2 : circles) {
                    auto perim_B = circle2.GetPerimeterPoints();
                    if (EdgeIntersect(perim_A, perim_B) && circle2 != circle) {
                        /*if (circle2.value != 0) {
                            circle.value = circle2.value;
                        }*/
                        // 
                        // add new chain
                        CircleContainer group = { circle, circle2 };
                        std::vector<V2<int>> positions = { {0,0}, circle2.position - circle.position };
                        auto new_chain = Chain{ circle.position, group, positions };
                        new_chain.value = circle.value;
                        new_chain.pos_old = new_chain.position;
                        new_chain.UpdateChain();
                        chains.emplace_back(new_chain);
                        LOG("New chain (" << new_chain.group.size() << " members) formed at: " << new_chain.position);
                        // mark circles for removal
                        circles_remove.emplace_back(circle);
                        circles_remove.emplace_back(circle2);
                        break;
                    }
                }
                if (!circles_remove.empty()) {
                    for (auto& circle : circles_remove) {
                        circles.erase(std::remove(circles.begin(), circles.end(), circle), circles.end());
                    }
                    circles_remove.clear();
                    break;
                }

                for (auto& chain : chains) {
                    auto perim_B = chain.GetPerimeterPoints();
                    if (EdgeIntersect(perim_A, perim_B)) {
                        /*if (chain.value != 0) {
                            circle.value = chain.value;
                        }*/
                        // add circle to chain
                        chain.group.emplace_back(circle);
                        chain.rel_positions.emplace_back(circle.position - chain.position);
                        chain.velocity = { 0,0 };
                        chain.UpdateChain();
                        LOG("Chain collision (" << chain.group.size() << " members) at: " << chain.position);

                        // mark circle for removal
                        circles_remove.emplace_back(circle);
                        //chains_remove.emplace_back(chain);
                        break;
                    }
                }
                if (!circles_remove.empty()) {
                    for (auto& circle : circles_remove) {
                        circles.erase(std::remove(circles.begin(), circles.end(), circle), circles.end());
                    }
                    circles_remove.clear();
                    break;
                }

                for (auto& circle2 : static_circles) {
                    auto perim_B = circle2.GetPerimeterPoints();
                    if (EdgeIntersect(perim_A, perim_B)) {
                        //circles_remove.emplace_back(circle);
                        //circles_static_remove.emplace_back(circle2);

                        // create new chain with static circle at root
                        CircleContainer group = { circle2, circle };
                        std::vector<V2<int>> positions = { {0,0}, circle.position - circle2.position };
                        auto new_chain = Chain{ circle.position, group, positions };
                        new_chain.value = circle2.value;
                        new_chain.pos_old = new_chain.position;
                        new_chain.UpdateChain();
                        static_chains.emplace_back(new_chain);
                        LOG("New static chain (" << new_chain.group.size() << " members) formed at: " << new_chain.position);

                        // mark old circles for removal
                        static_circles_remove.emplace_back(circle2);
                        circles_remove.emplace_back(circle);
                        break;
                    }
                }
                if (!circles_remove.empty()) {
                    for (auto& circle : circles_remove) {
                        circles.erase(std::remove(circles.begin(), circles.end(), circle), circles.end());
                    }
                    for (auto& static_circle : static_circles_remove) {
                        static_circles.erase(std::remove(static_circles.begin(), static_circles.end(), circle), static_circles.end());
                    }
                    circles_remove.clear();
                    static_circles_remove.clear();
                    break;
                }

                for (auto& chain : static_chains) {
                    auto perim_B = chain.GetPerimeterPoints();
                    if (EdgeIntersect(perim_A, perim_B)) {
                        /*if (chain.value != 0) {
                            circle.value = chain.value;
                        }*/
                        // add circle to chain
                        chain.group.emplace_back(circle);
                        chain.rel_positions.emplace_back(circle.position - chain.position);
                        chain.UpdateChain();
                        LOG("Chain collision (" << chain.group.size() << " members) at: " << chain.position);

                        // mark circle for removal
                        circles_remove.emplace_back(circle);
                    }
                }
                if (!circles_remove.empty()) {
                    for (auto& circle : circles_remove) {
                        circles.erase(std::remove(circles.begin(), circles.end(), circle), circles.end());
                    }
                    circles_remove.clear();
                    break;
                }

                // std::find(static_chains_remove.begin(), static_chains_remove.end(), chain) == static_chains_remove.end()

                for (auto& needle : needles) {
                    // coalescence method
                    /*
                    auto intersection = Intersect(circle.position, old_position, needle.edge_points);
                    if (intersection != circle.position) {
                        circle.position = intersection;
                        break;
                    }*/
                    // chain method
                    auto perim_B = needle.edge_points;
                    if (EdgeIntersect(perim_A, perim_B)) {
                        // add new static circle
                        circle.value = needle.value;
                        static_circles.emplace_back(circle);

                        // mark old circle for removal
                        circles_remove.emplace_back(circle);
                        break;
                    }
                }
                if (!circles_remove.empty()) {
                    for (auto& circle : circles_remove) {
                        circles.erase(std::remove(circles.begin(), circles.end(), circle), circles.end());
                    }
                    circles_remove.clear();
                    break;
                }
            }

            for (auto& chain : chains) {
                auto perim_A = chain.GetPerimeterPoints();

                for (auto& chain2 : chains) {
                    auto perim_B = chain2.GetPerimeterPoints();
                    if (EdgeIntersect(perim_A, perim_B) && chain != chain2) {
                        /*if (chain.value != 0) {
                            circle.value = chain.value;
                        }*/
                        
                        //chains_moving_remove.emplace_back(chain);
                        // add chains together
                        for (auto& member : chain2.group) {
                            chain.group.emplace_back(member);
                            chain.rel_positions.emplace_back(member.position - chain.position);
                        }
                        chain.velocity = { 0,0 };
                        chain.UpdateChain();
                        LOG("Chain collision (" << chain.group.size() << " members) at: " << chain.position);

                        // move chain to remove list
                        chains_remove.emplace_back(chain2);
                        break;
                    }
                }
                if (!chains_remove.empty()) {
                    for (auto& chain : chains_remove) {
                        chains.erase(std::remove(chains.begin(), chains.end(), chain), chains.end());
                    }
                    chains_remove.clear();
                    break;
                }

                for (auto& circle2 : static_circles) {
                    auto perim_B = circle2.GetPerimeterPoints();
                    if (EdgeIntersect(perim_A, perim_B)) {
                        // form new static chain with static circle as root
                        CircleContainer group = { circle2 };
                        std::vector<V2<int>> positions = { {0,0} };
                        for (auto& member : chain.group) {
                            group.emplace_back(member);
                            positions.emplace_back(member.position - circle2.position);
                        }
                        auto new_chain = Chain{ circle2.position, group, positions };
                        new_chain.value = circle2.value;
                        new_chain.UpdateChain();
                        static_chains.emplace_back(new_chain);
                        LOG("Chain collision (" << new_chain.group.size() << " members) at: " << new_chain.position);

                        // mark old chain and circle for removal
                        chains_remove.emplace_back(chain);
                        static_circles_remove.emplace_back(circle2);
                        break;
                    }
                }
                if (!static_circles_remove.empty()) {
                    for (auto& static_circle : static_circles_remove) {
                        static_circles.erase(std::remove(static_circles.begin(), static_circles.end(), static_circle), static_circles.end());
                    }
                    for (auto& chain : chains_remove) {
                        chains.erase(std::remove(chains.begin(), chains.end(), chain), chains.end());
                    }
                    static_circles_remove.clear();
                    chains_remove.clear();
                    break;
                }
                // std::find(chains_remove.begin(), chains_remove.end(), chain) == chains_remove.end()

                for (auto& chain2 : static_chains) {
                    auto perim_B = chain2.GetPerimeterPoints();
                    if (EdgeIntersect(perim_A, perim_B)) {
                        /*if (chain.value != 0) {
                            circle.value = chain.value;
                        }*/
                        // add circle, chain to remove list, then add circle to chain
                        
                        // add chain to static chain
                        for (auto& member : chain.group) {
                            chain2.group.emplace_back(member);
                            chain2.rel_positions.emplace_back(member.position - chain2.position);
                        }
                        chain2.UpdateChain();
                        LOG("Chain collision (" << chain2.group.size() << " members) at: " << chain2.position);

                        // mark old chain for removal
                        chains_remove.emplace_back(chain);
                        break;
                    }
                }
                if (!chains_remove.empty()) {
                    for (auto& chain : chains_remove) {
                        chains.erase(std::remove(chains.begin(), chains.end(), chain), chains.end());
                    }
                    chains_remove.clear();
                    break;
                }

                for (auto& needle : needles) {
                    auto perim_B = needle.edge_points;
                    if (EdgeIntersect(perim_A, perim_B)) {
                        chain.value = needle.value;
                        /*
                        // determine which circle in contact with electrode (this will be root of chain)
                        auto new_root = Circle{ {0,0}, 0, 0 };
                        for (auto& member : chain.group) {
                            auto perim_C = member.GetPerimeterPoints();
                            if (EdgeIntersect(perim_C, perim_B)) {
                                new_root = member;
                                break;
                            }
                        }
                        CircleContainer group = { new_root };
                        std::vector<V2<int>> positions = { {0,0} };
                        for (auto& member : chain.group) {
                            if (member != new_root) {
                                group.emplace_back(member);
                                // need actual grid coordinates, then subtract new_root position for relative position
                                positions.emplace_back(member.position + chain.position - new_root.position);
                            }
                        }
                        */
                        chain.UpdateChain();
                        static_chains.emplace_back(chain);

                        // mark chain for removal
                        chains_remove.emplace_back(chain);
                        break;
                    }
                }
                if (!chains_remove.empty()) {
                    for (auto& chain : chains_remove) {
                        chains.erase(std::remove(chains.begin(), chains.end(), chain), chains.end());
                    }
                    chains_remove.clear();
                    break;
                }
            }
            /*
            // remove collided circles, remove collided chains
            for (auto& circle : circles_remove) {
                moving_circles.erase(std::remove(moving_circles.begin(), moving_circles.end(), circle), moving_circles.end());
                circles.erase(std::remove(circles.begin(), circles.end(), circle), circles.end());
            }
            for (auto& circle : circles_static_remove) {
                static_circles.erase(std::remove(static_circles.begin(), static_circles.end(), circle), static_circles.end());
            }
            for (auto& chain : chains_remove) {
                moving_chains.erase(std::remove(moving_chains.begin(), moving_chains.end(), chain), moving_chains.end());
                chains.erase(std::remove(chains.begin(), chains.end(), chain), chains.end());
            }
            for (auto& chain : chains_static_remove) {
                static_chains.erase(std::remove(static_chains.begin(), static_chains.end(), chain), static_chains.end());
            }
            
            circles_remove.clear();
            circles_static_remove.clear();
            chains_remove.clear();
            chains_static_remove.clear();
               */
            /*
            for (auto& static_circle : static_circles) {
                auto perimeter_points = circle.GetPerimeterPoints();
                auto contact = Intersect(Pf, circle.position, perimeter_points);

            }*/

            //LOG("Old circle position: " << circle.pos_old << " ----> New: " << circle.position);

            /*
            // move chains
            for (auto& group : chains) {
                V2<float> group_force = { 0,0 };
                std::vector<V2<int>> group_old_pos;
                for (auto& circle : group) {
                    group_force += ComputeForceOnCircle(electric_field_squared, circle);
                    group_old_pos.emplace_back(circle.position);
                }
                V2<float> group_vel = { 1.0f * group_force.x, 1.0f * group_force.y };
                for (auto& circle : group) {
                    circle.position += Round(dt * group_vel);
                }
                // check collisions
                for
            }
            */
            
        }
    }

    /*int GetCombinedRadius(const Circle& a, const Circle& b) {
        return static_cast<int>(std::round(std::sqrt((a.radius * a.radius) + (b.radius * b.radius))));
    }*/

    std::pair<int, int> GetCombinedSize(const Circle& A, const Circle& B) {
        // returns a and b; the ratio of these values is set by larger ellipse (ex. a=4 and b=3 for larger ellipse, then a/b for new
        // ellipse will be 4/3
        double ratio;
        if (A.a * A.b >= B.a * B.b) {
            ratio = static_cast<double>(A.a) / static_cast<double>(A.b);
        }
        else {
            ratio = static_cast<double>(B.a) / static_cast<double>(B.b);
        }
        double area = A.a * A.b + B.a * B.b; // = a*b = ratio*b^2 of new ellipse
        double b_new_ = std::sqrt(static_cast<double>(area) / static_cast<double>(ratio));
        int a_new = static_cast<int>(std::round(static_cast<double>(area) / static_cast<double>(b_new_)));
        int b_new = static_cast<int>(std::round(b_new_));
        return {a_new, b_new};
    }
    /*
    CircleContainer CollisionRecursion(CircleContainer& all_circles, CollisionContainer& out_collisions, CircleContainer& out_remainder) {
        CollisionSubroutine(all_circles, out_collisions, out_remainder);
        if (out_collisions.size() > 0) {
            LOG("Collision found!");
            // in for-loop, I iterate over every object in 'out_collisions'; I also def a and b as elements of each pair in 'out_collisions';
            int size = sizeof(out_collisions) / sizeof(out_collisions[0]);
            for (int i = 0; i < size; i++) {
                auto a = out_collisions[i].first;
                auto b = out_collisions[i].second;
                auto new_position = Round(((a.a * a.b * a.position) + (b.a * b.b * b.position)) / (a.a*a.b + b.a*b.b)); // weighted new pos
                //auto new_radius = GetCombinedRadius(a, b);
                auto new_size = GetCombinedSize(a, b);
                auto new_a = new_size.first;
                auto new_b = new_size.second;
                LOG("New Position: " << new_position);
                LOG("New a, b: " << new_a << ", " << new_b);
                //LOG("New Radius: " << new_radius);
                out_remainder.emplace_back(new_position, new_a, new_b);
            }
            all_circles = out_remainder;
            out_collisions.clear();
            out_remainder.clear();
            return CollisionRecursion(all_circles, out_collisions, out_remainder);
        }
        else {
            return out_remainder;
        }
    }
    */

    void UpdateCollisions() {
        bool collisions = true;
        while (collisions == true) {
            CircleContainer all_circles{ circles };
            CollisionContainer out_collisions;
            CircleContainer out_remainder;
            //out_remainder = CollisionRecursion(all_circles, out_collisions, out_remainder);
            //circles = out_remainder;
            CircleContainer collided;
            CircleContainer new_circles;
            for (auto& a : circles) {
                for (auto& b : circles) {
                    if (a != b && CircleVsCircle(a, b) && !HasCircle(a, collided) && !HasCircle(b, collided)) {
                        // coalescence method ---------------------------------------------------------------
                        auto new_position = Round(((a.a * a.b * a.position) + (b.a * b.b * b.position)) / (a.a * a.b + b.a * b.b));
                        //auto new_radius = GetCombinedRadius(a, b);
                        auto new_size = GetCombinedSize(a, b);
                        auto new_a = new_size.first;
                        auto new_b = new_size.second;
                        LOG("New Position: " << new_position);
                        LOG("New a, b: " << new_a << ", " << new_b);
                        //LOG("New Radius: " << new_radius);
                        new_circles.emplace_back(Circle{ new_position, new_a, new_b });
                        collided.emplace_back(a);
                        collided.emplace_back(b);
                        break;
                    }
                }
            }
            for (auto& circle : circles) {
                if (!HasCircle(circle, collided)) {
                    new_circles.emplace_back(circle);
                }
            }
            circles = new_circles;
            CircleContainer new_static;
            CircleContainer pre_static_collided;
            out_remainder.clear();
            for (auto& circle : circles) {
                bool colliding = false;
                for (auto& static_circle : static_circles) {
                    if (!colliding && !HasCircle(circle, static_circles) && CircleVsCircle(circle, static_circle)) {
                        LOG("Static Circle collision!");
                        LOG("Circle start a, b: " << circle.a << ", " << circle.b);
                        LOG("Stat circ start a, b: " << static_circle.a << ", " << static_circle.b);
                        colliding = true;
                        auto new_size = GetCombinedSize(static_circle, circle);
                        auto new_a = new_size.first;
                        auto new_b = new_size.second;
                        // radius of free circle is scaled for similar reason as below (read comment in next code block)
                        // circle.radius = GetCombinedRadius(circle, circle);
                        pre_static_collided.emplace_back(static_circle);

                        // code line below is old method of merging ellipses
                        new_static.emplace_back(Circle{ static_circle.position, new_a, new_b, 0, static_circle.value });

                        // chain method
                        /*
                        auto perimeter_points = circle.GetPerimeterPoints();
                        auto contact = Intersect(Pf, circle.position, perimeter_points);
                        new_static.emplace_back(static_circle);
                        */
                    }
                }
                for (const auto& rect : rectangles) {
                    if (!colliding && !HasCircle(circle, static_circles) && CircleVsRectangle(circle, rect)) {
                        LOG("Electrode collision!");
                        colliding = true;
                        circle.position = ClosestPointOnRectangle(circle.position, rect);
                        // radius is scaled such that half circle sticking out of electrode has same area as original whole circle
                        //circle.radius = GetCombinedRadius(circle, circle);
                        circle.value = rect.value;
                        new_static.emplace_back(circle);
                    }
                }
                for (const auto& tri : triangles) {
                    if (!colliding && !HasCircle(circle, static_circles) && CircleVsTriangle(circle, tri)) {
                        LOG("Electrode collision!");
                        colliding = true;
                        circle.position = ClosestPointOnTriangle(circle, tri);
                        circle.value = tri.value;
                        new_static.emplace_back(circle);
                    }
                }
                for (const auto& needle : needles) {
                    if (!colliding && !HasCircle(circle, static_circles) && CircleVsNeedle(circle, needle)) {
                        LOG("Electrode collision!");
                        colliding = true;
                        circle.position = ClosestPointOnNeedle(circle, needle);
                        circle.value = needle.value;
                        new_static.emplace_back(circle);
                    }
                }
                if (!colliding) {
                    out_remainder.emplace_back(circle);
                }
            }
            circles = out_remainder;
            for (const auto& static_circle : static_circles) {
                if (!HasCircle(static_circle, new_static) && !HasCircle(static_circle, pre_static_collided)) {
                    new_static.emplace_back(static_circle);
                }
            }
            static_circles = new_static;



            CircleContainer static_collided;
            CircleContainer static_new_circles;
            for (auto& a : static_circles) {
                for (auto& b : static_circles) {
                    if (a != b && CircleVsCircle(a, b) && !HasCircle(a, static_collided) && !HasCircle(b, static_collided)) {
                        if (OnLeftSide(a.position, glob_grid_size) == !OnLeftSide(b.position, glob_grid_size)) {
                            sim_stopped = true;
                            return;
                        }
                        auto new_position = Round(((a.a * a.b * a.position) + (b.a * b.b * b.position)) / (a.a * a.b + b.a * b.b));
                        //auto new_radius = GetCombinedRadius(a, b);
                        auto new_size = GetCombinedSize(a, b);
                        auto new_a = new_size.first;
                        auto new_b = new_size.second;
                        LOG("New Position: " << new_position);
                        LOG("New a, b: " << new_a << ", " << new_b);
                        //LOG("New Radius: " << new_radius);
                        static_new_circles.emplace_back(Circle{ new_position, new_a, new_b, 0, a.value });
                        static_collided.emplace_back(a);
                        static_collided.emplace_back(b);
                        break;
                    }
                }
            }
            for (auto& circle : static_circles) {
                if (!HasCircle(circle, static_collided)) {
                    static_new_circles.emplace_back(circle);
                }
            }
            static_circles = static_new_circles;

            if (collided.empty() && pre_static_collided.empty() && static_collided.empty()) {
                collisions = false;
            }
        }
    }
    
    std::vector<double> Measure(const V2<int>& grid_size, const Grid<Value>& magnitude) {
        CircleContainer static_left;
        CircleContainer static_right;

        // SEPARATION CALCULATION NEEDS TO BE UPDATED FOR CHAINS

        // throw static circles into left or right bin
        for (auto& a : static_circles) {
            if (OnLeftSide(a.position, grid_size)) {
                static_left.emplace_back(a);
            }
            else {
                static_right.emplace_back(a);
            }
        }

        // identify left and right electrodes
        std::vector<V2<int>> left_elec_verts;
        std::vector<V2<int>> right_elec_verts;
        for (auto& tri : triangles) {
            if (OnLeftSide(tri.v1, grid_size)) {
                auto left_elec = tri;
                left_elec_verts.emplace_back( tri.v1 );
                left_elec_verts.emplace_back( tri.v2 );
                left_elec_verts.emplace_back( tri.v3 );
            }
            else {
                auto right_elec = tri;
                right_elec_verts.emplace_back(tri.v1);
                right_elec_verts.emplace_back(tri.v2);
                right_elec_verts.emplace_back(tri.v3);
            }
        }

        for (auto& needle : needles) {
            if (OnLeftSide(needle.v1, grid_size)) {
                auto left_elec = needle;
                left_elec_verts.emplace_back(needle.v1);
                left_elec_verts.emplace_back(needle.v2);
                left_elec_verts.emplace_back(needle.v3);
            }
            else {
                auto right_elec = needle;
                right_elec_verts.emplace_back(needle.v1);
                right_elec_verts.emplace_back(needle.v2);
                right_elec_verts.emplace_back(needle.v3);
            }
        }

        double dist = grid_size.x;

        // accounts for *no static circles* and **static circles only on right electrode**
        for (V2<int>& u : left_elec_verts) {
            for (V2<int>& v : right_elec_verts) {
                if (Distance(u, v) < dist) {
                    dist = Distance(u, v);
                }
            }
            for (auto& b : static_right) {
                if (Distance(b.position, u) - b.a < dist) {
                    dist = Distance(b.position, u) - b.a;
                }
            }
        }

        // accounts for ***static circles only on left electrode***
        for (V2<int>& v : right_elec_verts) {
            for (auto& b : static_left) {
                if (Distance(b.position, v) - b.a < dist) {
                    dist = Distance(b.position, v) - b.a;
                }
            }
        }

        // if both electrodes have >= 1 static circle
        for (auto& a : static_left) {
            for (auto& b : static_right) {
                auto sep = Distance(a.position, b.position) - a.a - b.a;
                if (sep < dist) {
                    dist = sep;
                }
            }
        }
        /*
        // define integrand
        struct params_t {
            double s;
            double V;
            double W;
        };
        
        auto integrate = [](double sep, double voltage) {

            gsl_integration_workspace* workspace = gsl_integration_workspace_alloc(1000);
            double scale = 0.01; // scales voltage and separation distance (used to avoid underflow errors)
            double result, error;
            double lower_limit = -abs(voltage) * scale;
            double upper_limit = abs(voltage) * scale;
            params_t params;
            params.s = sep * scale; params.V = voltage * scale; params.W = 3;
            gsl_function F;
            F.function = [](double E, void* params) {
                params_t* p = static_cast<params_t*>(params);
                if (p->W < p->V) {
                    LOG("Warning! Work function less than bounds of integration.")
                }
                return (
                    (gsl_sf_exp(-p->s * sqrt(abs(p->W - E)))
                        * (gsl_sf_exp(E + (p->V / 2)) - gsl_sf_exp(E - (p->V / 2)))
                        / ((1 + gsl_sf_exp(E + (p->V / 2))) * (1 + gsl_sf_exp(E - (p->V / 2)))))
                    );
                };
            F.params = &params;
            gsl_integration_qags(&F, lower_limit, upper_limit, 0, 1e-7, 1000, workspace, &result, &error);
            return(result);
            };
        // integrate function
        
        double current = integrate(dist, voltage);
        */
        double resistance = Resistance(grid_size, magnitude);

        
        LOG("Separation: " << dist);
        //LOG("Current: " << current);
        //LOG("Voltage: " << voltage);
        LOG("Resistance: " << resistance);
        return { dist, resistance };
    } 

    double Resistance(const V2<int>& size, const Grid<Value>& magnitude) {
        double R_total_inv = 0;
        for (int j = 1; j < size.y - 1; j++) {
            double R_series = 0;
            for (int i = 1; i < size.x - 1; i++) {
                if (magnitude[j * size.x + i] >= 1E-4) {
                    R_series++;
                }
                else {
                    continue;
                }
            }
            if (R_series != 0) {
                R_total_inv += 1 / R_series;
            }
            else if (R_series == 0) {
                R_total_inv = INFINITY;
            }
            else {
                LOG("Series resistance not computed");
            }
        }
        double R_total = 1 / R_total_inv;
        return(R_total);
    }
    
    bool HasCircle(const Circle& circle, const CircleContainer& container) {
        for (const auto& c : container) {
            if (circle == c) return true;
        }
        return false;
    }

    bool InRectangle(const V2<int> loc, const V2<int> position, const V2<int> size) {
        // returns true if circle (position = loc) is in rectangle; returns false otherwise
        // rectangle defined by lower-left vertex (position) and size (size)
        bool in_rect = false;
        int x1 = position.x;
        int x2 = position.x + size.x;
        int y1 = position.y;
        int y2 = position.y + size.y;
        
        if (int i = loc.x, j = loc.y; i > x1 && i < x2 && j > y1 && j < y2) {
            in_rect = true;
        }
        return { in_rect };
    }

    bool OnLeftSide(const V2<int> position, const V2<int> grid_size) {
        if (position.x < grid_size.x / 2) {
            return true;
        }
        else {
            return false;
        }
    }
    /*
    V2<int> FirstIntersection(const V2<int> init, const V2<int> final, const V2<int> position, const V2<int> size) {
        //determines first point of intersection of line with rectangular region defined by position of lower left vertex and size
        //init and final are first and last points of path
        //returns coordinates { x, y }
        float slope = Slope(init, final);
        int parts = 1000;
        double dx = (final.x - init.x) / parts;
        double dy = (final.y - init.y) / parts;
        std::vector<V2<int>> steps;
        for (int i = 0; i < parts; i++) {
            steps.push_back(V2<int> ( std::round(init.x + (i * dx)), std::round(init.y + (i * dy)) ));
        }
        steps.push_back(Round(final));
        
        for (int i = 0; i <= parts; i++) {
            if (InRectangle(steps[i], position, size)) {
                return steps[i-1];
            }
        }
    }
    */

    // Computes the electric field with the magnitude squared of the potential gradient. @return electric_field, electric_field_squared
    std::pair<Grid<Value>, Grid<Value>> ComputeElectricField(const Grid<Value>& potentials) const {
        Grid<Value> electric_field{ potentials.GetSize() };
        electric_field.InitValue();
        auto electric_field_squared = electric_field;
        auto gradient_grid = potentials.GetGradient();
        for (auto i = 0; i < electric_field.GetLength(); i++) {
            auto magnitude_squared = static_cast<Value>(gradient_grid[i].MagnitudeSquared());
            electric_field_squared[i] = magnitude_squared;
            electric_field[i] = std::sqrt(magnitude_squared);
        }
        return { electric_field, electric_field_squared };
    }
    /* Runs RelaxGrid for a set number of loops using the input grid, num of loops, and if force_circles.
    Description of RelaxGrid below.*/
    void Relax(Grid<Value>& grid, int relaxation_loops, bool force_circles) {
        auto size = grid.GetSize();
        for (auto i = 0; i < relaxation_loops; ++i) { // Runs relaxgrid for a set number of times
            RelaxGrid(size, grid, force_circles);
            // LOG(i);
        }
    }
    /* Function for relaxing grid; updates grid values based on average of 8 surrounding points; 
    also updates circle positions if forces_circles is true*/
    void RelaxGrid(const V2<int>& size, Grid<Value>& grid, bool forces_circles) {  
        auto old_grid = grid;
        #pragma omp parallel for
        for (auto i = 1; i < size.x - 1; ++i) {
            auto x_last = i - 1;
            auto x_next = i + 1;
            for (auto j = 1; j < size.y - 1; ++j) {
                auto y_part = size.x * j;   // y_part is the starting index of the jth row, reminder grid is a 1D array so to access (i,j) we need to do y_part + i
                auto y_index_minus = y_part - size.x;
                auto y_index_plus = y_part + size.x;
                grid[y_part + i] = (    // average of 8 surrounding points
                      old_grid[y_index_minus + x_last]
                    + old_grid[y_index_minus + i]
                    + old_grid[y_index_minus + x_next]
                    + old_grid[y_part + x_last]
                    + old_grid[y_part + x_next]
                    + old_grid[y_index_plus + x_last]
                    + old_grid[y_index_plus + i]
                    + old_grid[y_index_plus + x_next]
                ) / static_cast<Value>(8.0);
            }
        }
        if (electrode == 0) {
            UpdateTriangles(grid);
        }
        else if (electrode == 1) {
            UpdateNeedles(grid);
        }
        else if (electrode == 2) {
            UpdateRectangles(grid);
        } 
        if (forces_circles) {
            UpdateCircles(grid, circles);
            UpdateCircles(grid, static_circles);
            UpdateChains(grid, chains);
            UpdateChains(grid, static_chains);
        }
        
    }

    //V2<Value> ComputeForceOnCircle(const Grid<Value>& electric_field_squared, const Circle& circle) const {
    //    V2<Value> force;
    //    auto gradient_grid = electric_field_squared.GetGradient();
    //    for (auto i = circle.position.x - circle.radius; i < circle.position.x + circle.radius; ++i) {
    //        for (auto j = circle.position.y - circle.radius; j < circle.position.y + circle.radius; ++j) {
    //            V2<int> point = { i, j };
    //            if (PointVsCirclePerimeter(point, circle)) {
    //                force += gradient_grid[point];
    //            }
    //        }
    //    }
    //    return force;
    //}

    V2<Value> ComputeForceOnCircle(const Grid<Value>& electric_field_squared, const Circle& circle) const {
        V2<Value> force;
        auto origin = V2<int>{ 0, 0 };
        const V2<int> grid_size = electric_field_squared.GetSize();
        auto gradient_grid = electric_field_squared.GetGradient();
        auto perimeter_points = circle.GetPerimeterPoints();
        for (const auto& point : perimeter_points) {
            /*
            if (!InRectangle(point, origin, grid_size)) {
                continue;
            }
            */
            force += gradient_grid[point];
        }
        return force;
    }

    double Slope(const V2<int> initial, const V2<int> final) {
        // Accepts initial and final positions of circle and returns slope of line between them (as dy/dx)
        double slope;
        int dx = final.x - initial.x;
        int dy = final.y - initial.y;
        if (dx == 0) {
            if (dy < 0) {
                slope = -INFINITY;
            }
            else if (dy > 0) {
                slope = INFINITY;
            }
            else {
                slope = 0;
            }
        }
        else {
            slope = dy / dx;
        }
        return slope;
    }

    float Distance(const V2<int> pos1, const V2<int> pos2) {
        // Calculates distance between 2 points
        auto dist = sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y));
        return dist;
    }

    std::vector<int> ComputeForceTest() {
        std::vector<int> forces;
        for (const auto& circle : circles) {
            LOG("Looking at next circle.")
            int force = 0;
            auto perimeter_points = circle.GetPerimeterPoints();
            LOG(perimeter_points);
            force += perimeter_points.size();
            forces.push_back(force);
        }
        return forces;
    }

    int CheckAreaTest() {
        int A = 0;
        for (const auto& c : circles) {
            int area = c.a * c.b;
            A += area;
            
        }
        for (const auto& c : static_circles) {
            int area = c.a * c.b;
            A += area;
        }
        return A;
        for (const auto& chain : chains) {
            for (const auto& c : chain.group) {
                int area = c.a * c.b;
                A += area;
            }
        }
        for (const auto& static_chain : chains) {
            for (const auto& c : static_chain.group) {
                int area = c.a * c.b;
                A += area;
            }
        }
    }
    /*
    void CollisionSubroutine(const CircleContainer& all_circles, CollisionContainer& out_collisions, CircleContainer& out_remainder) {
        for (const auto& a : all_circles) {
            bool collided = false;
            for (const auto& b : all_circles) {
                if (a != b && CircleVsCircle(a, b) && std::find(out_collisions.begin(), out_collisions.end(), a) == out_collisions.end() 
                    && std::find(out_collisions.begin(), out_collisions.end(), b) == out_collisions.end()) {
                    auto it = std::find_if(std::begin(out_collisions), std::end(out_collisions),
                        [&](std::pair<Circle, Circle> pair) {
                        return (a == pair.first && b == pair.second) || (a == pair.second && b == pair.first);
                    });
                    if (it == std::end(out_collisions)) {
                        out_collisions.emplace_back(a, b);
                    }
                    collided = true;
                }
            }
            if (!collided) {
                out_remainder.emplace_back(a);
            }
        }
    }
    */
    // With a potential grid, and a container of circles, set the grid's potential for the points that the cirlces occupy = to the circle's potential with grid.SetCircle(circle)
    void UpdateCircles(Grid<Value>& grid, const CircleContainer& container) {
        for (const auto& circle : container) {
            grid.SetCircle(circle);
        }
    }

    void UpdateChains(Grid<Value>& grid, const ChainContainer& container) {
        for (const auto& chain : container) {
            grid.SetChain(chain);
        }
    }

    void UpdateRectangles(Grid<Value>& grid) {
        for (const auto& rectangle : rectangles) {
            grid.SetRectangle(rectangle);
        }
    }

    void UpdateTriangles(Grid<Value>& grid) {
        for (const auto& triangle : triangles) {
            grid.SetTriangle(triangle);
        }
    }

    void UpdateNeedles(Grid<Value>& grid) {
        for (const auto& needle : needles) {
            grid.SetNeedle(needle);
        }
    }

    void AddCircle(const Circle& circle) {
        circles.emplace_back(circle);
    }

    void AddRectangle(const Rectangle& rectangle) {
        rectangles.emplace_back(rectangle);
    }

    void AddTriangle(const Triangle& triangle) {
        triangles.emplace_back(triangle);
    }

    void AddNeedle(const Needle& needle) {
        needles.emplace_back(needle);
        }

    void AddChain(const Chain& chain) {
        chains.emplace_back(chain);
    }

private:
    Grid<Value> relaxed_grid;
    int electrode;
    CircleContainer circles;
    CircleContainer static_circles;
    ChainContainer chains;
    ChainContainer static_chains;
    std::vector<Rectangle> rectangles;
    std::vector<Triangle> triangles;
    std::vector<Needle> needles;
    int loops{ 0 };
    DataHandler data_handler;
};
