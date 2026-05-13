#include "GPUMemoryManager.h"


uint32_t GPUMemoryManager::addEntry(const char* name_, size_t size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, bool cpuVisible_) 
{
	return this->memoryEntries.add(PhysicalGPUBuffer(name_, size_, bufferUsageFlags_, bufferSharingMode_, cpuVisible_));
}

void GPUMemoryManager::removeEntry(uint32_t id_) 
{
	this->memoryEntries.remove(id_);
}

void GPUMemoryManager::upload(uint32_t entryId_, const void* data_, VkQueue queue_, CommandPool& rCommandPool_) // full upload
{
	this->memoryEntries.get(entryId_).upload(data_, queue_, rCommandPool_); 
}

void GPUMemoryManager::upload(uint32_t entryId_, const void* data_, VkQueue queue_, CommandPool& rCommandPool_, size_t byteAmount_, size_t srcStartingByte_, size_t dstStartingByte_) // partial upload
{
	this->memoryEntries.get(entryId_).upload(data_, queue_, rCommandPool_, byteAmount_, srcStartingByte_, dstStartingByte_); 
}

PhysicalGPUBuffer& GPUMemoryManager::getEntry(uint32_t id_) 
{
	return this->memoryEntries.get(id_); 	
} 
