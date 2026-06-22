#include "GPUMemoryEntry.h"
#include "Api.h"

namespace Graphics
{
GPUMemoryEntry::GPUMemoryEntry(const char* name_, VkDeviceSize size_, StorageUnitEnum unit_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuShared_)
{
	this->cpuShared = cpuShared_; 
	this->size = size_; 
	this->unit = unit_;
	std::array<VkMemoryRequirements, 1> memRequirement;
	if (!cpuShared_) {
		 
		/* GPU SIDE BUFFER (Data is stored in VRAM. Faster, but only GPU accessible)*/
		// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data
		this->buffer.create(this->size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsageFlags_, bufferSharingMode_, name_); // Create a buffer that can both recieve Data from Transfer, and be read as whatever you want(Vertex Buffer, Uniform buffer, Texture buffer)

		// Allocate memory
		vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffer.get(), &memRequirement[0]);
		this->memoryBlock.create(this->size, this->unit, memRequirement, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, name_);

		// Bind buffer to memory block 
		vkBindBufferMemory(getMainDevice().logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
	}
	else
	{
		this->buffer.create(this->size, bufferUsageFlags_, bufferSharingMode_, name_); 

		// Allocate memory
		vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffer.get(), &memRequirement[0]);
		this->memoryBlock.create(this->size, this->unit, memRequirement, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, name_);

		// Bind buffer to memory block 
		vkBindBufferMemory(getMainDevice().logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
	}
}; 


void GPUMemoryEntry::destroy()
{
	this->memoryBlock.destroy(); 
	this->buffer.destroy();
}

GPUMemoryEntry::~GPUMemoryEntry() 
{
	this->memoryBlock.destroy(); 
	this->buffer.destroy(); 
}
}

