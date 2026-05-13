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
	void useMeshAsset(uint32_t id_);
	void useTextureAsset(uint32_t id_); 

	uint32_t addMeshAsset(const char* name_, uint32_t meshTypeId_, const char* path_); // returns meshId
	uint32_t addTextureAsset(const char* name_, const char* path_); // returns imageId 
}; 

