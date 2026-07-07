#include "Device.h"
#include "Api.h"
#include <stdexcept>
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


void Device::createLogicalDevice() {
	// Get queue family indices for the chosen Physical Device
	this->queueFamilyIndices = getQueueFamilies(this->physicalDevice);

	// Queues the logical device needs to create, and info to do so(Only 1 now, will add more later!)
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int32_t> neededQueueIndexes = { this->queueFamilyIndices.graphicsFamily, this->queueFamilyIndices.presentationFamily }; // So if some family indices are pointing to the same Queue - We wouldn't create it multiple times.

	for (int queueFamilyIndex : neededQueueIndexes)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;											// Number of queues to create 
		float priority = 1.0;													// Which queue is used first, second, etc. Normalized
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.emplace_back(queueCreateInfo);
	}

	// Info to create Logical Device
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	// Queues
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(neededQueueIndexes.size()); // Number of queue CreateInfos
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data(); // List of queue infos, so device can create required queues

	// Logical Device extensions
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()); // Number of enabled Logical Device Extensions(Swapchain, RTX, DSSL, etc) 
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data(); // List of enabled Extensions 

	// Physical Device Features - the Logical Device will be using 
	VkPhysicalDeviceFeatures deviceFeatures = {};
	//deviceFeatures.depthClamp = VK_TRUE; Enable - if you want to enable clamping in Rasterizer 
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures; // default, as empty. 

	VkResult result = vkCreateDevice(this->physicalDevice, &deviceCreateInfo, nullptr, &this->logicalDevice); // third arg - allocator
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Logical Device");
	}

	// Queues are created at the same time as the device..
	// So we want handle to queues
	// From our logical device, of given Queue Family, of given Queue Index(0, since we only got one), place reference in given VkQueue
	vkGetDeviceQueue(this->logicalDevice, this->queueFamilyIndices.graphicsFamily, 0, &this->queues.graphicsQueue);
	vkGetDeviceQueue(this->logicalDevice, this->queueFamilyIndices.transferFamily, 0, &this->queues.transferQueue); // Maybe same as graphics queue(depends on device)
	// vkGetDeviceQueue(this->logicalDevice, this->queueFamilyIndices.commandFamily, 0, &this->queues.commandQueue);
	vkGetDeviceQueue(this->logicalDevice, this->queueFamilyIndices.presentationFamily, 0, &this->queues.presentQueue);
	
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
				indicies.graphicsFamily = i;
			}

			// Transfer Queue family index search
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				indicies.transferFamily = i;
			}

			// Compute Queue family index search
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				indicies.computeFamily = i;
			}

			// Presentation capable Queue index search
			VkBool32 presentationSupport = VK_FALSE;

			// Check if [i] Queue of this Device supports Surfaces 
			vkGetPhysicalDeviceSurfaceSupportKHR(rDevice, i, getSurface().get(), &presentationSupport);
			if (presentationSupport == VK_TRUE)
			{
				indicies.presentationFamily = i;
			}
		}

		if (indicies.transferFamily == -1) // Default to graphics queue
		{
			indicies.transferFamily = indicies.graphicsFamily; 
		}

		if (indicies.computeFamily == -1) // Default to graphics queue
		{
			indicies.computeFamily = indicies.graphicsFamily;
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
