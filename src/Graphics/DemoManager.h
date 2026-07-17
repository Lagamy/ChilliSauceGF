// Manages demo setup and connects it to the renderer.
#pragma once 

#include "Utilities.h"
#include "PassesPack.h"
#include "Triangle.h"
#include <vector>
#include <thread>

namespace Graphics
{
using demoFunc = std::function<void()>;
struct DemoManager {
	PassesPack framePassesPack; 
	PassesPack oneShotPassesPack;
	

	
	demoFunc loadDemo; 
	demoFunc createGPUObjects; 
	demoFunc submitToGPU; 
	// You can just include demos there, and comment out demos that aren't active
	Triangle triangle; 
};
}
