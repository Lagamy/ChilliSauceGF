#include "DescriptorsManager.h"
#include "Api.h"
#include <algorithm>
#include <format>
#include <stdexcept>
#include <vector>

namespace Graphics
{
    PoolId DescriptorsManager::addDescriptorSetLayout(const char* name_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_)
    {
        return this->layouts.add(name_, name_, shaderStages_, setsPerPool_);
    }

    PoolId DescriptorsManager::getLayoutId(const char* layoutName_)
    {
        return this->layouts.getIdByName(layoutName_);
    }

    PoolId DescriptorsManager::addBufferDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_)
    {
        return this->layouts[layoutId_].addBufferDescriptorLayout(descriptorLayoutName_, type_);
    }

    PoolId DescriptorsManager::addBufferDynamicDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_, uint32_t dynamicCount_)
    {
        return this->layouts[layoutId_].addBufferDynamicDescriptorLayout(descriptorLayoutName_, type_, dynamicCount_);
    }

    PoolId DescriptorsManager::addImageDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, DescriptorTypeEnum type_)
    {
        return this->layouts[layoutId_].addImageDescriptorLayout(descriptorLayoutName_, type_);
    }

    PoolId DescriptorsManager::addMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_).addMemberBlueprint(name_, dataType_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_).addMemberBlueprint(name_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addStructMemberToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, const char* name_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_).addStructMemberBlueprint(name_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addVariableToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_).addVariable(structMemberId_, name_, dataType_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addVariableToDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_).addVariable(structMemberId_, name_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addDescriptorSet(PoolId layoutId_, AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_)
    {
        return this->layouts[layoutId_].deriveDescriptorSet(layoutId_, allocatorType_, memoryVisability_);
    }

    void DescriptorsManager::removeDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_)
    {
        this->layouts[layoutId_].removeDescriptorLayout(descriptorLayoutId_);
    }

    /*
        vkFreeDescriptorSets requires every submitted command that uses the set to be complete. The set is not tracked per frame, so every frame in flight is waited on (FRAME passes only, a ONESHOT pass has no frame fence).
        Dynamic entries are removed (GPU_LOCAL ones through the deferred REMOVE operation). Static entries can't be removed, their bytes stay in the static heap until shutdown.
        The set is also unregistered from its descriptor layouts, otherwise they could never be changed again.
    */
    void DescriptorsManager::removeDescriptorSet(PoolId setId_)
    {
        DescriptorSet& rSet = this->descriptorSet[setId_];
        for(uint32_t frame = 0; frame < Globals::renderer.framesAtFlightCount; frame++)
        {
            Globals::renderer.passesGraph.waitForFrame(frame);
        }

        rSet.destroy();
        for(uint32_t i = 0; i < rSet.descriptors.allocatedSize(); i++)
        {
            if(!rSet.descriptors.alive[i])
            {
                continue;
            }

            Descriptor& rDescriptor = rSet.descriptors.getInternal(i);
            if(rSet.descriptorsMemoryAllocated && rSet.allocatorType == DYNAMIC_ALLOCATOR)
            {
                for(MemoryEntryId entryId : rDescriptor.memoryEntryIds)
                {
                    getMemoryManager().removeDynamicEntry(entryId);
                }
            }
            std::erase(rDescriptor.getDescriptorLayout().derivedDescriptors, setId_);
        }
        this->descriptorSet.remove(setId_);
    }

    /*
        A PipelineLayout copies the layout handle and keeps the id, and nothing detaches a layout from it, so a layout that is still used can't go.
        The VkDescriptorSetLayout is destroyed before the descriptor layouts are removed, otherwise each removal would recreate it. Descriptor layouts go one by one, each removal checks that no set is derived.
    */
    void DescriptorsManager::removeDescriptorSetLayout(PoolId layoutId_)
    {
        DescriptorSetLayout& rLayout = this->layouts[layoutId_];
        rLayout.throwIfSetsDerived("remove");
        for(uint32_t i = 0; i < rLayout.usedByPipelineLayoutIds.allocatedSize(); i++)
        {
            if(rLayout.usedByPipelineLayoutIds.alive[i])
            {
                throw std::runtime_error(std::format("Descriptor Set Layout {}: Can't remove a layout that a Pipeline Layout uses.", rLayout.name));
            }
        }

        rLayout.destroy();
        for(uint32_t i = 0; i < rLayout.descriptorLayouts.allocatedSize(); i++)
        {
            if(rLayout.descriptorLayouts.alive[i])
            {
                rLayout.removeDescriptorLayout(PoolId(i, rLayout.descriptorLayouts.generation[i]));
            }
        }
        this->layouts.remove(layoutId_);
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
            if(this->descriptorSet.alive[i] && this->descriptorSet.getInternal(i).vkHandles.empty())
            {
                this->descriptorSet.getInternal(i).create();
            }
        }
    }

    void DescriptorsManager::syncToFrame(uint32_t frame_)
    {
        for(uint32_t i = 0; i < this->descriptorSet.allocatedSize(); i++)
        {
            if(this->descriptorSet.alive[i])
            {
                this->descriptorSet.getInternal(i).syncToFrame(frame_);
            }
        }
    }

    void DescriptorsManager::queueLockOnSubmit(PoolId setId_, PoolId descriptorId_)
    {
        for(const BoundDescriptor& rBound : this->boundDescriptors)
        {
            if(rBound.setId == setId_ && rBound.descriptorId.id == descriptorId_.id)
            {
                return;
            }
        }
        this->boundDescriptors.push_back({setId_, descriptorId_});
    }

    /*
        CPU-side rule only, no Vulkan sync: a readOnly entry is written without waiting, which is only safe while no GPU read can overlap the host write.
        A command buffer can only read a descriptor once it is submitted, so the entries are locked right after the submit that recorded the draw and not at record time.
        The list only lives inside one Renderer::draw, so it holds no stale ids.
    */
    void DescriptorsManager::lockBoundDescriptors()
    {
        for(const BoundDescriptor& rBound : this->boundDescriptors)
        {
            this->descriptorSet[rBound.setId].lockReadOnlyDescriptor(rBound.descriptorId);
        }
        this->boundDescriptors.clear();
    }

    /* Sets before layouts, the same order they were created in reverse. */
    void DescriptorsManager::destroy()
    {
        this->descriptorSet.clear();
        for(uint32_t i = 0; i < this->layouts.allocatedSize(); i++)
        {
            if(this->layouts.alive[i])
            {
                this->layouts.getInternal(i).destroy();
            }
        }
        this->layouts.clear();
    }
}
