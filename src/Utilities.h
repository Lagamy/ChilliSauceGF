// Basically a settings/requirments header  
#pragma once 
#include "PoolId.h"
#include "Bool.h"
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

	static std::string queueToName[3]
	{
		"Graphics",
		"Transfer",
		"Compute"
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
		INDEX = 0, 
		VERTEX = 1,  
		// UNIFORM = 2,
		// STORAGE = 3,
		// TEXTURE = 4
	};

	inline const uint8_t BufferTypesCount = 2; 

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
	
	// Reflection
	enum AlignmentLayoutEnum  : uint8_t // For differentiating GPU layout alignments
	{
		NONE,
		STD140,
		STD430
	};

	enum AlignmentTypeEnum  : uint8_t // For differentiating GPU layout alignment types
	{
		AL_SCALAR,
		AL_VEC2, // Exception as its alignment is 8 bytes, despite being an array
		AL_ARRAY, // Used for internal GLSL array: VEC3, VEC4, MAT2, MAT3, MAT4, MAT2x2, etc.  
		AL_USERARRAY // Used for user authored arrays. Differ in calculation between std140 and std430
	};

	enum DataTypeEnum  : uint8_t
	{ 
		INT32,
		UINT32,
		FLOAT,
		BOOL,
		VEC2, 
		VEC3,
		VEC4, 
		MAT2,
		MAT3, 
		MAT4, 
		MAT2X3,
		MAT2X4, 
		MAT3X2,
		MAT3X4,
		MAT4X2,	
		MAT4X3
	};

	inline constexpr const char* dataTypeToName[]
	{
		"Int",
		"Uint",
		"Float",
		"Bool",
		"Vec2",
		"Vec3",
		"Vec4",
		"Mat2",
		"Mat3",
		"Mat4",
		"Mat2x3",
		"Mat2x4",
		"Mat3x2",
		"Mat3x4",
		"Mat4x2",
		"Mat4x3"
	};

	inline constexpr uint32_t dataTypeToElementCount[]
	{
		1, // INT
		1, // UINT
		1, // FLOAT
		1, // BOOL
		2, // VEC2
		3, // VEC3
		4, // VEC4
		4, // MAT2, 
		9, // MAT3	
		16, // MAT4
		6, // MAT2X3
		8, // MAT2X4
		6, // MAT3X2 
		12, // MAT3X4
		8, // MAT4X2
		12 // MAT4X3
	};

	inline constexpr glm::vec2 dataTypeToColumnsAndRows[]
	{
		{1, 1}, // INT
		{1, 1}, // UINT
		{1, 1}, // FLOAT
		{1, 1}, // BOOL
		{2, 1}, // VEC2
		{3, 1}, // VEC3
		{4, 1}, // VEC4
		{2, 2}, // MAT2, 
		{3, 3}, // MAT3	
		{4, 4}, // MAT4
		{2, 3}, // MAT2X3
		{2, 4}, // MAT2X4
		{3, 2}, // MAT3X2 
		{3, 4}, // MAT3X4
		{4, 2}, // MAT4X2
		{4, 3} // MAT4X3
	};

	inline constexpr VkFormat typeToVkFormat[]
	{
		VK_FORMAT_R32_SINT, // INT32
		VK_FORMAT_R32_UINT, // UINT32
		VK_FORMAT_R32_SFLOAT, // FLOAT
		VK_FORMAT_R32_UINT, // BOOL
		VK_FORMAT_R32G32_SFLOAT, // VEC2T.  GLSL internal vec and mat types. All are a fixed array of floats 
		VK_FORMAT_R32G32B32_SFLOAT, // VEC3T
		VK_FORMAT_R32G32B32A32_SFLOAT// VEC4T
	};

	inline constexpr uint32_t alignmentTypeRules[]  // Exist to simply padd and align data on CPU side - so GPU would read it correctly after upload
	{
		4, // AL_SCALAR
		8, // AL_VEC2
		16 // AL_ARRAY. for vec3/4, mat2/3/4
	};

	inline const uint32_t getAlignmentTypeRule(AlignmentTypeEnum alignmentTypeEnum_)
	{
		return alignmentTypeRules[static_cast<size_t>(alignmentTypeEnum_)];
	};


	template<typename T>
	struct GPUTypeMap; // intentionally undefined

	// template<> tells the compiler: this is a specialization of a template, and not just random structs 
	template<> struct GPUTypeMap<int32_t> { static constexpr DataTypeEnum value = INT32; }; 
	template<> struct GPUTypeMap<uint32_t> { static constexpr DataTypeEnum value = UINT32; };
	template<> struct GPUTypeMap<float> { static constexpr DataTypeEnum value = FLOAT; };
	template<> struct GPUTypeMap<Bool> { static constexpr DataTypeEnum value = BOOL; };

	template<> struct GPUTypeMap<glm::vec2> { static constexpr DataTypeEnum value = VEC2; };
	template<> struct GPUTypeMap<glm::vec4> { static constexpr DataTypeEnum value = VEC4; };
	template<> struct GPUTypeMap<glm::vec3> { static constexpr DataTypeEnum value = VEC3; };
	template<> struct GPUTypeMap<glm::mat2> { static constexpr DataTypeEnum value = MAT2; };
	template<> struct GPUTypeMap<glm::mat3> { static constexpr DataTypeEnum value = MAT3; }; 
	template<> struct GPUTypeMap<glm::mat4> { static constexpr DataTypeEnum value = MAT4; };
	template<> struct GPUTypeMap<glm::mat2x3> { static constexpr DataTypeEnum value = MAT2X3; };
	template<> struct GPUTypeMap<glm::mat2x4> { static constexpr DataTypeEnum value = MAT2X4; };
	template<> struct GPUTypeMap<glm::mat3x2> { static constexpr DataTypeEnum value = MAT3X2; };
	template<> struct GPUTypeMap<glm::mat3x4> { static constexpr DataTypeEnum value = MAT3X4; };
	template<> struct GPUTypeMap<glm::mat4x2> { static constexpr DataTypeEnum value = MAT4X2; };
	template<> struct GPUTypeMap<glm::mat4x3> { static constexpr DataTypeEnum value = MAT4X3; };

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


