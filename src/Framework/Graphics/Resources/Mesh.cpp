#include "Mesh.h"
#include "Api.h"
#include "Utilities.h"
#include "VertexLayoutUpdate.h"
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
}

/*
    The vertex entry is only reserved: it has its place in the GPU buffer but no data yet. For GPU_LOCAL its whole-entry staging range is handed out by
    setVerticeVariable, so the initial data is written straight into upload staging. commitUpload queues the copy once every variable is set.
    With READ_ONLY access the index entry is already complete here, so it is locked right away. The vertex entry is locked in commitUpload, after the last initial write.
*/
void Mesh::queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, const MemoryAccessParams& access_)
{
    this->allocatorType = allocatorType_;
    this->access = access_;
    this->memoryVisability = memoryVisability_;
    this->readOnly = access_.isReadOnly();

    std::string vertexName = std::format("{} Vertex data", this->name);
    
    MemoryEntryCreateParams vertexParams = {};
    vertexParams.name = vertexName.c_str();
    vertexParams.allocatorType = allocatorType_;
    vertexParams.memoryVisability = memoryVisability_;
    vertexParams.bufferType = VERTEX_BUFFER;
    vertexParams.size = getVerticeLayout(this->verticeLayoutId).size * this->verticesCount;
    vertexParams.access = access_;
    
    this->vbMemoryUploadId = getMemoryManager().reserveEntry(vertexParams);

    // Create Index Buffer and fill it with data.
    std::string indexName = std::format("{} Index data", this->name);
    
    MemoryEntryCreateParams indexParams = {};
    indexParams.name = indexName.c_str();
    indexParams.allocatorType = allocatorType_;
    indexParams.memoryVisability = memoryVisability_;
    indexParams.bufferType = INDEX_BUFFER;
    indexParams.size = this->indices.size() * sizeof(uint32_t);
    indexParams.data = this->indices.data();
    indexParams.access = access_;
    
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
