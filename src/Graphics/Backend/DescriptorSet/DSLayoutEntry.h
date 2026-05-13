#pragma once 

#include "Asset.h"
#include "ContainerBlueprint.h"
#include "DSBlueprintEntry.h"


struct DSLayoutEntry : DSBlueprintEntry { 
	bool isAssetResource; // Otherwise -> descriptor created one 
	/* If Resource is inherented from Asset with same template */ 
	// If Editor Mode 
	uint32_t assetTypeId; 
	uint32_t assetId; // To trigger upload, if lazy-loaded 
	uint32_t resourceId; // To know - which resource of entities ResourceBundle to use 

	// if Runtime Mode(stable pointers)
	Asset* pAsset; 

	/* If Resource is created by the DS. Comparable to DS Blueprint to check if suitable */
	ReflectionSystem::ContainerBlueprint blueprint;
};
