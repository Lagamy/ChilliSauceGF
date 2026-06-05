#include "GPUMemoryEntry.h"
#include "Globals.h"

GPUMemoryEntry::GPUMemoryEntry(const char* name_, VkDeviceSize size_, StorageUnitEnum unit_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuShared_) {
	this->name = name_;
	this->cpuShared = cpuShared_; 
	this->size = size_; 
	this->unit = unit_;
	VkMemoryRequirements memRequirements;
	if (!cpuShared_) {
		 
		/* GPU SIDE BUFFER (Data is stored in VRAM. Faster, but only GPU accessible)*/
		// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data
		this->buffer.create(this->size, this->unit, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsageFlags_, bufferSharingMode_, name_); // Create a buffer that can both recieve Data from Transfer, and be read as whatever you want(Vertex Buffer, Uniform buffer, Texture buffer)

		// Allocate memory
		vkGetBufferMemoryRequirements(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), &memRequirements);
		this->memoryBlock.create(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, name_);

		// Bind buffer to memory block 
		vkBindBufferMemory(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
	}
	else
	{
		this->buffer.create(this->size, bufferUsageFlags_, bufferSharingMode_, name_); 

		// Allocate memory
		vkGetBufferMemoryRequirements(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), &memRequirements);
		this->memoryBlock.create(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, name_);

		// Bind buffer to memory block 
		vkBindBufferMemory(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
		vkMapMemory(Demo::renderer.mainDevice.logicalDevice, this->memoryBlock.get(), 0, this->size, 0, &this->pCpuSharedData);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping. 

	}
}; 




GPUMemoryEntry::~GPUMemoryEntry() {
	this->memoryBlock.destroy(); 
	this->buffer.destroy(); 
}


