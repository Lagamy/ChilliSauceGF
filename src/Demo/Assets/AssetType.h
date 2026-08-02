#pragma once 

#include "Pool.h"
#include "Asset.h"
#include <string> 
#include <cstdint> 
#include <sstream>

template <typename Resource>
struct AssetType { 
	// Project assets are loaded first, than per scene on top to preserve cache locality when unloading scenes using sceneAssetIds
	using parseFunc = Resource(*)(uint32_t id_); 
	Pool<Asset<Resource>> assets;
	parseFunc parse;

	void addAsset(const char* name_, const char* path_)
	{
		this->assets.add(Asset<Resource>(name_, path_));
	}

	AssetType(const char* name_) 
	{ 
		std::stringstream namestream;
		namestream << name_ << " Assets"; 
		this->assets = Pool<Asset<Resource>>(namestream.str());
	}
}; 
