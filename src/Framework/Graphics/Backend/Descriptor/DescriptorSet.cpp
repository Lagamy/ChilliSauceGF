#include "DescriptorSet.h"
#include "Api.h"

namespace Graphics 
{   
    void DescriptorSet::create()
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
    }   
}