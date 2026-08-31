#include "Layout.h"

namespace Graphics
{
PoolId ReflectionLayout::addMemberBlueprint(const char* name_, DataTypeEnum dataType_)
{
    PoolId id;
    if(this->memberBlueprints.allocatedSize() == 0)
    {
        id = this->memberBlueprints.add(name_, dataType_, 0);  
    }
    else 
    {
        id = this->memberBlueprints.add(name_, dataType_, this->memberBlueprints.back().lastByteId + 1); 
    }
    this->size += this->memberBlueprints.back().size; 
    return id;
}


PoolId ReflectionLayout::getMemberIdFromName(const char* name_)
{
    return this->memberBlueprints.getIdByName(name_);
}; 

void ReflectionLayout::setDataContainer(PoolId derivedDataContainerId_, void* data_, size_t offset_, size_t size_)
{
    DataContainer& rDataContainer = this->dataContainers[derivedDataContainerId_]; 
    std::memcpy(&rDataContainer.data[offset_], data_, size_); 
}

PoolId ReflectionLayout::createDataContainer(const char* name_, uint32_t repeatCount_)
{
    PoolId id = this->dataContainers.add(name_);
    DataContainer& rDataContainer = this->dataContainers[id]; 
    rDataContainer.data.resize(this->size * repeatCount_); 
    rDataContainer.repeatCount = repeatCount_; 
    return id; 
}
}