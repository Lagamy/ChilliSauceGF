#include "DescriptorPool.h"
#include "Api.h"
#include "Utilities.h"
#include <cstdint>

namespace  Graphics 
{
    void DescriptorPool::setDescriptorCountPerType(DescriptorTypeEnum descriptorType_, uint32_t count_)
    {
        if(this->initializedDescriptorTypes[descriptorType_] == UninitializedId)
        {
            this->poolSizePerType.emplace_back(); 
            this->initializedDescriptorTypes[descriptorType_] = this->poolSizePerType.size() - 1; 
        }

        uint8_t descriptorTypeId = this->initializedDescriptorTypes[descriptorTypeId]; 
        this->poolSizePerType[descriptorTypeId].descriptorCount = count_; 
        this->poolSizePerType[descriptorTypeId].type = descriptorTypes[descriptorType_];  
    }

    void DescriptorPool::create(uint32_t maxSets_)
    {
	    // Data to create Descriptor Pool
	    VkDescriptorPoolCreateInfo poolCreateInfo = {};
	    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    poolCreateInfo.maxSets = maxSets_; 
	    poolCreateInfo.poolSizeCount = this->poolSizePerType.size(); // Amount of Pool Sizes being passed
	    poolCreateInfo.pPoolSizes = this->poolSizePerType.data(); // Pool Sizes to create pool with


	    // Create Descriptor Pool
	    VkResult result = vkCreateDescriptorPool(getMainDevice().logicalDevice, &poolCreateInfo, nullptr, &this->vkHandle);
	    if (result != VK_SUCCESS)
	    {
		    throw std::runtime_error("Failed to create a Descriptor Pool!");
	    }
    }

    void DescriptorPool::destroy()
    {
	    vkDestroyDescriptorPool(getMainDevice().logicalDevice, this->vkHandle, nullptr);
    }
}
 