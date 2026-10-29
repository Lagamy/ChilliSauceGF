#include "DescriptorPool.h"
#include "Api.h"
#include "Utilities.h"
#include <cstdint>
#include <stdexcept>

namespace  Graphics 
{
    void DescriptorPool::setDescriptorCountPerType(DescriptorTypeEnum descriptorType_, uint32_t count_)
    {
        if(this->initializedDescriptorTypes[descriptorType_] == UninitializedId)
        {
            this->poolSizePerType.emplace_back(); 
            this->initializedDescriptorTypes[descriptorType_] = this->poolSizePerType.size() - 1; 
        }

        uint32_t poolSizeId = this->initializedDescriptorTypes[descriptorType_]; 
        this->poolSizePerType[poolSizeId].descriptorCount = count_; 
        this->poolSizePerType[poolSizeId].type = descriptorTypes[descriptorType_];  
    }

    /* Vulkan requires poolSizeCount > 0, so a pool without any configured descriptor type can't be created. */
    void DescriptorPool::create(uint32_t maxSets_)
    {
	    if(this->poolSizePerType.empty())
	    {
		    throw std::runtime_error("Descriptor Pool: Can't create a pool without descriptors. Set descriptor counts first.");
	    }

	    VkDescriptorPoolCreateInfo poolCreateInfo = {};
	    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    poolCreateInfo.maxSets = maxSets_; 
	    poolCreateInfo.poolSizeCount = this->poolSizePerType.size();
	    poolCreateInfo.pPoolSizes = this->poolSizePerType.data();

	    VkResult result = vkCreateDescriptorPool(getMainDevice().logicalDevice, &poolCreateInfo, nullptr, &this->vkHandle);
	    if (result != VK_SUCCESS)
	    {
		    throw std::runtime_error("Failed to create a Descriptor Pool!");
	    }
    }

    void DescriptorPool::destroy()
    {
	    if(this->vkHandle != VK_NULL_HANDLE)
	    {
		    vkDestroyDescriptorPool(getMainDevice().logicalDevice, this->vkHandle, nullptr);
		    this->vkHandle = VK_NULL_HANDLE;
	    }
    }
}
