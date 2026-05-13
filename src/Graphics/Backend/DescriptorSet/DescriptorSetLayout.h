#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <string>
#include "DSLayoutEntry.h"
#include "DescriptorSet.h" 
#include "DescriptorSetBlueprint.h"

enum class BufferDescriptorTypeEnum {
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Small structured read - only buffers
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER // Large structured read/write buffers
};

enum class ImageDescriptorTypeEnum {
	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Image + sampler, read - only in shader
	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, // Image only, read-only in shader
	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE // Image only, read/write in shader
};

struct DescriptorSetLayout {
	std::string name;
	VkDescriptorSetLayout vkHandle = VK_NULL_HANDLE;
	std::vector<DSLayoutEntry> layoutEntry; 

	bool isBindless; 
	uint32_t startingEntityAmount; // For Bindless

	std::vector<DescriptorSet> derivedSets; 

	void addBufferLayout(); // Adds new entry to both template and structsBlueprints 
	void addImageLayout(); // Adds new entry to template


	void create(); // only call after vector was completlly setup 
	void destroy(); 
	VkDescriptorSetLayout get() const; 
	DescriptorSetLayout(DescriptorSetBlueprint& rParentDSBlueprint_); 
};
