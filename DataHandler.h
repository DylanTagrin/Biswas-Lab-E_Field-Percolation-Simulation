#pragma once
#include <vector>
#include <tuple>
#include <fstream>

#include "json.hpp"
#include "V2.h"
#include "TypeTraits.h"

using json = nlohmann::json;

class DataHandler {
public:
	DataHandler() = default;

	template <typename ...TArgs>
	DataHandler(std::pair<const char*, TArgs>&&... args) {
		(Write(args.first, args.second), ...);
	}

	template <typename T>
	void Write(const char* property, const T& value) {
		data[property] = value;
	}

	template <typename T>
	void Add(const char* property, const T& value) {
		data[property].push_back(value);
	}

	template <typename T, is_number<T> = true>
	T Read(const char* property) {
		return data[property];
	}

	template <typename T>
	const T& Read(const char* property) {
		return data[property];
	}

	void SaveToFile(const char* file_path) {
		std::ofstream o(file_path);
		o << data << std::endl; //<< std::setw(4) <<
	}

private:
	json data;
};