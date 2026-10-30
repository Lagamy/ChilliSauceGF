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

			/*
				A CONCURRENT buffer has to list each queue family that uses it once (spec: VkBufferCreateInfo, pQueueFamilyIndices): graphics, transfer and compute, the ones that touch GPU_LOCAL buffers.
			*/
			uint32_t queueFamilyIndices[3];
			uint32_t queueFamilyCount = 0;
			if(bufferSharingMode_ == VK_SHARING_MODE_CONCURRENT)
			{
				for(uint8_t family = GRAPHICS; family <= COMPUTE; family++)
				{
					uint32_t familyIndex = (uint32_t)getMainDevice().queueFamilyIndices.indices[family];
					bool listed = false;
					for(uint32_t i = 0; i < queueFamilyCount; i++)
					{
						listed = listed || queueFamilyIndices[i] == familyIndex;
					}
					if(!listed)
					{
						queueFamilyIndices[queueFamilyCount++] = familyIndex;
					}
				}
				bufferCreateInfo.queueFamilyIndexCount = queueFamilyCount;
				bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
			}

			// TODO: Inegrate an engine wide error context provider(So name of component pops up - which was unable to create buffer)     
			VkResult result = vkCreateBuffer(getMainDevice().logicalDevice, &bufferCreateInfo, nullptr, &this->vkHandle);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error(std::format("Failed to create Buffer! For upload {}", name_));
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

	Buffer::Buffer(VkDeviceSize size_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, const char* name_)
	{
		this->create(size_, bufferUsageFlags_, bufferSharingMode_, name_);
	}
}
