#include "FrameResources.h"
#include "Globals.h"

void FrameResources::init()
{
	this->graphicsCommandPool.create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, GRAPHICS); 
	this->computeCommandPool.create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, COMPUTE);
	this->transferCommandPool.create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, TRANSFER);
}

void FrameResources::resetCMDBuffers()
{
	this->graphicsCommandPool.resetCMDBuffers(); 
	this->computeCommandPool.resetCMDBuffers(); 
	this->transferCommandPool.resetCMDBuffers(); 
}

void FrameResources::destroy()
{
	this->graphicsCommandPool.destroy(); 
	this->computeCommandPool.destroy(); 
	this->transferCommandPool.destroy(); 
}
