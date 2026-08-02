#pragma once 
#include "Globals.h"
#include "Api.h"

struct Core {
	void setup(); 
	void shutdown(); 
	void loadProject(const char* path_);
	void addProject(const char* name_); 
	void buildWindow();
	static void windowSizeCallback(GLFWwindow*, int width, int height) 
	{
		Globals::resizing = true; 
		vkDeviceWaitIdle(Graphics::getMainDevice().logicalDevice);
    	Globals::windowWidth = width;
    	Globals::windowHeight = height;
		Graphics::getSwapchain().destroyFramebuffers();
		Graphics::getSwapchain().destroy(); 
		Graphics::getSwapchain().create(); 
		Graphics::getSwapchain().createFramebuffers(Graphics::getPresentationRenderPass());
		// Globals::Mouse::lastX = width / 2.0f;
		// Globals::Mouse::lastY = height / 2.0f;
    	// glViewport(0, 0, width, height);
		Globals::resizing = false;
	};

	void run(); 
};
