#pragma once
#include "V2.h"
#include "Defines.h"

struct Rectangle {
    Rectangle(const V2<int>& position, const V2<int>& size, Value value = 0) : position{ position }, size{ size }, value{ value } {}
    V2<int> position;
    V2<int> size;
    Value value{ 0 };
};