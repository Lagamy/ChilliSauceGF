#include "Mesh.h"
#include "Api.h"
#include "Utilities.h"
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
    setVerticeMember, so the initial data is written straight into upload staging. commitUpload queues the copy once every member is set.
*/
void Mesh::queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_)
{
    this->memoryVisability = memoryVisability_;
    this->vbMemoryUploadId = getMemoryManager().reserveEntry(std::format("{} Vertex data", this->name).c_str(), allocatorType_, memoryVisability_, VERTEX_BUFFER, getVerticeLayout(this->verticeLayoutId).size * this->verticesCount);
    // Create Index Buffer and fill it with data.
	this->ibMemoryUploadId = addAndUploadMemoryEntry(std::format("{} Index data", this->name).c_str(), allocatorType_, memoryVisability_, INDEX_BUFFER, this->indices.data(), this->indices.size() * sizeof(uint32_t));
}

void Mesh::commitUpload()
{
    getMemoryManager().commitEntry(this->vbMemoryUploadId);
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
