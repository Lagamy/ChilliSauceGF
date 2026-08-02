#pragma once 

#include "AssetType.h"
#include "Mesh.h"
#include "Texture.h"
#include <cstdint>
#include <stdexcept>

struct AssetsManager { 
	AssetType<Mesh>  modelAT = AssetType<Mesh>("Model"); 
	AssetType<Texture> pictureAT = AssetType<Texture>("Picture"); 

	// Uploads to the GPU
	uint32_t useMeshAsset(uint32_t id_);
	// uint32_t useTextureAsset(uint32_t id_); 

	PoolId addMeshAsset(const char* name_, uint32_t meshTypeId_, const char* path_); // returns meshId
	// PoolId addTextureAsset(const char* name_, const char* path_); // returns imageId 
}; 

