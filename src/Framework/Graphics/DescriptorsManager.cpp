#include "DescriptorsManager.h"
#include "Api.h"
#include <algorithm>
#include <chrono>
#include <format>
#include <stdexcept>
#include <vector>

namespace Graphics
{
    PoolId DescriptorsManager::addDescriptorSetLayout(const char* name_, uint32_t setIndex_, VkShaderStageFlags shaderStages_, uint32_t setsPerPool_)
    {
        return this->layouts.add(name_, name_, setIndex_, shaderStages_, setsPerPool_);
    }

    PoolId DescriptorsManager::getLayoutId(const char* layoutName_)
    {
        return this->layouts.getIdByName(layoutName_);
    }

    PoolId DescriptorsManager::addBufferDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, uint32_t binding_, DescriptorTypeEnum type_)
    {
        return this->layouts[layoutId_].addBufferDescriptorLayout(descriptorLayoutName_, binding_, type_);
    }

    PoolId DescriptorsManager::addDynamicDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_)
    {
        return this->layouts[layoutId_].addDynamicDescriptorLayout(descriptorLayoutName_, binding_, type_, dynamicCount_);
    }

    PoolId DescriptorsManager::addImageDescriptorLayout(PoolId layoutId_, const char* descriptorLayoutName_, uint32_t binding_, DescriptorTypeEnum type_)
    {
        return this->layouts[layoutId_].addImageDescriptorLayout(descriptorLayoutName_, binding_, type_);
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

    PoolId DescriptorsManager::addVariableToStructInDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_).addVariable(structMemberId_, name_, dataType_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addVariableToStructInDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_, PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_).addVariable(structMemberId_, name_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addDescriptorSet(const DescriptorSetCreateParams& rParams_)
    {
        return this->layouts[rParams_.layoutId].deriveDescriptorSet(rParams_);
    }

    void DescriptorsManager::removeDescriptorLayout(PoolId layoutId_, PoolId descriptorLayoutId_)
    {
        this->layouts[layoutId_].removeDescriptorLayout(descriptorLayoutId_);
    }

    /*
        vkFreeDescriptorSets requires every submitted command that uses the set to be complete. The set is not tracked per frame or per pass, so the device is waited on (waitForFrame only covers FRAME passes, a ONESHOT pass has no frame fence).
        Dynamic entries of the loaded descriptors are removed (GPU_LOCAL ones through the deferred REMOVE operation), an unloaded descriptor has none left. Static entries can't be removed, their bytes stay in the static heap until shutdown.
        The set is also unregistered from its descriptor layouts, otherwise they could never be changed again.
    */
    void DescriptorsManager::removeDescriptorSet(PoolId setId_)
    {
        DescriptorSet& rSet = this->descriptorSet[setId_];
        this->waitForGpu();

        rSet.destroy();
        for(uint32_t i = 0; i < rSet.descriptors.allocatedSize(); i++)
        {
            if(!rSet.descriptors.alive[i])
            {
                continue;
            }

            Descriptor& rDescriptor = rSet.descriptors.getInternal(i);
            if(rSet.descriptorsMemoryAllocated && rSet.allocatorType == DYNAMIC_ALLOCATOR && rDescriptor.loaded)
            {
                getMemoryManager().removeDynamicEntry(rDescriptor.memoryEntryId);
            }
            std::erase(rDescriptor.getDescriptorLayout().derivedDescriptors, setId_);
        }
        this->descriptorSet.remove(setId_);
    }

    /*
        A PipelineLayout copies the layout handle and keeps the id, and nothing detaches a layout from it, so a layout that is still used can't go.
        The VkDescriptorSetLayout is destroyed before the descriptor layouts are removed, otherwise each removal would recreate it. Descriptor layouts go one by one, each removal checks that no set is derived.
        The device is idle before the destroy: submitted commands may still reference the layout's pools and handle, and destroying an object in use by pending commands is invalid.
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

        this->waitForGpu();
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

    void DescriptorsManager::waitForGpu()
    {
        vkDeviceWaitIdle(getMainDevice().logicalDevice);
    }

    /*
        The decisions are collected first and applied after one wait for the device, since a load updates the descriptor sets and an unload destroys buffers, which no submitted command may use (see DescriptorSet::loadDescriptor).
        A descriptor is unloaded once it was not used for absenceSecondsCount seconds, a request (DescriptorSet::markUsed) loads it. Draws and passes that use an unloaded descriptor are skipped / held until the load's upload completed.
        resolvePendingOperations runs again afterwards, so the copies of the loaded bytes are queued and the deferred removals of the unloaded entries happen in this frame: the upload batch is submitted this frame, which is what waitForLoads waits on.
    */
    void DescriptorsManager::updateLoading()
    {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::vector<BoundDescriptor> toLoad;
        std::vector<BoundDescriptor> toUnload;
        for(uint32_t i = 0; i < this->descriptorSet.allocatedSize(); i++)
        {
            if(!this->descriptorSet.alive[i])
            {
                continue;
            }

            DescriptorSet& rSet = this->descriptorSet.getInternal(i);
            if(!rSet.canUnload() || rSet.vkHandles.empty())
            {
                continue;
            }

            for(uint32_t j = 0; j < rSet.descriptors.allocatedSize(); j++)
            {
                if(!rSet.descriptors.alive[j])
                {
                    continue;
                }

                Descriptor& rDescriptor = rSet.descriptors.getInternal(j);
                PoolId setId = PoolId(i, this->descriptorSet.generation[i]);
                double absentSeconds = std::chrono::duration<double>(now - rDescriptor.lastUsedTime).count();
                if(!rDescriptor.loaded && rDescriptor.loadRequested)
                {
                    toLoad.push_back({setId, rDescriptor.DescriptorLayoutId});
                }
                else if(rDescriptor.loaded && rSet.unloadsAutomatically() && absentSeconds > rSet.params.absenceSecondsCount)
                {
                    toUnload.push_back({setId, rDescriptor.DescriptorLayoutId});
                }
            }
        }
        if(toLoad.empty() && toUnload.empty())
        {
            return;
        }

        this->waitForGpu();
        for(const BoundDescriptor& rBound : toUnload)
        {
            this->descriptorSet[rBound.setId].unloadDescriptor(rBound.descriptorId);
        }
        for(const BoundDescriptor& rBound : toLoad)
        {
            DescriptorSet& rSet = this->descriptorSet[rBound.setId];
            rSet.loadDescriptor(rBound.descriptorId);
            if(rSet.params.loadMode == WAIT_UNTIL_LOADED)
            {
                this->loadsToWaitFor.push_back(rBound);
            }
        }
        getMemoryManager().resolvePendingOperations();
    }

    /* The same steps as updateLoading for one descriptor: the application decides. A load by hand honours the set's loadMode too. */
    void DescriptorsManager::loadDescriptor(PoolId setId_, PoolId descriptorId_)
    {
        DescriptorSet& rSet = this->descriptorSet[setId_];
        this->waitForGpu();
        rSet.loadDescriptor(descriptorId_);
        if(rSet.params.loadMode == WAIT_UNTIL_LOADED)
        {
            this->loadsToWaitFor.push_back({setId_, descriptorId_});
        }
        getMemoryManager().resolvePendingOperations();
    }

    void DescriptorsManager::unloadDescriptor(PoolId setId_, PoolId descriptorId_)
    {
        this->waitForGpu();
        this->descriptorSet[setId_].unloadDescriptor(descriptorId_);
        getMemoryManager().resolvePendingOperations();
    }

    /*
        Runs right after the submit that carries the upload batches: waiting on a batch before it was submitted would never end (its fence is reset when the batch starts and signaled by the submit). Blocks the calling thread, which is why WAIT_UNTIL_LOADED is meant for a renderer thread of its own.
        A GPU_LOCAL descriptor waits for the batch of its entry, a CPU_SHARED one has nothing to upload. The batch is only marked completed by the next updateUploadBatches, so the draws that use the descriptor start the next frame.
    */
    void DescriptorsManager::waitForLoads()
    {
        for(const BoundDescriptor& rBound : this->loadsToWaitFor)
        {
            DescriptorSet& rSet = this->descriptorSet[rBound.setId];
            Descriptor& rDescriptor = rSet.descriptors.getInternal(rBound.descriptorId.id);
            if(rDescriptor.loaded && rDescriptor.memoryVisability == GPU_LOCAL)
            {
                getMemoryManager().waitForBatch(getMemoryEntry(rDescriptor.memoryEntryId).uploadBatchId);
            }
        }
        this->loadsToWaitFor.clear();
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
