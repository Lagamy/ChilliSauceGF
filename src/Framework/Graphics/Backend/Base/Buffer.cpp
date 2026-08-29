#include "Buffer.h"
#include "Api.h"

namespace Graphics
{
	void Buffer::create(VkDeviceSize bufferSize_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, const char* name_)
	{
		if(bufferSize_ != 0)
		{
			VkBufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = bufferSize_;	// Size of buffer
			bufferCreateInfo.usage = bufferUsageFlags_; // Creating buffer for multiple usage goals is possible. (Transfer recieve and Vertex for example)
			bufferCreateInfo.sharingMode = bufferSharingMode_; // Can it share with multiple queue families or not(VK_SHARING_MODE_EXCLUSIVE = not) 

			// TODO: Inegrate an engine wide error context provider(So name of component pops up - which was unable to create buffer)     
			VkResult result = vkCreateBuffer(getMainDevice().logicalDevice, &bufferCreateInfo, nullptr, &this->vkHandle);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error(std::format("Failed to create {} Buffer!", name_));
			}
			vkGetBufferMemoryRequirements(getMainDevice().logicalDevice, this->vkHandle, &memoryReqs); 
		}
	}

	void Buffer::destroy()
	{
		if (this->vkHandle != VK_NULL_HANDLE)
    	{
			vkDestroyBuffer(getMainDevice().logicalDevice, this->vkHandle, nullptr); // Destroys on GPU side 
			this->vkHandle = VK_NULL_HANDLE;
		}
	}

	VkBuffer Buffer::get() const
	{
		return this->vkHandle;
	}
}
