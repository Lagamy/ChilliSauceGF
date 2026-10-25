#include "StagingHeap.h"
#include "Api.h"
#include "Utilities.h"

namespace Graphics
{
void StagingHeap::create(VkDeviceSize size_, StagingDirectionEnum direction_)
{
	this->size = size_; 
	std::array<VkMemoryRequirements, 1> memRequirement;

	/*
		UPLOAD heaps are copy sources in coherent memory the CPU only writes. READBACK heaps are copy destinations in HOST_CACHED memory, since reading write-combined memory is very slow.
		HOST_CACHED memory is not necessarily HOST_COHERENT, so a coherent+cached type is preferred (readers need no invalidate) and plain cached is the fallback.
		MemoryBlock::isCoherent tells readers whether they must call vkInvalidateMappedMemoryRanges before reading.
	*/
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	const char* name = "Staging Buffer";
	if(direction_ == READBACK)
	{
		usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		name = "Readback Buffer";
	}
	this->buffer.create(this->size, usage, VK_SHARING_MODE_EXCLUSIVE, name); 

	/* The driver may round the buffer's size up. The allocation has to cover memRequirement.size, not just the requested size, or the bind below is invalid. */
	// Allocate memory
	vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->buffer.get(), &memRequirement[0]);
	if(direction_ == READBACK && this->memoryBlock.tryFindMemoryTypeIndex(memRequirement, properties | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) >= 0)
	{
		properties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
	this->memoryBlock.createForStaticOrStaging(memRequirement[0].size, memRequirement, properties, CPU_SHARED, true);

	// Bind buffer to memory block 
	vkBindBufferMemory(getMainDevice().logicalDevice, this->buffer.get(), this->memoryBlock.get(), 0);
	this->created = true; 
}

void StagingHeap::destroy()
{
	this->memoryBlock.destroy(); 
	this->buffer.destroy();
	this->created = false; 
}

StagingHeap::StagingHeap(uint64_t size_, StagingDirectionEnum direction_)
{
	this->create(size_, direction_); 
} 
}