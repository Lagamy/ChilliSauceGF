#pragma once 
#include "PoolMap.h"
#include "DataContainer.h"
#include "MemberBlueprint.h"
#include "Utilities.h"
#include <cstring>
#include <stdexcept>
#include <format>

namespace Graphics { 
struct ReflectionLayout
{
    AlignmentLayoutEnum alignment = NONE; 
    uint32_t alignmentRule; 
    PoolMap<MemberBlueprint> memberBlueprints = ("Member Blueprints"); 
    PoolMap<DataContainer> dataContainers = ("Data Containers"); 
    size_t size = 0; 

    PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_);
    PoolId getMemberIdFromName(const char* name_);
    PoolId createDataContainer(const char* name_, uint32_t repeatCount_); // repeatCount - exist for repeating data with same blueprint, like vertices; 

    template<typename T>
    void setMemberInDataContainer(PoolId memberBlueprintId_, T data_, PoolId dataContainerId_, uint32_t repeatUnitId_)
    {
        MemberBlueprint& rMemberBlueprint = this->memberBlueprints[memberBlueprintId_]; 
        if(GPUTypeMap<T>::value != rMemberBlueprint.dataType)
        {
			throw std::runtime_error(std::format("{} Data Container: Can't set Member {} with type {} to {} type inside of {} ", 
            dataContainers.getName(dataContainerId_), this->memberBlueprints.getName(memberBlueprintId_), 
            dataTypeToName[rMemberBlueprint.dataType], dataTypeToName[GPUTypeMap<T>::value], this->dataContainers.getName(dataContainerId_)); 
        }
        DataContainer& rDataContainer = this->dataContainers[dataContainerId_];
        if(repeatUnitId_ >= rDataContainer.repeatCount)
        {
            std::stringstream errorMessageStream; 
			errorMessageStream << this->dataContainers.getName(dataContainerId_) << " Data Container: Repeat Unit " << repeatUnitId_ << " is out of bounds." << "\n"; 
			throw std::runtime_error(errorMessageStream.str());
        }
        size_t repeatUnitOffset = repeatUnitId_ * this->size;  
        std::memcpy(&rDataContainer.data[repeatUnitOffset + rMemberBlueprint.firstByteId], &data_, rMemberBlueprint.size);
    }; 

    void setDataContainer(PoolId derivedDataContainerId_, void* data_, size_t offset_, size_t size_); // Be carefull, it doesn't gurantee anything. 
};
}