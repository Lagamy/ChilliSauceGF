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
#include <vulkan/vulkan_core.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

//#include "DescriptorSetLayout.h"
inline const uint32_t UninitializedId = std::numeric_limits<uint32_t>::max(); 
inline const PoolId UninitializedPoolId = PoolId{UninitializedId, UninitializedId};

inline void EmptyFunction(){}

enum StorageUnitEnum : uint8_t 
{ 
	BYTE = 0,
	KB = 1,
	MB = 2,
	GB = 3
};

const inline uint32_t StorageUnitToBytes[4]
{
	1, 
	1024,
	1048576,
	1073741824
};

inline VkDeviceSize toBytes(size_t size_, StorageUnitEnum unit_)
{
	return size_ << (10 * static_cast<uint8_t>(unit_));
}

namespace Graphics
{
	enum QueueFamilyEnum : uint8_t
	{
		GRAPHICS = 0,
		TRANSFER = 1,
		COMPUTE = 2,
	};

	enum StagingDirectionEnum : uint8_t
	{
		UPLOAD = 0,
		READBACK = 1
	};

	const inline uint8_t StagingDirectionsCount = 2;

	enum MemoryVisabilityEnum : bool 
	{
		GPU_LOCAL,
		CPU_SHARED
	}; 

	const inline std::array<const char*, 2> MemVisabilityTypeToName = {
		"GPU Local", 
		"CPU Shared", 
	};

	const inline uint8_t PresentationQueueId = 3; 

	static std::string queueToName[3]
	{
		"Graphics",
		"Transfer",
		"Compute"
	}; 

	enum CmdLifetimeEnum : bool
	{
		FRAME = false, 
		ONESHOT = true
	};

	enum AttachmentTypeEnum {
		COLOR,
		DEPTH,
		STENCIL,
		DEPTHSTENCIL,
		RESOLVE
	};

	enum AllocatorTypeEnum { 
		UI,
		STATIC, 
		DYNAMIC
	};

	const inline uint8_t AllocatorTypesCount = 3; 

	enum BufferTypeEnum : uint8_t 
	{
		INDEX_BUFFER = 0, 
		VERTEX_BUFFER = 1,  
		UNIFORM_BUFFER = 2,
		STORAGE_BUFFER = 3,
		TEXTURE_BUFFER = 4,
		SAMPLER_BUFFER = 5
	};

	
	enum DescriptorTypeEnum : uint8_t 
	{
		UNIFORM_DESCRIPTOR = 0,
		STORAGE_DESCRIPTOR = 1,
		TEXTURE_DESCRIPTOR = 2,
		SAMPLER_DESCRIPTOR = 3
	};

	inline VkDescriptorType descriptorTypes[4] 
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_SAMPLER 
	};

	inline VkBufferUsageFlagBits BufferTypeToUsage[4]
	{
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	};

	inline const uint8_t BufferTypesCount = 4; // INDEX, VERTEX, UNIFORM, STORAGE, TEXTURE and SAMPLER

	struct MemoryInterval  
	{
		std::vector<PoolId> bufferIds;
		uint64_t start;
		uint64_t end;

		MemoryInterval(uint64_t start_, uint64_t end_) : start(start_), end(end_) {}
	};

	struct MemoryIntervalKey
	{
    	uint32_t memoryBlockId;
    	uint64_t byteId;

    	bool operator==(const MemoryIntervalKey& other) const
    	{
        	return memoryBlockId == other.memoryBlockId
        	    && byteId == other.byteId;
    	}
	};

	struct MemoryIntervalKeyHash
	{
    	std::size_t operator()(const MemoryIntervalKey& key) const
    	{
        	std::size_t h1 = std::hash<uint32_t>{}(key.memoryBlockId);
        	std::size_t h2 = std::hash<uint64_t>{}(key.byteId);

        	return h1 ^ (h2 << 1);
    	}
	};

	// Forward decloration 
	struct Buffer; 
	struct MemoryBlock;
	struct MemoryEntry;

	using SyncRetrivalFunc = PoolId(*)();
	struct DynamicSemaphoreRef
	{
    	SyncRetrivalFunc func; 
    	uint32_t id; 
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
		std::array<int32_t, 4> indices = {-1, -1, -1, -1}; 

		bool isValid() {
			bool valid = true; 
			for(uint8_t i = 0; i < indices.size(); i++)
			{
				if(indices[i] < 0) valid = false; 
			}
			return valid;
		}
	};
	
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device_);
	SwapchainDetails getSwapchainDetails(VkPhysicalDevice device_);

	// Swapchain suitable params functions 
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& rAvailableFormats_);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& rAvailablePresentationModes_);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& rSurfaceCapabilities_); // What size are images can be, based on limitations(our window size)

	// Utills 
	VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment); // Adds missing bytes starting bytes, and end bytes

	// Reflection
	enum STDAlignmentLayoutEnum  : uint8_t // For differentiating GPU layout alignments
	{
		NONE,
		STD140,
		STD430
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

	enum DataShapeEnum : uint8_t 
	{
		SCALAR, 
		ARRAY, 
		MATRIX
	};

	inline constexpr const char* dataShapeToName[]
	{
		"Scalar", 
		"Array",
		"Matrix"
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

	inline constexpr uint32_t dataTypeToInternalElementCount[]
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

	inline constexpr uint8_t dataTypeToBaseAlignment[] // std430 base alignment in bytes. A matrix aligns like one of its columns (a vector of rowCount floats). std140 rounds matrices and arrays up to 16, see resolveSTDPaddingRule
	{
		4, // INT
		4, // UINT
		4, // FLOAT
		4, // BOOL
		8, // VEC2
		16, // VEC3
		16, // VEC4
		8, // MAT2 (columns are vec2)
		16, // MAT3
		16, // MAT4
		16, // MAT2X3
		16, // MAT2X4
		8, // MAT3X2
		16, // MAT3X4
		8, // MAT4X2
		16 // MAT4X3
	};

	uint8_t resolveSTDPaddingRule(STDAlignmentLayoutEnum aligLayout_, DataTypeEnum dataType_, bool isArray_); // Returns the byte alignment a member must start at. dataType_ is the element's type if isArray_

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


namespace Disk
{
	std::string findExecutablePath(); 
	std::vector<char> readFile(const std::string& rFilename_);
    inline std::string executablePath = Disk::findExecutablePath();
}


