#pragma once 
#include "Pool.h"
#include "DataContainer.h"
#include "MemberBlueprint.h"
#include "Utilities.h"
#include <cstring>
#include <stdexcept>

namespace Graphics { 
struct ReflectionLayout
{
    AlignmentLayoutEnum alignment = NONE; 
    uint32_t alignmentRule; 
    Pool<MemberBlueprint> memberBlueprints; 
    Pool<DataContainer> dataContainers; 
    size_t size = 0; 

    PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_);
    PoolId createDataContainer(const char* name_, uint32_t repeatCount_); // repeatCount - exist for repeating data with same blueprint, like vertices; 
    
    template<typename T>
    void setMemberInDataContainer(PoolId memberBlueprintId_, T data_, PoolId dataContainerId_, uint32_t repeatUnitId_)
    {
        MemberBlueprint& rMemberBlueprint = this->memberBlueprints.get(memberBlueprintId_); 
        if(GPUTypeMap<T>::value != rMemberBlueprint.dataType)
        {
            std::stringstream errorMessageStream; 
			errorMessageStream << dataContainers.getName(dataContainerId_)<< " Data Container: Can't set Member " << this->memberBlueprints.getName(memberBlueprintId_) << " with type " << dataTypeToName[rMemberBlueprint.dataType] << " to "
            << dataTypeToName[GPUTypeMap<T>::value] << " type inside of " << this->dataContainers.getName(dataContainerId_) << ".\n"; 
			throw std::runtime_error(errorMessageStream.str());
        }
        DataContainer& rDataContainer = this->dataContainers.get(dataContainerId_);
        if(repeatUnitId_ >= rDataContainer.repeatCount)
        {
            std::stringstream errorMessageStream; 
			errorMessageStream << this->dataContainers.getName(dataContainerId_) << " Data Container: Repeat Unit " << repeatUnitId_ << " is out of bounds." << "\n"; 
			throw std::runtime_error(errorMessageStream.str());
        }
        size_t repeatUnitOffset = repeatUnitId_ * size;  
        std::memcpy(&rDataContainer.data[repeatUnitOffset + rMemberBlueprint.firstByteId], &data_, rMemberBlueprint.size);
    }; 

    void setDataContainer(PoolId derivedDataContainerId_, void* data_, size_t offset_, size_t size_); // Be carefull, it doesn't gurantee anything. 
};
}