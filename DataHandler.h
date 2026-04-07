#pragma once
#include <vector>
#include <tuple>
#include <fstream>
#include "json.hpp"
#include "V2.h"
#include "TypeTraits.h"


// Grabs a json library from json.hpp
using json = nlohmann::json;

/* Lightweight Data wrapper that allows for the output to be put into a json file.*/
class DataHandler {
public:
	// Create empty handler
	DataHandler() = default;
	/* This allows for a data handler to be constructed with several arguments at once by 
	calling write like with 
	data_handler{
    Data<int>{ "width", grid_size.x },
    Data<int>{ "height", grid_size.y }
	}*/
	template <typename ...TArgs>
	DataHandler(std::pair<const char*, TArgs>&&... args) {
		(Write(args.first, args.second), ...);
	}
	// Allows you to write a data value with write("name", value)
	template <typename T>
	void Write(const char* property, const T& value) {
		data[property] = value;
	}
	// Appends a new value to an array with data_handler.Add("measurements", Measure(...));
	template <typename T>
	void Add(const char* property, const T& value) {
		data[property].push_back(value);
	}
	// Two read commands, one that returns a number, and another others
	template <typename T, is_number<T> = true>
	T Read(const char* property) {
		return data[property];
	}
	template <typename T>
	// Was const T& but that might make compile errors so it was changed into const T
	const T Read(const char* property) {
		return data[property];
	}
	// Allows for the json file to be written, might have some issues
	void SaveToFile(const char* file_path) {
		std::ofstream o(file_path);
		o << data << std::endl; //<< std::setw(4) <<
	}

private:
	json data;
};