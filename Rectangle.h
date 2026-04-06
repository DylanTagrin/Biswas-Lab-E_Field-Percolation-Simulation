#pragma once
#include "V2.h"
#include "Defines.h"

// Rectangle struct, initilized with a V2 position, V2 size, and float(Value) 'value' which is voltage
// Note: The constructor argument (ie position) and the member variable (ie position) have the same name

// Rectangle class structure intilized with V2 pos, V2 size, value voltage with pos being the lower left vertx
struct Rectangle {
    // note that position is lower left vertex of rectangle and size are dimensions in x, y
    // value is voltage
    Rectangle(const V2<int>& position, const V2<int>& size, Value value = 0) : position{ position }, size{ size }, value{ value } {}
    V2<int> position;
    V2<int> size;
    Value value{ 0 };
};