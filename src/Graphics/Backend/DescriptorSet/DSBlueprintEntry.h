#pragma once 
#include "ContainerBlueprint.h"

enum DSBlueprintEntryTypeEnum { 
	TEXTURE_ENTRY,
	UNIFORM_STRUCT_ENTRY,
	STORAGE_STRUCT_ENTRY

};

struct DSBlueprintEntry { 
	ReflectionSystem::ContainerBlueprint entryBlueprint; // Filled by Shader or DescriptorSetBlueprint If struct. Uninitialized for images or samplers
	DSBlueprintEntryTypeEnum type;
};
