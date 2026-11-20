#include "ResourcesManager.h"
#include "Api.h"
#include "MemoryManager.h"
#include <chrono>
#include <format>
#include <stdexcept>

namespace Graphics
{
    void ResourceManager::updateLoading()
    {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::vector<PoolId> toLoad;
        std::vector<PoolId> toUnload;
        for(uint32_t i = 0; i < this->meshes.allocatedSize(); i++)
        {
            if(!this->meshes.alive[i])
            {
                continue;
            }

            Mesh& rMesh = this->meshes.getInternal(i);
            if(!rMesh.canUnload() || !rMesh.hasVertexEntry())
            {
                continue;
            }

            PoolId meshId = PoolId(i, this->meshes.generation[i]);
            double absentSeconds = std::chrono::duration<double>(now - rMesh.residency.lastUsedTime).count();
            if(!rMesh.residency.loaded && rMesh.residency.loadRequested)
            {
                toLoad.push_back(meshId);
            }
            else if(rMesh.residency.loaded && rMesh.unloadsAutomatically() && absentSeconds > rMesh.residencyParams.absenceSecondsCount && !getMemoryManager().getEntry(rMesh.vbMemoryUploadId).awaitingCommit)
            {
                toUnload.push_back(meshId);
            }
        }
        if(toLoad.empty() && toUnload.empty())
        {
            return;
        }

        /* A load makes new buffers and touches no existing set, so only the unloads wait: for the submits that used the mesh. */
        for(PoolId meshId : toUnload)
        {
            getPassesManager().waitForMeshUse(meshId);
        }
        for(PoolId meshId : toUnload)
        {
            this->meshes[meshId].unload();
        }
        for(PoolId meshId : toLoad)
        {
            Mesh& rMesh = this->meshes[meshId];
            rMesh.load(meshId);
            if(rMesh.residencyParams.loadMode == WAIT_UNTIL_LOADED)
            {
                this->meshLoadsToWaitFor.push_back(meshId);
            }
        }
        getMemoryManager().resolvePendingOperations();
    }

    void ResourceManager::loadMesh(PoolId meshId_)
    {
        Mesh& rMesh = this->meshes[meshId_];
        rMesh.load(meshId_);
        if(rMesh.residencyParams.loadMode == WAIT_UNTIL_LOADED)
        {
            this->meshLoadsToWaitFor.push_back(meshId_);
        }
        getMemoryManager().resolvePendingOperations();
    }

    void ResourceManager::unloadMesh(PoolId meshId_)
    {
        getPassesManager().waitForMeshUse(meshId_);
        this->meshes[meshId_].unload();
        getMemoryManager().resolvePendingOperations();
    }

    MemoryEntryId ResourceManager::getMeshVertexEntry(PoolId meshId_)
    {
        Mesh& rMesh = this->meshes[meshId_];
        if(!rMesh.residency.loaded)
        {
            throw std::runtime_error(std::format("Resource Manager: mesh {} is unloaded, it has no vertex entry.", rMesh.name));
        }
        return rMesh.vbMemoryUploadId;
    }

    void ResourceManager::saveMeshToDisk(PoolId meshId_)
    {
        Mesh& rMesh = this->meshes[meshId_];
        if(rMesh.residencyParams.saveToDisk == nullptr)
        {
            throw std::runtime_error(std::format("Resource Manager: mesh {} has no saveToDisk, it needs loadFromDisk.", rMesh.name));
        }
        rMesh.residencyParams.saveToDisk(meshId_, UninitializedPoolId);
    }

    void ResourceManager::changeMeshLayoutOnDisk(PoolId meshId_)
    {
        Mesh& rMesh = this->meshes[meshId_];
        if(rMesh.residencyParams.changeLayoutOnDisk == nullptr)
        {
            throw std::runtime_error(std::format("Resource Manager: mesh {} has no changeLayoutOnDisk, it needs loadFromDisk.", rMesh.name));
        }
        rMesh.residencyParams.changeLayoutOnDisk(meshId_, UninitializedPoolId);
        rMesh.markDiskLayoutChanged();
    }

    /* Runs right after the submit that carries the upload batches, see DescriptorsManager::waitForLoads. A GPU_LOCAL mesh waits for the batches of its vertex and index entry, a CPU_SHARED one has nothing to upload. */
    void ResourceManager::waitForLoads()
    {
        for(PoolId meshId : this->meshLoadsToWaitFor)
        {
            Mesh& rMesh = this->meshes[meshId];
            if(rMesh.residency.loaded && rMesh.memoryVisability == GPU_LOCAL)
            {
                getMemoryManager().waitForBatch(getMemoryEntry(rMesh.vbMemoryUploadId).uploadBatchId);
                getMemoryManager().waitForBatch(getMemoryEntry(rMesh.ibMemoryUploadId).uploadBatchId);
            }
        }
        this->meshLoadsToWaitFor.clear();
    }
}
