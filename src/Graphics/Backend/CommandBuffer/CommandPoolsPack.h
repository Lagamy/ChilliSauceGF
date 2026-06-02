#pragma once
#include "Utilities.h"
#include <vulkan/vulkan.hpp>
#include <array>

template <typename CommandPool>
struct CommandPoolsPack { 
	std::array<CommandPool, 3> commandPools;
	
	void create() 
	{ 
		this->commandPools[0].create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, GRAPHICS); 
		this->commandPools[1].create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, COMPUTE);
		this->commandPools[2].create(VK_COMMAND_BUFFER_LEVEL_PRIMARY, TRANSFER);
	}

	void destroy()
	{ 
		this->commandPools[0].destroy(); 
		this->commandPools[1].destroy(); 
		this->commandPools[2].destroy(); 	
	}
}; 
