#include "Device.h"
#include "Api.h"
#include "Utilities.h"
#include <stdexcept>
#include <set>
#include <vulkan/vulkan_core.h>

namespace Graphics
{
void Device::setup() {
	getPhysicalDevice();
	createLogicalDevice();
}


void Device::destroy() {
	vkDestroyDevice(this->logicalDevice, nullptr);
	this->logicalDevice = VK_NULL_HANDLE;
}


void Device::getPhysicalDevice() {
	// Enumerate Physical devices the vkInstance can access 
	uint32_t deviceCount = 0;
	// Get number of physical devices, so we can create an array of a correct size. 
	vkEnumeratePhysicalDevices(getInstance().get(), &deviceCount, nullptr);

	// If no devices available, then none support Vulkan! 
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPU that supports Vulkan Instance.");
	}

	// Get a list of all available physical devices
	std::vector<VkPhysicalDevice> availablePhysicalDeviceList(deviceCount);
	vkEnumeratePhysicalDevices(getInstance().get(), &deviceCount, availablePhysicalDeviceList.data());

	for (const auto& device : availablePhysicalDeviceList)
	{
		if (checkDeviceSuitable(device))
		{
			this->physicalDevice = device;
			break;
		}
	}
}


void Device::createLogicalDevice()
{
    this->queueFamilyIndices = getQueueFamilies(this->physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // Remove duplicate queue families.
    std::set<uint32_t> uniqueQueueFamilies(
        this->queueFamilyIndices.indices.begin(),
        this->queueFamilyIndices.indices.end()
    );

    float priority = 1.0f;

    for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &priority;

        queueCreateInfos.emplace_back(queueCreateInfo);
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceCreateInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    deviceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(requiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames =
        requiredDeviceExtensions.data();

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VkResult result = vkCreateDevice(
        this->physicalDevice,
        &deviceCreateInfo,
        nullptr,
        &this->logicalDevice
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Logical Device");
    }

    vkGetDeviceQueue(
        this->logicalDevice,
        this->queueFamilyIndices.indices[GRAPHICS],
        0,
        &this->queues[GRAPHICS]
    );

    vkGetDeviceQueue(
        this->logicalDevice,
        this->queueFamilyIndices.indices[TRANSFER],
        0,
        &this->queues[TRANSFER]
    );

    vkGetDeviceQueue(
        this->logicalDevice,
        this->queueFamilyIndices.indices[COMPUTE],
        0,
        &this->queues[COMPUTE]
    );

    vkGetDeviceQueue(
        this->logicalDevice,
        this->queueFamilyIndices.indices[PresentationQueueId],
        0,
        &this->queues[PresentationQueueId]
    );
}


bool Device::checkDeviceSuitable(VkPhysicalDevice device_) {
	/*
	// Information about the device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	QueueFamilyIndices indices = getQueueFamilies(device_);
	bool extensionsSupported = checkDeviceExtensionsSupport(device_);
	SwapchainDetails swapchainDetails = getSwapchainDetails(device_);
	bool swapchainValid = !swapchainDetails.imageFormats.empty() && !swapchainDetails.presentationModes.empty(); // Is Swapchain with our params - possible to create on that device. 
	return indices.isValid() && extensionsSupported && swapchainValid;
}


bool Device::checkDeviceExtensionsSupport(VkPhysicalDevice _device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

	// if no extensions found - return failure 
	if (extensionCount == 0)
	{
		return false;
	}

	std::vector<VkExtensionProperties> allSupportedExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, allSupportedExtensions.data());
	for (const auto& requiredExtension : requiredDeviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : allSupportedExtensions)
		{
			if (strcmp(requiredExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		// If even 1 of required extensions wasn't supported - return failure. 
		if (!hasExtension) return false;
	}
	return true;
}

QueueFamilyIndices Device::getQueueFamilies(VkPhysicalDevice & rDevice)
{
	QueueFamilyIndices indicies;

	// Get all Queue Family Property's info for given device
	uint32_t queueFamiliesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(rDevice, &queueFamiliesCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(rDevice, &queueFamiliesCount, queueFamilyList.data());

	// Go through each family, and check if it has at least 1 of the required types of queues

	int i = 0; // manual index for foreach.
	for (const auto& queueFamily : queueFamilyList)
	{
		// Check if queue family has at least 1 queue 
		if (queueFamily.queueCount > 0)
		{
			// Queue can be multiple types, defined by bitfield(flags). Use biwise AND. 
			// Graphics Queue family index search
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indicies.indices[GRAPHICS] = i;
			}

			// Transfer Queue family index search
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				indicies.indices[TRANSFER] = i;
			}

			// Compute Queue family index search
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				indicies.indices[COMPUTE] = i;
			}

			// Presentation capable Queue index search
			VkBool32 presentationSupport = VK_FALSE;

			// Check if [i] Queue of this Device supports Surfaces 
			vkGetPhysicalDeviceSurfaceSupportKHR(rDevice, i, getSurface().get(), &presentationSupport);
			if (presentationSupport == VK_TRUE)
			{
				indicies.indices[PresentationQueueId] = i;
			}
		}

		if (indicies.indices[TRANSFER] == -1) // Default to graphics queue
		{
			indicies.indices[TRANSFER] = indicies.indices[GRAPHICS]; 
		}

		if (indicies.indices[COMPUTE] == -1) // Default to graphics queue
		{
			indicies.indices[COMPUTE] = indicies.indices[GRAPHICS];
		}

		// Check if all required queues were found. 
		if (indicies.isValid())
		{
			break;
		}
		i++;
	}
	return indicies;
}
};
