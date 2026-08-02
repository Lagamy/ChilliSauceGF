#pragma once
#include <string>

template <typename Resource> 
struct Asset { 
	std::string name; 
	std::string path; 
	Resource resource; 
	bool uploaded = false; 
	bool exists = true; 	

	Asset(const char* name_, const char* path_) : name(name_), path(path_) {}; 
};
