#include "DescriptorSetLayout.h"
#include "Api.h"
#include <vulkan/vulkan_core.h>

namespace Graphics 
{
    DescriptorBlueprint& DescriptorSetLayout::getDescriptorBlueprint(PoolId id_)
    {

    } 
	
	PoolId DescriptorSetLayout::addDescriptorBlueprint(const char* name_)
    {
         if(this->vkHandle != VK_NULL_HANDLE)
        {
            // Recreate all Pipeline Layouts and Pipelines themself that use
        }
    }

	void DescriptorSetLayout::removeDescriptorBlueprint(PoolId id_)
    {

    } 

	void DescriptorSetLayout::create()
    {
        
    }

	void DescriptorSetLayout::destroy()
    {
        if(this->vkHandle != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(getMainDevice().logicalDevice, this->vkHandle, nullptr); 
        }
    }
}