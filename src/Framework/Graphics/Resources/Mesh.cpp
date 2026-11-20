#include "Mesh.h"
#include "Api.h"
#include "Utilities.h"
#include "VertexLayoutUpdate.h"
#include <chrono>
#include <cstring>
#include <format>
#include <stdexcept>

namespace Graphics
{
Mesh::Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_)
{
    this->name = name_;
    this->verticeLayoutId = verticeLayoutId_;
    this->verticesCount = repeatCount_;
    this->residencyParams.absenceSecondsCount = UninitializedId;
}

void Mesh::queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, const MemoryAccessParams& access_)
{
    ResidencyParams residency = {};
    residency.absenceSecondsCount = UninitializedId;
    this->queueUpload(allocatorType_, memoryVisability_, access_, residency);
}

void Mesh::queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, const MemoryAccessParams& access_, const ResidencyParams& residency_)
{
    residency_.validate(allocatorType_);
    this->allocatorType = allocatorType_;
    this->access = access_;
    this->memoryVisability = memoryVisability_;
    this->readOnly = access_.isReadOnly();
    this->residencyParams = residency_;
    this->residency.lastUsedTime = std::chrono::steady_clock::now();
    this->residency.backup.data.assign(this->mirrorsWrites() ? getVerticeLayout(this->verticeLayoutId).size * this->verticesCount : 0, 0);

    this->reserveVertexEntry();
    this->uploadIndexEntry();
}

/*
    The vertex entry is only reserved: it has its place in the GPU buffer but no data yet. For GPU_LOCAL its whole-entry staging range is handed out by
    setVerticeVariable, so the initial data is written straight into upload staging. commitUpload queues the copy once every variable is set.
*/
void Mesh::reserveVertexEntry()
{
    std::string vertexName = std::format("{} Vertex data", this->name);

    MemoryEntryCreateParams vertexParams = {};
    vertexParams.name = vertexName.c_str();
    vertexParams.allocatorType = this->allocatorType;
    vertexParams.memoryVisability = this->memoryVisability;
    vertexParams.bufferType = VERTEX_BUFFER;
    vertexParams.size = getVerticeLayout(this->verticeLayoutId).size * this->verticesCount;
    vertexParams.access = this->access;

    this->vbMemoryUploadId = getMemoryManager().reserveEntry(vertexParams);
}

/* With READ_ONLY access the index entry is already complete here, so it is locked right away. The vertex entry is locked in commitUpload, after the last initial write. */
void Mesh::uploadIndexEntry()
{
    std::string indexName = std::format("{} Index data", this->name);

    MemoryEntryCreateParams indexParams = {};
    indexParams.name = indexName.c_str();
    indexParams.allocatorType = this->allocatorType;
    indexParams.memoryVisability = this->memoryVisability;
    indexParams.bufferType = INDEX_BUFFER;
    indexParams.size = this->indices.size() * sizeof(uint32_t);
    indexParams.data = this->indices.data();
    indexParams.access = this->access;

    this->ibMemoryUploadId = addAndUploadMemoryEntry(indexParams);
    if(this->readOnly)
    {
        getMemoryManager().lockEntry(this->ibMemoryUploadId);
    }
}

void Mesh::commitUpload()
{
    getMemoryManager().commitEntry(this->vbMemoryUploadId);
    if(this->readOnly)
    {
        getMemoryManager().lockEntry(this->vbMemoryUploadId);
    }
}

bool Mesh::hasVertexEntry() const
{
    return this->vbMemoryUploadId.id != UninitializedPoolId;
}

void Mesh::throwIfCantReplaceVertexEntry()
{
    /* An unloaded mesh has no entry to replace, only its CPU copy changes (restride). */
    if(!this->residency.loaded)
    {
        return;
    }
    if(this->allocatorType == STATIC_ALLOCATOR)
    {
        throw std::runtime_error(std::format("Mesh {}: The vertex layout change needs a new vertex entry, which a STATIC_ALLOCATOR mesh can't have (static entries can't be removed).", this->name));
    }
    if(getMemoryManager().getEntry(this->vbMemoryUploadId).awaitingCommit)
    {
        throw std::runtime_error(std::format("Mesh {}: The vertex layout change needs a new vertex entry, commit the upload of the mesh first.", this->name));
    }
}

void Mesh::restride(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_)
{
    if(!this->hasVertexEntry())
    {
        return;
    }

    /* The CPU copy follows the layout whether the entry is there or not. The file of a loadFromDisk mesh keeps the old layout until the application converts it (changeLayoutOnDisk), so a load throws meanwhile. */
    this->remapBackup(rOldLayout_, rNewLayout_);
    this->residency.diskLayoutStale = this->residencyParams.loadFromDisk != nullptr;
    if(!this->residency.loaded)
    {
        return;
    }
    if(vertexKeepsEntry(rOldLayout_, rNewLayout_))
    {
        getMemoryManager().zeroEntryRanges(this->vbMemoryUploadId, makeVertexZeroRegions(rOldLayout_, rNewLayout_, this->verticesCount, false));
        return;
    }

    MemoryManager& rMemoryManager = getMemoryManager();
    std::vector<MemoryCopyRegion> regions = makeVertexCopyRegions(rOldLayout_, rNewLayout_, this->verticesCount);

    std::string vertexName = std::format("{} Vertex data", this->name);
    MemoryEntryCreateParams vertexParams = {};
    vertexParams.name = vertexName.c_str();
    vertexParams.allocatorType = this->allocatorType;
    vertexParams.memoryVisability = this->memoryVisability;
    vertexParams.bufferType = VERTEX_BUFFER;
    vertexParams.size = rNewLayout_.size * this->verticesCount;
    vertexParams.access = this->access;
    MemoryEntryId newEntryId = rMemoryManager.allocateEntry(vertexParams);

    /* Entries are fetched after the add, which can grow the allocator's storage. */
    MemoryEntry& rOldEntry = rMemoryManager.getEntry(this->vbMemoryUploadId);
    if(this->memoryVisability == GPU_LOCAL && !rOldEntry.hasShadow)
    {
        rMemoryManager.copyEntry(this->vbMemoryUploadId, newEntryId, regions);
        getMemoryManager().zeroEntryRanges(newEntryId, makeVertexZeroRegions(rOldLayout_, rNewLayout_, this->verticesCount, true));
    }
    else
    {
        std::vector<char> oldBytes(rOldEntry.size);
        std::vector<char> newBytes(vertexParams.size, 0);
        rMemoryManager.readShared(rOldEntry, 0, oldBytes.data(), oldBytes.size());
        for(const MemoryCopyRegion& rRegion : regions)
        {
            std::memcpy(&newBytes[rRegion.dstOffset], &oldBytes[rRegion.srcOffset], rRegion.size);
        }
        rMemoryManager.writeShared(rMemoryManager.getEntry(newEntryId), 0, newBytes.data(), newBytes.size());
    }

    rMemoryManager.removeDynamicEntry(this->vbMemoryUploadId);
    this->vbMemoryUploadId = newEntryId;
    if(this->readOnly)
    {
        rMemoryManager.lockEntry(newEntryId);
    }
}

bool Mesh::canUnload() const
{
    return this->allocatorType == DYNAMIC_ALLOCATOR;
}

bool Mesh::unloadsAutomatically() const
{
    return this->canUnload() && this->residencyParams.absenceSecondsCount != UninitializedId;
}

bool Mesh::mirrorsWrites() const
{
    return this->unloadsAutomatically() && this->residencyParams.loadFromDisk == nullptr && !this->access.hasShadow();
}

/* Stamps the use for the absence time (only a mesh that unloads by itself has one) and requests a load of an unloaded mesh, also when it was unloaded by hand. */
void Mesh::markUsed()
{
    if(!this->canUnload())
    {
        return;
    }
    this->residency.markUsedFrame();
    if(this->unloadsAutomatically())
    {
        this->residency.lastUsedTime = std::chrono::steady_clock::now();
    }
    if(!this->residency.loaded)
    {
        this->residency.loadRequested = true;
    }
}

void Mesh::markDiskLayoutChanged()
{
    this->residency.diskLayoutStale = false;
}

bool Mesh::isSettled()
{
    if(!this->residency.loaded)
    {
        return false;
    }
    MemoryManager& rMemoryManager = getMemoryManager();
    return rMemoryManager.isFrameCopyReady(this->vbMemoryUploadId) && rMemoryManager.isFrameCopyReady(this->ibMemoryUploadId);
}

/*
    Vulkan rule: a buffer may only be destroyed once no submitted command uses it (VUID-vkDestroyBuffer-buffer-00922), which the caller guarantees by waiting for the device. The removal of a GPU_LOCAL entry is deferred until its uploads are done.
    The CPU keeps one copy of the bytes: an entry with a shadow hands it over (takeShadow) before it is removed, a CPU_SHARED entry is read, any other mirrored its writes into the backup already. A loadFromDisk mesh needs none.
*/
void Mesh::unload()
{
    if(!this->canUnload())
    {
        throw std::runtime_error(std::format("Mesh {}: Only a DYNAMIC_ALLOCATOR mesh can unload, static entries can't be removed.", this->name));
    }
    if(!this->residency.loaded)
    {
        return;
    }

    MemoryManager& rMemoryManager = getMemoryManager();
    if(rMemoryManager.getEntry(this->vbMemoryUploadId).awaitingCommit)
    {
        throw std::runtime_error(std::format("Mesh {}: Can't unload before the upload was committed (commitUpload).", this->name));
    }
    if(this->residencyParams.loadFromDisk == nullptr)
    {
        if(this->access.hasShadow())
        {
            this->residency.backup.data = rMemoryManager.takeShadow(this->vbMemoryUploadId);
        }
        else if(!this->mirrorsWrites())
        {
            if(this->memoryVisability == CPU_SHARED)
            {
                MemoryEntry& rEntry = rMemoryManager.getEntry(this->vbMemoryUploadId);
                this->residency.backup.data.resize(rEntry.size);
                rMemoryManager.readShared(rEntry, 0, this->residency.backup.data.data(), rEntry.size);
            }
            else
            {
                throw std::runtime_error(std::format("Mesh {}: Can't unload, nothing could bring its bytes back. Queue it with residency.loadFromDisk or an absenceSecondsCount (the engine then keeps a CPU copy).", this->name));
            }
        }
    }

    rMemoryManager.removeDynamicEntry(this->vbMemoryUploadId);
    rMemoryManager.removeDynamicEntry(this->ibMemoryUploadId);
    this->residency.loaded = false;
    this->residency.loadRequested = false;
}

/*
    New entries made like queueUpload's, then the bytes come back from loadFromDisk (the same setVerticeVariable calls that wrote them the first time) or the CPU copy, and commitUpload queues the copy. The mesh is settled once that upload completed.
    loaded is set before the writes since the setters throw on an unloaded mesh.
*/
void Mesh::load(PoolId meshId_)
{
    if(this->residency.loaded)
    {
        return;
    }
    if(this->residency.diskLayoutStale && this->residencyParams.loadFromDisk != nullptr)
    {
        throw std::runtime_error(std::format("Mesh {}: Can't be loaded from disk, a vertex layout update changed its layout and the file still has the old one. Convert the file first.", this->name));
    }

    this->reserveVertexEntry();
    this->uploadIndexEntry();
    this->residency.loaded = true;
    this->residency.loadRequested = false;
    this->residency.lastUsedTime = std::chrono::steady_clock::now();
    if(this->residencyParams.loadFromDisk != nullptr)
    {
        this->residencyParams.loadFromDisk(meshId_, UninitializedPoolId);
    }
    else
    {
        this->restoreBackup();
    }
    this->commitUpload();
}

/* One write of the whole entry, so a GPU_LOCAL mesh is one update (one staging range) and not one per variable. */
void Mesh::restoreBackup()
{
    uint64_t size = this->residency.backup.data.size();
    if(size == 0)
    {
        return;
    }

    if(this->memoryVisability == GPU_LOCAL && !this->access.hasShadow())
    {
        StagingDataContainer container = this->getStagingContainer(0, size);
        container.write(0, this->residency.backup.data.data(), size);
    }
    else
    {
        SharedDataContainer container = this->getSharedContainer();
        container.validFirstOffset = 0;
        container.validSize = size;
        container.write(0, this->residency.backup.data.data(), size);
    }

    /* Unless the writes are mirrored the bytes are in the new entry now (its shadow, or the CPU_SHARED memory), so the backup would be a second CPU copy of them. */
    if(!this->mirrorsWrites())
    {
        std::vector<char>().swap(this->residency.backup.data);
    }
}

/* The bytes of the variables both layouts have move to their new offsets, everything else is 0 (like the zeroed ranges of the entry). */
void Mesh::remapBackup(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_)
{
    if(this->residency.backup.data.empty())
    {
        return;
    }

    std::vector<char> newBytes(rNewLayout_.size * this->verticesCount, 0);
    for(const MemoryCopyRegion& rRegion : makeVertexCopyRegions(rOldLayout_, rNewLayout_, this->verticesCount))
    {
        std::memcpy(&newBytes[rRegion.dstOffset], &this->residency.backup.data[rRegion.srcOffset], rRegion.size);
    }
    this->residency.backup.data = std::move(newBytes);
}

bool Mesh::hasCopies()
{
    return getMemoryManager().getEntry(this->vbMemoryUploadId).hasShadow;
}

StagingDataContainer Mesh::getStagingContainer(uint64_t inEntryOffset_, uint64_t size_)
{
    #ifdef ENGINE_DEBUG
        if(this->vbMemoryUploadId.id == UninitializedPoolId)
        {
            throw std::runtime_error(std::format("Mesh {}: Can't set vertex data before the mesh was uploaded with queueUpload.", this->name));
        }
    #endif
    return getMemoryManager().getStagingContainer(this->vbMemoryUploadId, inEntryOffset_, size_);
}

SharedDataContainer Mesh::getSharedContainer()
{
    #ifdef ENGINE_DEBUG
        if(this->vbMemoryUploadId.id == UninitializedPoolId)
        {
            throw std::runtime_error(std::format("Mesh {}: Can't set vertex data before the mesh was uploaded with queueUpload.", this->name));
        }
    #endif
    return getMemoryManager().getSharedContainer(this->vbMemoryUploadId);
}
}
