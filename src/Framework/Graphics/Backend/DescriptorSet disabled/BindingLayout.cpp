#include "BindingLayout.h"

namespace Graphics
{
BindingLayout::BindingLayout(uint32_t bindingId_, ResourceTypeEnum resourceType_, VkDescriptorType descriptorType_, uint32_t descriptorCount_, VkShaderStageFlags shaderStageFlags_, Sampler* pSampler_)
{
	this->resourceType  = resourceType_; 
	this->layout.binding = bindingId_; // Binding point in shader 
	this->layout.descriptorType = descriptorType_; // What data type(Textures, values, input Attachments) descriptors will be and how to store them(Uniform = read only, Storage = read / write) 
	this->layout.descriptorCount = descriptorCount_; // Amount of descriptors(for array of images) 
	this->layout.stageFlags = shaderStageFlags_; // Stage/s of shading to bind it to(Vertex, Teselation, Geometry or Fragment)

	// Samplers are backed into Layout creation.
	if (pSampler_ != nullptr)
	{
		layout.pImmutableSamplers = reinterpret_cast<VkSampler*>(pSampler_); // For Textures: Will predefine Samplers for all data in binding and make it immutible(For GPU to optimize access and storage).
	}
	else
	{
		layout.pImmutableSamplers = nullptr;
	}
}



VkDescriptorSetLayoutBinding BindingLayout::get() const {
	return this->layout;
}
}
