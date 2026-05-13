#include "DescriptorSetBlueprint.h"

void DescriptorSetBlueprint::addTextureEntry() {
	this->entries.emplace_back(); 
	this->entries.back().type = TEXTURE_ENTRY;
}

void DescriptorSetBlueprint::addStructEntry(DSTemplateEntryTypeEnum entryType_, ReflectionSystem::ContainerTemplate entryBlueprint_) { 
	this->entries.emplace_back();
	this->entries.back().type = entryType_; 
	this->entries.back().entryTemplate = entryBlueprint_; 
}
