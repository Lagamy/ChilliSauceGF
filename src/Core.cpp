#include "Core.h"
#include "Api.h"
#include "Globals.h"
#include <GLFW/glfw3.h>

void Core::setup()
{
	this->buildWindow();
	Globals::renderer.setup(); // Sets up instance, chooses device, created swapchain.  	
}


void Core::run()
{
	while(!glfwWindowShouldClose(Graphics::getWindowPointer()))
	{
		Globals::renderer.draw(); 
 		glfwPollEvents();
	}

}

void Core::shutdown()
{
	Globals::renderer.shutdown();
	glfwDestroyWindow(Globals::appWindow);
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
	Globals::appWindow = glfwCreateWindow(Globals::windowWidth, Globals::windowHeight, Globals::processName.c_str(), nullptr, nullptr);
	
	glfwPollEvents();
};
