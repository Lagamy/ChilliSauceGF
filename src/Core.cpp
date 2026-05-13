#include "Core.h"

void Core::setup()
{
	Demo::renderer.setup(); // Sets up instance, chooses device, created swapchain.  	
}

void Core::loadProject(const char* path_)
{
	// Load from binary
}



void Core::addProject(const char* name_)
{
	// EngineGlobals::currentProject = Project(name_);

	// Create a binary 
}

void Core::run()
{
	Demo::renderer.run(); 
}


void Core::buildWindow() {

	glfwSetErrorCallback([](int error, const char* description) {
  		fprintf(stderr, "GLFW Error [%d]: %s\n", error, description);
	});

	// Initialize glfw 
	glfwInit(); 

	// No default rendering client, I will hook Vulkan up instead. 
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Resizing breaks the swapchain, I will disable it for now. 
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); 

	// Create window 
	App::appWindow = glfwCreateWindow(App::windowWidth, App::windowHeight, App::processName.c_str(), nullptr, nullptr);
	
	glfwPollEvents();
};
