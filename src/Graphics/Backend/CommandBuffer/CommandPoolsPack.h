#pragma once
#include "Utilities.h"
#include <vulkan/vulkan.hpp>
#include <array>

template <typename CommandPool>
struct CommandPoolsPack { 
	std::array<CommandPool, 3> pools;
	
	void create() 
	{ 
		this->pools[0].create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, GRAPHICS); 
		this->pools[1].create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, TRANSFER);
		this->pools[2].create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, COMPUTE);
	}

	void destroy()
	{ 
		this->pools[0].destroy(); 
		this->pools[1].destroy(); 
		this->pools[2].destroy(); 	
	}
	
	CommandPool& getPoolByQueue(QueueFamilyEnum queueFamily_)
	{
		return this->pools[queueFamily_];
	}
}; 
