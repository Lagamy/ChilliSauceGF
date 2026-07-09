#include "Layout.h"

namespace Graphics
{
PoolId ReflectionLayout::addMemberBlueprint(const char* name_, DataTypeEnum dataType_)
{
    PoolId id;
    if(this->memberBlueprints.size() == 0)
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

void ReflectionLayout::setDataContainer(PoolId derivedDataContainerId_, void* data_, size_t offset_, size_t size_)
{
    std::vector<char>& rDataContainer = this->dataContainers.get(derivedDataContainerId_); 
    std::memcpy(&rDataContainer[offset_], data_, size_); 
}

PoolId ReflectionLayout::createDataContainer(const char* name_, uint32_t repeatCount_)
{
    PoolId id = this->dataContainers.add(name_);
    std::vector<char>& rDataContainer = this->dataContainers.get(id); 
    rDataContainer.resize(this->size * repeatCount_); 
    return id; 
}
}