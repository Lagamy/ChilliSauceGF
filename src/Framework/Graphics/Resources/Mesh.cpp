#include "Mesh.h"
#include "Api.h"
#include "Utilities.h"
#include <stdexcept>

namespace Graphics
{
Mesh::Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_)
{
    this->name = name_; 
    this->verticeLayoutId = verticeLayoutId_;
    this->vertexDataContainerId = getVerticeLayout(verticeLayoutId_).createDataContainer(this->name.c_str(), repeatCount_); 
}

void Mesh::setVerticeDataContainer(void* data_, size_t offset_, size_t size_)
{
    getVerticeLayout(this->verticeLayoutId).setDataContainer(this->vertexDataContainerId, data_, offset_, size_); 
}


void* Mesh::getData()
{
    return getVerticeLayout(this->verticeLayoutId).dataContainers[this->vertexDataContainerId].data.data(); 
}

size_t Mesh::getSize()
{
    return getVerticeLayout(this->verticeLayoutId).dataContainers[this->vertexDataContainerId].data.size();
}

void Mesh::queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_)
{
    this->vbMemoryUploadId = addAndUploadMemoryEntry(std::format("{} Vertex data", this->name).c_str(), allocatorType_, memoryVisability_, VERTEX, this->getData(), this->getSize()); 
    // Create Index Buffer and fill it with data.
	this->ibMemoryUploadId = addAndUploadMemoryEntry(std::format("{} Index data", this->name).c_str(), allocatorType_, memoryVisability_, INDEX, this->indices.data(), this->indices.size() * sizeof(uint32_t));
}

void Mesh::markVerticeDirty(PoolId memberId_, uint32_t verticeId_)
{
    ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
    MemberBlueprint& rMember = rLayout.memberBlueprints[memberId_];
    uint64_t firstByte = verticeId_ * rLayout.size + rMember.firstByteId;
    uint64_t endByte = firstByte + rMember.size;

    /*
        The vertex data container and the vertex memory entry have the same layout, so a byte has the same offset in both.
        Changes join a DirtyInMesh only when their vertice is inside it or right next to it. Distant vertices get their own DirtyInMesh, so the bytes between them are not re-uploaded.
    */
    for(DirtyInMesh& rDirty : this->dirtyInMeshes)
    {
        if(verticeId_ + 1 >= rDirty.firstVerticeId && verticeId_ <= rDirty.lastVerticeId + 1)
        {
            uint64_t newFirstByte = std::min(rDirty.inSrcOffset, firstByte);
            uint64_t newEndByte = std::max(rDirty.inSrcOffset + rDirty.size, endByte);
            rDirty.firstVerticeId = std::min(rDirty.firstVerticeId, verticeId_);
            rDirty.lastVerticeId = std::max(rDirty.lastVerticeId, verticeId_);
            rDirty.inSrcOffset = newFirstByte;
            rDirty.inEntryOffset = newFirstByte;
            rDirty.size = newEndByte - newFirstByte;
            return;
        }
    }
    this->dirtyInMeshes.push_back({verticeId_, verticeId_, firstByte, firstByte, rMember.size});
}

void Mesh::queueUpdate()
{
    #ifdef ENGINE_DEBUG
        if(this->vbMemoryUploadId.id == UninitializedPoolId)
        {
            throw std::runtime_error(std::format("Mesh {}: Can't queue an update before the mesh was uploaded with queueUpload.", this->name));
        }
    #endif
    for(DirtyInMesh& rDirty : this->dirtyInMeshes)
    {
        updateMemoryEntry(this->vbMemoryUploadId, this->getData(), rDirty.inSrcOffset, rDirty.inEntryOffset, rDirty.size);
    }
    this->dirtyInMeshes.clear();
}
}
