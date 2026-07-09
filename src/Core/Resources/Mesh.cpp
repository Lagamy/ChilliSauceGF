#include "Mesh.h"
#include "Api.h"
#include "Utilities.h"
#include <stdexcept>

namespace Graphics
{
template<typename T>
void Mesh::setVerticeMember(PoolId memberId_, uint32_t repeateUnitId_, T data_)
{
    if(!this->created)
    {
        throw std::runtime_error("Create mesh before setting anything.\n"); 
    }
	getVerticeLayout(this->verticeLayoutId).setMemberInDataContainer(memberId_, data_, this->vertexDataContainerId, repeateUnitId_);
}

void Mesh::create(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_)
{
    getVerticeLayout(verticeLayoutId_).createDataContainer(name_, repeatCount_); 
    
}

void Mesh::setVerticeDataContainer(void* data_, size_t offset_, size_t size_)
{
    if(!this->created)
    {
        throw std::runtime_error("Create mesh before setting anything.\n"); 
    }
    getVerticeLayout(this->verticeLayoutId).setDataContainer(this->vertexDataContainerId, data_, offset_, size_); 
}


void* Mesh::getData()
{
    if(!this->created)
    {
        throw std::runtime_error("Create mesh before setting anything.\n"); 
    }
    return getVerticeLayout(this->verticeLayoutId).dataContainers.get(this->vertexDataContainerId).data.data(); 
}

size_t Mesh::getSize()
{
    if(!this->created)
    {
        throw std::runtime_error("Create mesh before setting anything.\n");
    }
    return getVerticeLayout(this->verticeLayoutId).dataContainers.get(this->vertexDataContainerId).data.size();
}

} 
