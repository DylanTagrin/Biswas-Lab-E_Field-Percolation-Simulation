#pragma once
#include "V2.h"
#include "Defines.h"

struct Rectangle {
    // note that position is lower left vertex of rectangle and size are dimensions in x, y
    // value is voltage
    Rectangle(const V2<int>& position, const V2<int>& size, Value value = 0) : position{ position }, size{ size }, value{ value } {}
    V2<int> position;
    V2<int> size;
    Value value{ 0 };
};