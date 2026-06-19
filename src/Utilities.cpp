#include "Utilities.h"
#include "Globals.h"
#include "Pool.h"
#include "Semaphore.h"
#include <vulkan/vulkan_core.h>

/* Device filters */
	namespace Graphics
	{
		QueueFamilyIndicies getQueueFamilies(VkPhysicalDevice device_)
		{
			QueueFamilyIndicies indicies;

			// Get all Queue Family Property's info for given device
			uint32_t queueFamiliesCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device_, &queueFamiliesCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamiliesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device_, &queueFamiliesCount, queueFamilyList.data());

			// Go through each family, and check if it has at least 1 of the required types of queues

			int i = 0; // manual index for foreach.
			for (const auto& queueFamily : queueFamilyList)
			{
				// Check if queue family has at least 1 queue 
				if (queueFamily.queueCount > 0)
				{
					// Graphics Queue search
					// Queue can be multiple types, defined by bitfield(flags). Use biwise AND. 
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						indicies.graphicsFamily = i;
					}

					// Presentation Queue search 
					VkBool32 presentationSupport = VK_FALSE;

					// Check if [i] Queue of this Device supports Surfaces 
					vkGetPhysicalDeviceSurfaceSupportKHR(device_, i, Demo::renderer.surface.get(), &presentationSupport);
					if (presentationSupport == VK_TRUE)
					{
						indicies.presentationFamily = i;
					}
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

		SwapchainDetails getSwapchainDetails(VkPhysicalDevice device_)
		{
			SwapchainDetails swapchainDetails;

			// Getting surface Capabilities for given Surface on the given Physical Device 
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_, Globals::renderer.surface.get(), &swapchainDetails.surfaceCapabilities);

			// Getting surface Image Formats for given Surface on the given Physical Device  
			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device_, Globals::renderer.surface.get(), &formatCount, nullptr);
			if (formatCount > 0) {
				swapchainDetails.imageFormats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device_, Globals::renderer.surface.get(), &formatCount, swapchainDetails.imageFormats.data());
			}

			// Getting Presentation Modes for given Surface on the given Physical Device  
			uint32_t presentationModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device_, Globals::renderer.surface.get(), &presentationModeCount, nullptr);
			if (presentationModeCount > 0)
			{
				swapchainDetails.presentationModes.resize(presentationModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device_, Globals::renderer.surface.get(), &presentationModeCount, swapchainDetails.presentationModes.data());
			}
			return swapchainDetails;
		}


		/* Swapchain params filters */

		// Best format is subjective(depends on renderer needs), for my it will be: 
		// format		: VK_FORMAT_R8G8B8A8_SRGB (SRGB here just means Gamma Correction for monitors)  
		// color space	: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& rAvailableFormats)
		{
			if (rAvailableFormats.size() == 1 && rAvailableFormats[0].format == VK_FORMAT_UNDEFINED) // Means all formats are available, no restrictions (Vulkan API quirk) 
			{
				return { VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}

			// If restricted - search for my prefered format. 
			for (const auto& format : rAvailableFormats)
			{
				if ((format.format == VK_FORMAT_R8G8B8A8_SRGB || format.format == VK_FORMAT_R8G8B8A8_UNORM) && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // UNORM here is a backup, if SRGB isn't available(But surface image will be darker)  
				{
					return format;
				}
			}
			// Return first, if can't find our prefered format and colorspace. 
			return rAvailableFormats[0];
		}

		VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& rAvailablePresentationModes)
		{
			for (const auto& presentationMode : rAvailablePresentationModes)
			{
				if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) // If GPU finishes a new frame before v-blank, it replaces the previous frame in the queue with new one.
				{
					return presentationMode;
				}
			}

			// If can't find mailbox - use FIFO, as Vulkan spec says it always must be present. 
			return VK_PRESENT_MODE_FIFO_KHR; // The GPU always waits for the next v-blank before showing a new image. (Shows every image)
		}

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& rSurfaceCapabilities)
		{
			// If extent is at numeric limit - it can vary. Otherwise it just the size of the window. 
			if (rSurfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
			{
				return rSurfaceCapabilities.currentExtent;
			}
			else
			{
				// If value can vary - we need to set it manually 
				int width, height;
				glfwGetFramebufferSize(Globals::appWindow, &width, &height); // Get inner window size   

				VkExtent2D newExtent = {};
				newExtent.width = static_cast<uint32_t>(width);
				newExtent.height = static_cast<uint32_t>(height);

				// Surface also defines max and min, so im clamping it to make sure - its within the boundaries 
				newExtent.width = std::max(rSurfaceCapabilities.minImageExtent.width, std::min(rSurfaceCapabilities.maxImageExtent.width, newExtent.width));
				newExtent.height = std::max(rSurfaceCapabilities.minImageExtent.height, std::min(rSurfaceCapabilities.maxImageExtent.height, newExtent.height));

				return newExtent;
			}
		}

		// Utills
		
		// void writeBufferDescriptorSet(VkDescriptorSet set_, VkBuffer buffer_, VkDeviceSize dataSize_, uint32_t _binding, VkDescriptorType _descriptorType, uint32_t _arrayElement)
		// {
			// // Describes Buffer
			// VkDescriptorBufferInfo bufferInfo = {};
			// bufferInfo.buffer = rBuffer;
			// bufferInfo.offset = 0; // Offset to data. TODO: make it an optional param later on 
			// bufferInfo.range = _dataSize; // Size of data.  

			// // Describe Descriptor Set
			// VkWriteDescriptorSet setWrite = {};
			// setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			// setWrite.dstSet = rSet; // Which descriptor Set to bind
			// setWrite.dstBinding = _binding; // Which binding in shader
			// setWrite.dstArrayElement = 0; // Which element in array to update
			// setWrite.pBufferInfo = &bufferInfo; // Buffer Info to bind and get data from

			// // Update Descriptor Set with new Binding info. (Improve later on) 
			// vkUpdateDescriptorSets(Engine::renderer.mainDevice.logicalDevice, 1, &setWrite, 0, nullptr);
		// }
	
	}

	namespace Disk
	{
		std::vector<char> readFile(const std::string& rFilename) {
			// Open stream from given file 
			std::ifstream file(rFilename, std::ios::binary | std::ios::ate); // ate - move char pointer at end of the file(to identify file size via positions of char at the end). So it starts reading file from end 

			// Check if file stream has successfully opened
			if (!file.is_open())
			{
				throw std::runtime_error("Failed to open the file: " + rFilename);
			}
			// Get current read position(at end, since std::ios::ate) - and save it as size of our file 
			size_t fileSize = (size_t)file.tellg();
			std::vector<char> fileBuffer(fileSize);

			// Set read position to the start of the file  
			file.seekg(0);


			file.read(fileBuffer.data(), fileSize);
			file.close();
			return fileBuffer;
		}
	}
