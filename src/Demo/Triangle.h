#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <glm/glm.hpp>
#include <array>

namespace Graphics 
{
struct TriangleVertexVariableIds 
{
	PoolId positionId; 
	PoolId colorId; 
}; 

struct Triangle
{
	PoolId meshId; 
	TriangleVertexVariableIds vertexVariableIds; 
	std::array<glm::vec3, 3> positions; 
	std::array<glm::vec4, 3> colors; 

	Triangle() = default; 
	Triangle(PoolId vertexLayoutId_, const TriangleVertexVariableIds& vertexVariableIds_); 

	void upload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_); 
}; 
}
