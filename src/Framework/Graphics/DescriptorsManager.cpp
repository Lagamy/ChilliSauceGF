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

    /* The staged copy is taken here: the live layout is not touched until the commit, so writes through its ids keep working while the update is open. */
    PoolId DescriptorsManager::beginSetLayoutUpdate(PoolId layoutId_)
    {
        DescriptorSetLayout& rLayout = this->layouts[layoutId_];
        if(rLayout.updateOpen)
        {
            throw std::runtime_error(std::format("Descriptor Set Layout {}: An update of it is already open, commit it first.", rLayout.name));
        }
        rLayout.updateOpen = true;
        return this->setLayoutUpdates.add(layoutId_);
    }

    PoolId DescriptorsManager::beginDescriptorLayoutUpdate(PoolId layoutId_, PoolId descriptorLayoutId_)
    {
        DescriptorLayout& rLayout = this->layouts[layoutId_].getDescriptorLayout(descriptorLayoutId_);
        if(rLayout.updateOpen)
        {
            throw std::runtime_error(std::format("Descriptor Layout {}: An update of it is already open, commit it first.", rLayout.name));
        }
        rLayout.updateOpen = true;
        return this->descriptorLayoutUpdates.add(layoutId_, descriptorLayoutId_, rLayout);
    }

    PoolId DescriptorsManager::addBufferDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t headroom_)
    {
        SetLayoutUpdate& rUpdate = this->setLayoutUpdates[setLayoutUpdateId_];
        PoolId descriptorLayoutId = this->layouts[rUpdate.setLayoutId].addBufferDescriptorLayout(descriptorLayoutName_, binding_, type_, headroom_);
        rUpdate.addedDescriptorLayoutIds.push_back(descriptorLayoutId);
        return descriptorLayoutId;
    }

    PoolId DescriptorsManager::addDynamicDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_, uint32_t headroom_)
    {
        SetLayoutUpdate& rUpdate = this->setLayoutUpdates[setLayoutUpdateId_];
        PoolId descriptorLayoutId = this->layouts[rUpdate.setLayoutId].addDynamicDescriptorLayout(descriptorLayoutName_, binding_, type_, dynamicCount_, headroom_);
        rUpdate.addedDescriptorLayoutIds.push_back(descriptorLayoutId);
        return descriptorLayoutId;
    }

    PoolId DescriptorsManager::addImageDescriptorLayout(const char* descriptorLayoutName_, PoolId setLayoutUpdateId_, uint32_t binding_, DescriptorTypeEnum type_)
    {
        return this->layouts[this->setLayoutUpdates[setLayoutUpdateId_].setLayoutId].addImageDescriptorLayout(descriptorLayoutName_, binding_, type_);
    }

    /* A descriptor layout added by the same update never existed for the sets, so it just goes. Anything else is removed by the commit. */
    void DescriptorsManager::removeDescriptorLayout(PoolId setLayoutUpdateId_, PoolId descriptorLayoutId_)
    {
        SetLayoutUpdate& rUpdate = this->setLayoutUpdates[setLayoutUpdateId_];
        DescriptorSetLayout& rLayout = this->layouts[rUpdate.setLayoutId];
        rLayout.getDescriptorLayout(descriptorLayoutId_);
        auto added = std::find(rUpdate.addedDescriptorLayoutIds.begin(), rUpdate.addedDescriptorLayoutIds.end(), descriptorLayoutId_);
        if(added != rUpdate.addedDescriptorLayoutIds.end())
        {
            rUpdate.addedDescriptorLayoutIds.erase(added);
            rLayout.removeDescriptorLayout(descriptorLayoutId_);
        }
        else if(std::find(rUpdate.removedDescriptorLayoutIds.begin(), rUpdate.removedDescriptorLayoutIds.end(), descriptorLayoutId_) == rUpdate.removedDescriptorLayoutIds.end())
        {
            rUpdate.removedDescriptorLayoutIds.push_back(descriptorLayoutId_);
        }
    }

    PoolId DescriptorsManager::addMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->descriptorLayoutUpdates[descriptorLayoutUpdateId_].staged.addMemberBlueprint(name_, orderPosition_, dataType_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->descriptorLayoutUpdates[descriptorLayoutUpdateId_].staged.addMemberBlueprint(name_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addStructMemberToDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_)
    {
        return this->descriptorLayoutUpdates[descriptorLayoutUpdateId_].staged.addStructMemberBlueprint(name_, orderPosition_, repeatCountPerDimension_, headroom_);
    }

    PoolId DescriptorsManager::addVariableToStructInDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->descriptorLayoutUpdates[descriptorLayoutUpdateId_].staged.addVariable(name_, structMemberId_, orderPosition_, dataType_, repeatCountPerDimension_);
    }

    PoolId DescriptorsManager::addVariableToStructInDescriptorLayout(const char* name_, PoolId descriptorLayoutUpdateId_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->descriptorLayoutUpdates[descriptorLayoutUpdateId_].staged.addVariable(name_, structMemberId_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
    }

    void DescriptorsManager::removeMemberFromDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId memberId_)
    {
        this->descriptorLayoutUpdates[descriptorLayoutUpdateId_].staged.removeMemberBlueprint(memberId_);
    }

    void DescriptorsManager::removeVariableFromStructInDescriptorLayout(PoolId descriptorLayoutUpdateId_, PoolId variableId_)
    {
        this->descriptorLayoutUpdates[descriptorLayoutUpdateId_].staged.removeVariableFromStruct(variableId_);
    }

    /*
        The staged members replace the live ones. The sets derived from the layout whose memory exists are reshaped one by one (DescriptorSet::reshapeDescriptor: new entry, bytes carried over, bindings rewritten), after the device is idle: the old entries are destroyed and the bindings of the sets are updated, which no submitted command may overlap
        (VUID-vkDestroyBuffer-buffer-00922, VUID-vkUpdateDescriptorSets-None-03047). The binding itself doesn't change, so the VkDescriptorSetLayout and the pipelines stay.
        A static set can't be reshaped (static entries can't be removed), which is checked before anything changes. Sets that were not allocated yet take the new block when they are.
    */
    void DescriptorsManager::commitDescriptorLayoutUpdate(PoolId updateId_)
    {
        DescriptorLayoutUpdate& rUpdate = this->descriptorLayoutUpdates[updateId_];
        DescriptorLayout& rLayout = this->layouts[rUpdate.setLayoutId].getDescriptorLayout(rUpdate.descriptorLayoutId);

        std::vector<PoolId> liveSetIds;
        for(const PoolId& rSetId : rLayout.derivedDescriptors)
        {
            DescriptorSet& rSet = this->descriptorSet[rSetId];
            if(!rSet.descriptorsMemoryAllocated)
            {
                continue;
            }
            if(!rSet.canUnload())
            {
                throw std::runtime_error(std::format("Descriptor Layout {}: Can't change the block, a derived STATIC_ALLOCATOR set already has its memory and static entries can't be removed.", rLayout.name));
            }
            liveSetIds.push_back(rSetId);
        }

        if(liveSetIds.empty())
        {
            rLayout.members = std::move(rUpdate.staged.members);
            rLayout.variables = std::move(rUpdate.staged.variables);
        }
        else
        {
            this->waitForGpu();
            DescriptorLayout oldLayout = rLayout;
            std::vector<DescriptorShape> oldShapes;
            for(const PoolId& rSetId : liveSetIds)
            {
                oldShapes.push_back(this->descriptorSet[rSetId].descriptors.getInternal(rUpdate.descriptorLayoutId.id).shape);
            }
            rLayout.members = std::move(rUpdate.staged.members);
            rLayout.variables = std::move(rUpdate.staged.variables);
            for(uint32_t i = 0; i < liveSetIds.size(); i++)
            {
                this->descriptorSet[liveSetIds[i]].reshapeDescriptor(rUpdate.descriptorLayoutId, oldLayout, oldShapes[i]);
            }
            getMemoryManager().resolvePendingOperations();
        }
        rLayout.updateOpen = false;
        this->descriptorLayoutUpdates.remove(updateId_);
    }

    /*
        Removed descriptor layouts go first (their descriptors in every derived set, then the layout), then the pending ones become real, and a layout whose handle exists is recreated (DescriptorSetLayout::recreate also rebuilds the pipeline layouts and pipelines that use it).
        A VkDescriptorSet belongs to the VkDescriptorSetLayout it was allocated with, so every created derived set gets new ones (DescriptorSet::rebuildHandles) that point at the entries again. New descriptors of a set with memory get their entry; a STATIC_ALLOCATOR set can't (checked first, nothing changes before that).
        Shaders that declared a removed descriptor layout have to be changed by the application, nothing here edits them.
    */
    void DescriptorsManager::commitSetLayoutUpdate(PoolId updateId_)
    {
        SetLayoutUpdate& rUpdate = this->setLayoutUpdates[updateId_];
        DescriptorSetLayout& rLayout = this->layouts[rUpdate.setLayoutId];
        for(const PoolId& rAddedId : rUpdate.addedDescriptorLayoutIds)
        {
            if(rLayout.getDescriptorLayout(rAddedId).updateOpen)
            {
                throw std::runtime_error(std::format("Descriptor Set Layout {}: Descriptor layout {} has an uncommitted DescriptorLayoutUpdate, commit it before the set layout update.", rLayout.name, rLayout.getDescriptorLayout(rAddedId).name));
            }
        }

        std::vector<PoolId> setIds;
        for(uint32_t i = 0; i < rLayout.descriptorLayouts.allocatedSize(); i++)
        {
            if(!rLayout.descriptorLayouts.alive[i])
            {
                continue;
            }
            for(const PoolId& rSetId : rLayout.descriptorLayouts.getInternal(i).derivedDescriptors)
            {
                if(std::find(setIds.begin(), setIds.end(), rSetId) == setIds.end())
                {
                    setIds.push_back(rSetId);
                }
            }
        }
        for(const PoolId& rSetId : setIds)
        {
            DescriptorSet& rSet = this->descriptorSet[rSetId];
            if(!rUpdate.addedDescriptorLayoutIds.empty() && rSet.descriptorsMemoryAllocated && !rSet.canUnload())
            {
                throw std::runtime_error(std::format("Descriptor Set Layout {}: Can't add a descriptor layout, a derived STATIC_ALLOCATOR set already has its memory and static entries have to be added before the static allocation.", rLayout.name));
            }
        }

        if(rLayout.vkHandle != VK_NULL_HANDLE || !setIds.empty())
        {
            this->waitForGpu();
        }
        for(const PoolId& rRemovedId : rUpdate.removedDescriptorLayoutIds)
        {
            for(const PoolId& rSetId : setIds)
            {
                this->descriptorSet[rSetId].removeDescriptor(rRemovedId);
            }
            rLayout.removeDescriptorLayout(rRemovedId);
        }
        for(const PoolId& rAddedId : rUpdate.addedDescriptorLayoutIds)
        {
            rLayout.getDescriptorLayout(rAddedId).pending = false;
            for(const PoolId& rSetId : setIds)
            {
                this->descriptorSet[rSetId].addDescriptor(rSetId, rAddedId);
            }
        }
        rLayout.updateDynamicFlag();

        if(rLayout.vkHandle != VK_NULL_HANDLE)
        {
            rLayout.recreate();
            for(const PoolId& rSetId : setIds)
            {
                if(!this->descriptorSet[rSetId].vkHandles.empty())
                {
                    this->descriptorSet[rSetId].rebuildHandles();
                }
            }
        }
        getMemoryManager().resolvePendingOperations();
        rLayout.updateOpen = false;
        this->setLayoutUpdates.remove(updateId_);
    }

    PoolId DescriptorsManager::addDescriptorSet(const DescriptorSetCreateParams& rParams_)
    {
        return this->layouts[rParams_.layoutId].deriveDescriptorSet(rParams_);
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
            if(rSet.descriptorsMemoryAllocated && rSet.allocatorType == DYNAMIC_ALLOCATOR && rDescriptor.residency.loaded)
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
        The decisions are collected first and applied after waiting for the submits that used the descriptors (PassesGraph::waitForDescriptorUse, not the whole device), since a load updates the descriptor sets and an unload destroys buffers, which no submitted command may use (see DescriptorSet::loadDescriptor). A descriptor nobody used for the absence time has nothing to wait for.
        A descriptor is unloaded once it was not used for residency.absenceSecondsCount seconds, a request (DescriptorSet::markUsed) loads it. Draws and passes that use an unloaded descriptor are skipped / held until the load's upload completed.
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
                double absentSeconds = std::chrono::duration<double>(now - rDescriptor.residency.lastUsedTime).count();
                if(!rDescriptor.residency.loaded && rDescriptor.residency.loadRequested)
                {
                    toLoad.push_back({setId, rDescriptor.DescriptorLayoutId});
                }
                else if(rDescriptor.residency.loaded && rSet.unloadsAutomatically() && absentSeconds > rSet.params.residency.absenceSecondsCount)
                {
                    toUnload.push_back({setId, rDescriptor.DescriptorLayoutId});
                }
            }
        }
        if(toLoad.empty() && toUnload.empty())
        {
            return;
        }

        for(const BoundDescriptor& rBound : toUnload)
        {
            getPassesManager().waitForDescriptorUse(rBound.setId, rBound.descriptorId);
        }
        for(const BoundDescriptor& rBound : toLoad)
        {
            getPassesManager().waitForDescriptorUse(rBound.setId, rBound.descriptorId);
        }
        for(const BoundDescriptor& rBound : toUnload)
        {
            this->descriptorSet[rBound.setId].unloadDescriptor(rBound.descriptorId);
        }
        for(const BoundDescriptor& rBound : toLoad)
        {
            DescriptorSet& rSet = this->descriptorSet[rBound.setId];
            rSet.loadDescriptor(rBound.descriptorId);
            if(rSet.params.residency.loadMode == WAIT_UNTIL_LOADED)
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
        getPassesManager().waitForDescriptorUse(setId_, descriptorId_);
        rSet.loadDescriptor(descriptorId_);
        if(rSet.params.residency.loadMode == WAIT_UNTIL_LOADED)
        {
            this->loadsToWaitFor.push_back({setId_, descriptorId_});
        }
        getMemoryManager().resolvePendingOperations();
    }

    MemoryEntryId DescriptorsManager::getDescriptorMemoryEntry(PoolId setId_, PoolId descriptorId_)
    {
        return this->descriptorSet[setId_].getDescriptorEntryId(descriptorId_);
    }

    void DescriptorsManager::saveDescriptorToDisk(PoolId setId_, PoolId descriptorId_)
    {
        DescriptorSet& rSet = this->descriptorSet[setId_];
        if(rSet.params.residency.saveToDisk == nullptr)
        {
            throw std::runtime_error("Descriptors Manager: the set has no saveToDisk, it needs loadFromDisk.");
        }
        rSet.params.residency.saveToDisk(setId_, descriptorId_);
    }

    void DescriptorsManager::changeDescriptorLayoutOnDisk(PoolId setId_, PoolId descriptorId_)
    {
        DescriptorSet& rSet = this->descriptorSet[setId_];
        if(rSet.params.residency.changeLayoutOnDisk == nullptr)
        {
            throw std::runtime_error("Descriptors Manager: the set has no changeLayoutOnDisk, it needs loadFromDisk.");
        }
        rSet.params.residency.changeLayoutOnDisk(setId_, descriptorId_);
        rSet.markDiskLayoutChanged(descriptorId_);
    }

    void DescriptorsManager::unloadDescriptor(PoolId setId_, PoolId descriptorId_)
    {
        getPassesManager().waitForDescriptorUse(setId_, descriptorId_);
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
            if(rDescriptor.residency.loaded && rDescriptor.memoryVisability == GPU_LOCAL)
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
