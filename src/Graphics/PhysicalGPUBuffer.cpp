#include "PhysicalGPUBuffer.h"
#include "Globals.h"

PhysicalGPUBuffer::PhysicalGPUBuffer(const char* name_, size_t size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuShared_) {
	this->name = name_;
	this->cpuShared = cpuShared_; 
	this->size = size_; 
	VkMemoryRequirements memRequirements;
	if (!cpuShared_) {
		 
		/* GPU SIDE BUFFER (Data is stored in VRAM. Faster, but only GPU accessible)*/
		// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data
		this->buffer.create(this->size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsageFlags_, bufferSharingMode_, name_); // Create a buffer that can both recieve Data from Transfer, and be read as Vertex Buffer 

		// Allocate memory
		vkGetBufferMemoryRequirements(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), &memRequirements);
		this->memoryBlock.create(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, name_);

		// Bind buffer to memory block 
		vkBindBufferMemory(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
	}
	else
	{
		// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data
		this->buffer.create(this->size, bufferUsageFlags_, bufferSharingMode_, name_); // Create a buffer that can both recieve Data from Transfer, and be read as Vertex Buffer 

		// Allocate memory
		vkGetBufferMemoryRequirements(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), &memRequirements);
		this->memoryBlock.create(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, name_);

		// Bind buffer to memory block 
		vkBindBufferMemory(Demo::renderer.mainDevice.logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
		vkMapMemory(Demo::renderer.mainDevice.logicalDevice, this->stagingMemoryBlock.get(), 0, this->size, 0, &this->pCpuSharedData);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping. 

	}
}; 




PhysicalGPUBuffer::~PhysicalGPUBuffer() {
	if (this->cpuShared)
	{
		this->stagingMemoryBlock.destroy();
		this->stagingBuffer.destroy();
	}
	this->memoryBlock.destroy(); 
	this->buffer.destroy(); 
}

void PhysicalGPUBuffer::upload(const void* data_, VkQueue queue_, CommandPool& rCommandPool_) {
	// Doesn't need guard rails, as it is an internall process 
	if (this->cpuShared)
	{ 
		// Map our vertex data to vertex Buffer 
		memcpy(pCpuSharedData, data_, this->size);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer

	}
	else
	{
		// Map our vertex data to vertex Buffer 
		memcpy(this->pCpuSharedData, data_, this->size);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer

		// Copy staging buffer to vertex buffer on GPU
		GraphicsUtilities::copyBuffer(queue_, rCommandPool_.get(), this->stagingBuffer.get(), this->buffer.get(), 0, 0, this->size);
	}
}

void PhysicalGPUBuffer::upload(const void* data_, VkQueue queue_, CommandPool& rCommandPool_, size_t byteAmount_, size_t srcStartingByte_, size_t dstStartingByte_) {
	// Doesn't need guard rails, as it is an internall process 
	if (this->cpuShared)
	{
		// Map our vertex data to vertex Buffer 
		void* sharedDataP; // Create an empty typeless pointer.
		vkMapMemory(Demo::renderer.mainDevice.logicalDevice, this->stagingMemoryBlock.get(), dstStartingByte_, byteAmount_, 0, &sharedDataP);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping. 
		memcpy(sharedDataP, static_cast<const char*>(data_) + srcStartingByte_, byteAmount_);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer. Static cast to char* is for pointer math(as char is 1 byte exactly)
		vkUnmapMemory(Demo::renderer.mainDevice.logicalDevice, this->stagingMemoryBlock.get());	// Unmap vertexBufferMemory from data
	}
	else
	{
		// Map our vertex data to vertex Buffer 
		void* sharedDataP; // Create an empty typeless pointer.
		vkMapMemory(Demo::renderer.mainDevice.logicalDevice, this->stagingMemoryBlock.get(), dstStartingByte_, byteAmount_, 0, &sharedDataP);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping. 
		memcpy(sharedDataP, static_cast<const char*>(data_) + srcStartingByte_, byteAmount_);  // writes to *GPU memory/Shared memory in Ram* via CPU pointer
		vkUnmapMemory(Demo::renderer.mainDevice.logicalDevice, this->stagingMemoryBlock.get());	// Unmap vertexBufferMemory from data

		// Copy staging buffer to vertex buffer on GPU
		GraphicsUtilities::copyBuffer(queue_, rCommandPool_.get(), this->stagingBuffer.get(), this->buffer.get(), dstStartingByte_, dstStartingByte_, byteAmount_); // both are dstStartingBytes, since both buffers are 1 : 1, so data offset is identical 
	}
}
