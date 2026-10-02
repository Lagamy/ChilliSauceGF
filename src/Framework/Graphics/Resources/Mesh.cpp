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
        verticeToDirtyId maps every vertice inside a DirtyInMesh to that run, so we look up the vertice itself and its two neighbours instead of scanning all runs.
        Changes join a DirtyInMesh only when their vertice is inside it or right next to it. Distant vertices get their own DirtyInMesh, so the bytes between them are not re-uploaded.
    */
    auto found = this->verticeToDirtyId.find(verticeId_);
    if(found == this->verticeToDirtyId.end() && verticeId_ > 0)
    {
        found = this->verticeToDirtyId.find(verticeId_ - 1);
    }
    if(found == this->verticeToDirtyId.end())
    {
        found = this->verticeToDirtyId.find(verticeId_ + 1);
    }

    if(found == this->verticeToDirtyId.end())
    {
        this->verticeToDirtyId[verticeId_] = static_cast<uint32_t>(this->dirtyInMesh.size());
        this->dirtyInMesh.push_back({verticeId_, verticeId_, firstByte, firstByte, rMember.size});
        return;
    }

    DirtyInMesh& rDirty = this->dirtyInMesh[found->second];
    uint64_t newFirstByte = std::min(rDirty.inSrcOffset, firstByte);
    uint64_t newEndByte = std::max(rDirty.inSrcOffset + rDirty.size, endByte);
    rDirty.firstVerticeId = std::min(rDirty.firstVerticeId, verticeId_);
    rDirty.lastVerticeId = std::max(rDirty.lastVerticeId, verticeId_);
    rDirty.inSrcOffset = newFirstByte;
    rDirty.inEntryOffset = newFirstByte;
    rDirty.size = newEndByte - newFirstByte;
    this->verticeToDirtyId[verticeId_] = found->second;
}

void Mesh::queueUpdate()
{
    #ifdef ENGINE_DEBUG
        if(this->vbMemoryUploadId.id == UninitializedPoolId)
        {
            throw std::runtime_error(std::format("Mesh {}: Can't queue an update before the mesh was uploaded with queueUpload.", this->name));
        }
    #endif
    for(DirtyInMesh& rDirty : this->dirtyInMesh)
    {
        updateMemoryEntry(this->vbMemoryUploadId, this->getData(), rDirty.inSrcOffset, rDirty.inEntryOffset, rDirty.size);
    }
    this->dirtyInMesh.clear();
    this->verticeToDirtyId.clear();
}
}
