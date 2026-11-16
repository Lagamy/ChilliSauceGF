#pragma once 
#include "PoolId.h"
#include "Triangle.h"
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

namespace Graphics 
{
struct MVPMemberIds
{
	PoolId projectionId;
	PoolId viewId;
};

struct Objects 
{
	PoolId statesId; 
	PoolId colorId;
	PoolId rotationDirectionId;
	PoolId modelId;
};

namespace Demo { 
	inline PoolId vertexShaderId; 
	inline PoolId fragmentShaderId; 
	inline PoolId vertexLayoutId; 
	inline TriangleVertexVariableIds vertexVariableIds; 

	inline Triangle triangle;
	inline PoolId graphicsPipelineId; 

	inline PoolId dsLayoutId;
	inline PoolId vpDescriptorLayoutId;

	inline PoolId counterDescriptorLayoutId;  
	inline PoolId counterMemberId; 
	inline PoolId objectsDescriptorLayoutId; 
	inline Objects objectsMemberIds; 


	inline MVPMemberIds vpMemberIds;
	inline PoolId descriptorSetId;

	inline uint32_t counter = 0; 
	inline bool descriptorsHaveSet = false; 
	inline VkExtent2D lastProjectionExtent = {};

	void setupEnvironment(); 
	void defineLayouts();
	void defineResources(); 
	void definePasses(); 
	void updateResources(); 
	void recordCMDs(VkCommandBuffer& cmdBuffer_);
	
	void setGPUSceneToDemo();
}; 
}