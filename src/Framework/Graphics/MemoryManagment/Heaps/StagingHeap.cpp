#include "StagingHeap.h"
#include "Api.h"
#include "Utilities.h"

namespace Graphics
{
void StagingHeap::createStatic()
{
	std::array<VkMemoryRequirements, 1> memRequirement;
	this->buffer.create(this->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, "Staging Buffer"); 

	// Allocate memory
	vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffer.get(), &memRequirement[0]);
	this->memoryBlock.createForStaticOrStaging(this->size, memRequirement, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, CPU_SHARED, true);

	// Bind buffer to memory block 
	vkBindBufferMemory(getMainDevice().logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
};

void StagingHeap::create(VkDeviceSize size_)
{
	std::array<VkMemoryRequirements, 1> memRequirement;
	this->buffer.create(this->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, "Staging Buffer"); 

	// Allocate memory
	vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffer.get(), &memRequirement[0]);
	this->memoryBlock.createForStaticOrStaging(this->size, memRequirement, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, CPU_SHARED, true);

	// Bind buffer to memory block 
	vkBindBufferMemory(getMainDevice().logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
}

void StagingHeap::destroy()
{
	this->memoryBlock.destroy(); 
	this->buffer.destroy();
}
}