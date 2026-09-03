#pragma once 
#include <cstdint> 
#include <numeric>

struct PoolId
{
	uint32_t id; 
	uint32_t generation;

	PoolId(uint32_t id_, uint32_t generation_) : id(id_), generation(generation_) {}; 
	PoolId() : id(std::numeric_limits<uint32_t>::max()), generation(std::numeric_limits<uint32_t>::max()) {};
};
 