#include "Buffer.h"
#include "Globals.h"

void Buffer::create(VkDeviceSize bufferSize_, StorageUnitEnum unit_, VkBufferUsageFlags bufferUsageFlags_, VkSharingMode bufferSharingMode_, const char* name_)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = this->toBytes(bufferSize_, unit_);	// Size of buffer
	bufferCreateInfo.usage = bufferUsageFlags_; // Creating buffer for multiple usage goals is possible. (Transfer recieve and Vertex for example)
	bufferCreateInfo.sharingMode = bufferSharingMode_; // Can it share with multiple queue families or not(VK_SHARING_MODE_EXCLUSIVE = not) 

	// TODO: Inegrate an engine wide error context provider(So name of component pops up - which was unable to create buffer)     
	VkResult result = vkCreateBuffer(Demo::renderer.mainDevice.logicalDevice, &bufferCreateInfo, nullptr, &this->vkHandle);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(std::format("Failed to create {} Buffer!", name_));
	}
}

void Buffer::destroy()
{
	vkDestroyBuffer(Demo::renderer.mainDevice.logicalDevice, this->vkHandle, nullptr); // Destroys on GPU side 
	this->vkHandle = VK_NULL_HANDLE;
}

Buffer::~Buffer()
{
	if (this->vkHandle != VK_NULL_HANDLE)
    {
        this->destroy();
    }
}

VkBuffer Buffer::get() const
{
	return this->vkHandle;
}


VkDeviceSize toBytes(uint64_t size_, StorageUnitEnum unit_)
{
	return size_ << (10 * static_cast<uint8_t>(unit_));
}
