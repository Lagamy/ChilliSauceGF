#include "DescriptorsManager.h"
#include "Api.h"

namespace Graphics
{
    PoolId DescriptorsManager::addDescriptorSetLayout(const char* name_, DescriptorTypeEnum descriptorType_, VkShaderStageFlags shaderStages_)
    {
        return this->layouts.add(name_, name_, descriptorType_, shaderStages_);
    }

    PoolId DescriptorsManager::addDescriptorBlueprint(const char* layoutName_, const char* blueprintName_)
    {
        return this->layouts[this->layouts.getIdByName(layoutName_)].addDescriptorBlueprint(blueprintName_);
    }

    PoolId DescriptorsManager::addMemberToDescriptorBlueprint(PoolId layoutId_, PoolId blueprintId_, const char* name_, DataTypeEnum dataType_)
    {
        return this->layouts[layoutId_].getDescriptorBlueprint(blueprintId_).addMember(name_, dataType_);
    }

    PoolId DescriptorsManager::addDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_)
    {
        return this->layouts[layoutId_].deriveDescriptorSet(layoutId_, allocatorType_, memoryVisability_);
    }

    void DescriptorsManager::allocateAllDescriptorsMemory()
    {
        for(uint32_t i = 0; i < this->descriptorSet.allocatedSize(); i++)
        {
            if(this->descriptorSet.alive[i] && !this->descriptorSet.getInternal(i).descriptorsMemoryAllocated)
            {
                this->descriptorSet.getInternal(i).allocateDescriptorsMemory();
            }
        }
    }

    /* Layouts go first: a set is allocated from its layout's handle, and PipelineLayouts (created later with the pipelines) read the layout handles too. */
    void DescriptorsManager::createAll()
    {
        for(uint32_t i = 0; i < this->layouts.allocatedSize(); i++)
        {
            if(this->layouts.alive[i] && this->layouts.getInternal(i).vkHandle == VK_NULL_HANDLE)
            {
                this->layouts.getInternal(i).create();
            }
        }

        for(uint32_t i = 0; i < this->descriptorSet.allocatedSize(); i++)
        {
            if(this->descriptorSet.alive[i] && this->descriptorSet.getInternal(i).vkHandle == VK_NULL_HANDLE)
            {
                this->descriptorSet.getInternal(i).create();
            }
        }
    }

    /* Sets before layouts, the same order they were created in reverse. */
    void DescriptorsManager::destroy()
    {
        this->descriptorSet.clear();
        this->layouts.clear();
    }
}
