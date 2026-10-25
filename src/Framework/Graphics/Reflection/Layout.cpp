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
}