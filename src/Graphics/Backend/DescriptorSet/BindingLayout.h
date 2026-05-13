// Derives from DSTemplateEntry, filled by User

#pragma once
#include "Sampler.h"
#include "Asset.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

enum ResourceSourceEnum { 
	ASSET,
	DESCRIPTOR_SET
};

enum ResourceTypeEnum {
	BUFFER_RESOURCE,
	IMAGE_RESOURCE
};

constexpr ResourceSourceEnum resourceTypeToSource[2] = { 
	DESCRIPTOR_SET, // BUFFER
	ASSET // IMAGE
}; 

struct BindingLayout {
private:
	VkDescriptorSetLayoutBinding layout{};
public:
	ResourceTypeEnum resourceType; 
	
	// If this DS created the resource(like arbitrary uniform/storage buffers)
	uint32_t bufferMemoryEntryId;  
	
	// If resource is derived from Asset 
	uint32_t assetId; 
	GPUAssetTypeEnum assetType; 
	std::vector<uint32_t> additionalData; // like ImageView Id 

	BindingLayout(uint32_t bindingId_, ResourceTypeEnum resourceType_, VkDescriptorType descriptorType_, uint32_t descriptorCount_, VkShaderStageFlags shaderStageFlags_, Sampler* pSampler_);
	VkDescriptorSetLayoutBinding get() const;
};
