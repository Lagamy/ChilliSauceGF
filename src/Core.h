#pragma once 
#include "Globals.h"

struct Core {
public: 
	void setup(); 
	void shutdown(); 
	void loadProject(const char* path_);
	void addProject(const char* name_); 
	void buildWindow();
	
	void run(); 
};
