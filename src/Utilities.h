// Basically a settings/requirments header  
#pragma once 
#include "PoolId.h"
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <limits>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <concepts>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

//#include "DescriptorSetLayout.h"
inline const uint32_t UninitializedId = std::numeric_limits<uint32_t>::max(); 
inline const PoolId UninitializedPoolId = PoolId{UninitializedId, UninitializedId};
namespace Graphics
{
	enum QueueFamilyEnum : uint8_t
	{
		GRAPHICS = 0,
		TRANSFER = 1,
		COMPUTE = 2
	};

	

	enum CommandPoolTypeEnum 
	{
		FRAME, 
		ONESHOT
	};

	enum AttachmentTypeEnum {
		COLOR,
		DEPTH,
		STENCIL,
		DEPTHSTENCIL,
		RESOLVE
	};

	enum AllocatorTypeEnum { 
		STATIC 
	};

	enum BufferTypeEnum : uint8_t 
	{
		VERTEX = 0, 
		INDEX = 1, 
		// UNIFORM = 2,
		// STORAGE = 3,
		// TEXTURE = 4
	};


	/* Helper structs */

	struct SwapchainDetails {
		VkSurfaceCapabilitiesKHR surfaceCapabilities; // Surface properties(image size/extent, alpha compositing, etc)
		std::vector<VkSurfaceFormatKHR> imageFormats; // Surface Image format(Size of each color, order of channels, normalized or not, etc.) + Color space
		std::vector<VkPresentModeKHR> presentationModes; // How images should be swaped/presented on the screen. 
	};
	
	// enum class ColorFormatsEnum {
		// VK_FORMAT_R8_UNORM = VkFormat::VK_FORMAT_R8_UNORM,
		// VK_FORMAT_R8G8_UNORM = VkFormat::VK_FORMAT_R8G8_UNORM,
		// VK_FORMAT_R8G8B8_UNORM = VkFormat::VK_FORMAT_R8G8B8_UNORM,
		// VK_FORMAT_B8G8R8_UNORM = VkFormat::VK_FORMAT_B8G8R8_UNORM,
		// VK_FORMAT_R8G8B8A8_UNORM = VkFormat::VK_FORMAT_R8G8B8A8_UNORM,
		// VK_FORMAT_B8G8R8A8_UNORM = VkFormat::VK_FORMAT_B8G8R8A8_UNORM,
		// VK_FORMAT_R16G16B16A16_SFLOAT = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
		// VK_FORMAT_R32G32B32A32_SFLOAT = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT
	// };	

	// enum class DepthStencilFormatsEnum {
		// VK_FORMAT_D24_UNORM_S8_UINT = VkFormat::VK_FORMAT_D24_UNORM_S8_UINT, // 24‑bit depth + 8‑bit stencil
		// VK_FORMAT_D32_SFLOAT_S8_UINT = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT // 32‑bit float depth + 8‑bit stencil
	// };

	// enum class DepthFormatsEnum {
		// VK_FORMAT_D16_UNORM = VkFormat::VK_FORMAT_D16_UNORM, // 16‑bit unsigned normalized depth
		// VK_FORMAT_D32_SFLOAT = VkFormat::VK_FORMAT_D32_SFLOAT // 32‑bit float depth
	// };
	
	/* Helper methods */
	// Device suitable params functions 
	// indicies (locations) of Queue Families (if they exist at all);
	struct QueueFamilyIndices {
		int32_t graphicsFamily = -1;	 // Location of Graphics Queue Family(includes Transfer capabilities by Vulkan Standard)
		int32_t presentationFamily = -1; // Location of Presentation Queue Family(not a real Queue Family, more of "Queue that supports Presentation")
		int32_t transferFamily = -1;
		int32_t computeFamily = -1;

		bool isValid() {
			return this->graphicsFamily >= 0 && this->presentationFamily >= 0 && this->transferFamily >= 0 && this->computeFamily >= 0;
		}
		// Todo: if there are no Transfer or Compute queues - assign their indices to available Graphics queue, so further code works anyways, just on one queue. 
	};
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device_);
	SwapchainDetails getSwapchainDetails(VkPhysicalDevice device_);

	// Swapchain suitable params functions 
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& rAvailableFormats_);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& rAvailablePresentationModes_);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& rSurfaceCapabilities_); // What size are images can be, based on limitations(our window size)

	// Utills 
	// Write Descriptor Set Data(By binding it to a Buffer with that data).
	void writeBufferDescriptorSet(VkDescriptorSet& rSet, VkBuffer& rBuffer, VkDeviceSize _dataSize, uint32_t _binding, VkDescriptorType _descriptorType, uint32_t _arrayElement);
	VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment); // Adds missing bytes starting bytes, and end bytes

	inline std::vector<const char*> requiredDeviceExtensions = { // If you choose Ray tracing to be enabled -> this would change
            VK_KHR_SWAPCHAIN_EXTENSION_NAME	
	};


	inline uint64_t assetDelayBeforeUnload; // Can change between scenes.(Flexability). Used for assets with STREAMING lifetimeType


}

// namespace DebugUtilities
// {
	// // For debugging  
	// template<typename T>
	// std::concept Streamable = requires(std::ostream & os, const T & v)
	// {
		// { os << v } -> std::same_as<std::ostream&>;
	// };

	// template<Streamable T>
	// void elementToStrStream(const T& rElement_, std::ostringstream& rOss_)
	// {
		// rOss_ << rElement_ << ' ';
	// }

	// template<glm::length_t L, typename T, glm::qualifier Q>
	// void elementToStrStream(const glm::vec<L, T, Q>& rElement_, std::ostringstream& rOss_)
	// {
		// for (size_t i = 0; i < L; i++)
		// {
			// rOss_ << rElement_[i] << ' ';
		// }
	// }

	// template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
	// void elementToStrStream(const glm::mat<C, R, T, Q>& rElement_, std::ostringstream& rOss_)
	// {
		// rOss_ << "\n";
		// for (size_t i = 0; i < C; i++)
		// {
			// for (size_t j = 0; j < R; j++)
			// {

				// rOss_ << rElement_[i][j] << ' ';
			// }
			// if (i < C - 1)
			// {
				// rOss_ << "\n";
			// }
		// }
	// }

	// template<typename T>
	// void vectorToStrStream(const std::vector<T>& rVector_, std::ostringstream& rOss_)
	// {
		// for (const auto& element : rVector_)
		// {
			// elementToStrStream(element, rOss_);
		// }
	// }

	// template<typename T>
	// void matrixToStrStream(const Matrix<T>& rMatrix_, std::ostringstream& rOss_)
	// {
		// rOss_ << "\n";
		// for (size_t i = 0; i < rMatrix_.getRowCount(); i++)
		// {
			// for (size_t j = 0; j < rMatrix_.getColumnCount(); j++)
			// {
				// elementToStrStream(rMatrix_.getElement(i, j), rOss_);
			// }
			// rOss_ << "\n";
		// }
	// }
// }


namespace Disk
{
	std::string findExecutablePath(); 
	std::vector<char> readFile(const std::string& rFilename_);
    inline std::string executablePath = Disk::findExecutablePath();
}



