#include "DescriptorSet.h"
#include "Api.h"
#include "Descriptor.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics 
{   
    void DescriptorSet::create(AllocatorTypeEnum bufferAllocatorType_)
    {
	    // Descriptor Set Allocation Info
	    VkDescriptorSetAllocateInfo setAllocInfo = {};
	    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	    setAllocInfo.descriptorPool = this->pool.vkHandle;									// Pool to allocate Descriptor Set from
	    setAllocInfo.descriptorSetCount = 1;	// Number of sets to allocate
	    setAllocInfo.pSetLayouts = &getDescriptorsManager().layouts[this->setLayout].vkHandle;									

	    // Allocate descriptor sets (multiple)
	    VkResult result = vkAllocateDescriptorSets(getMainDevice().logicalDevice, &setAllocInfo, &this->vkHandle);
	    if (result != VK_SUCCESS)
	    {
		    throw std::runtime_error("Failed to allocate Descriptor Sets!");
	    }


		// TODO: upload all Descriptors to mem using memory manager 


	    // Allocate memory for buffers + update all of descriptor set buffer bindings
		// Buffer info and data offset info
		std::vector<VkWriteDescriptorSet> updateSetWrites; 
		for(uint32_t i = 0; i < this->descriptors.size(); i++)
		{
			if(this->descriptors.alive[i]) // This descriptor(even tho it uninitialized exist in the Pool)
			{
				// Allocate
				Descriptor& rDescriptor = this->descriptors.objects[i]; 	
				allocateMemoryEntry("", bufferAllocatorType_, rDescriptor.memoryVisability, static_cast<BufferTypeEnum>(this->descriptorType + 2), rDescriptor.getBlueprint().layoutId, 1);


				// VkDescriptorBufferInfo mvpBufferInfo = {};
				// mvpBufferInfo.buffer = getMemoryManager().getBuffer(const MemoryEntry &rEntry_);		// Buffer to get data from
				// mvpBufferInfo.offset = 0;		// Position of start of data
				// mvpBufferInfo.range = sizeof();	// Size of data

				// // Data about connection between binding and buffer
				// VkWriteDescriptorSet mvpSetWrite = {};
				// mvpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				// mvpSetWrite.dstSet = descriptorSets[i];								// Descriptor Set to update
				// mvpSetWrite.dstBinding = 0;											// Binding to update (matches with binding on layout/shader)
				// mvpSetWrite.dstArrayElement = 0;									// Index in array to update
				// mvpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// Type of descriptor
				// mvpSetWrite.descriptorCount = 1;									// Amount to update
				// mvpSetWrite.pBufferInfo = &mvpBufferInfo;							// Information about buffer data to bind
			}
		}
		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(getMainDevice().logicalDevice, updateSetWrites.size(), updateSetWrites.data(), 0, nullptr);
	
    }   
}