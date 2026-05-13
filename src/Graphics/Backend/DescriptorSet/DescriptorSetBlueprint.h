// Exist to compare resources with each other(Shader layout <-> DSLayout) to figure out if compatible
#pragma once 
#include "ContainerBlueprint.h"
#include "DSBlueprintEntry.h"

struct DescriptorSetBlueprint { 

	std::vector<DSBlueprintEntry> entries; // Order matters. For checking Shader glsl generated template for DS and DescriptorSetBlueprint one against one another(To check if compatible)

	void addTextureEntry(); 
	void addStructEntry(DSBlueprintEntryTypeEnum entryType_, ReflectionSystem::ContainerBlueprint entryBlueprint_);
}; 
