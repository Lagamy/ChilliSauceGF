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
    return getVerticeLayout(this->verticeLayoutId).dataContainers.get(this->vertexDataContainerId).data.data(); 
}

size_t Mesh::getSize()
{
    return getVerticeLayout(this->verticeLayoutId).dataContainers.get(this->vertexDataContainerId).data.size();
}

void Mesh::queueStaticGPUUpload()
{
    std::stringstream uploadName;
    uploadName << this->name << " Vertex data"; 
    this->vbMemoryId = addUpload(uploadName.str().c_str(), STATIC, VERTEX, this->getData(), this->getSize()); 
	uploadName.clear(); 
    // Create Index Buffer and fill it with data.
    uploadName << this->name << " Index data"; 
	this->ibMemoryId = addUpload(uploadName.str().c_str(), STATIC, INDEX, this->indices.data(), this->indices.size() * sizeof(uint32_t)); 
}
} 
