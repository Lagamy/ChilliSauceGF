#pragma once
#include "PoolMap.h"
#include "DataContainerConcepts.h"
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
    size_t size = 0;

    PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_);
    PoolId getMemberIdFromName(const char* name_);

    template<typename T, WritableDataContainer C>
    void setMember(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, T data_)
    {
        MemberBlueprint& rMemberBlueprint = this->memberBlueprints[memberBlueprintId_];
        if(GPUTypeMap<T>::value != rMemberBlueprint.dataType)
        {
			throw std::runtime_error(std::format("Layout: Can't set Member {} with type {} to {} type.",
            this->memberBlueprints.getName(memberBlueprintId_),
            dataTypeToName[rMemberBlueprint.dataType], dataTypeToName[GPUTypeMap<T>::value]));
        }
        if(repeatUnitId_ >= rContainer_.repeatCount)
        {
			throw std::runtime_error(std::format("Layout: Repeat Unit {} is out of bounds.", repeatUnitId_));
        }
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        rContainer_.write(repeatUnitOffset + rMemberBlueprint.firstByteId, &data_, rMemberBlueprint.size);
    };

    template<typename T, ReadableDataContainer C>
    const T getMember(const C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_)
    {
        MemberBlueprint& rMemberBlueprint = this->memberBlueprints[memberBlueprintId_];
        if(GPUTypeMap<T>::value != rMemberBlueprint.dataType)
        {
			throw std::runtime_error(std::format("Layout: Can't get Member {} with type {} as {} type.",
            this->memberBlueprints.getName(memberBlueprintId_),
            dataTypeToName[rMemberBlueprint.dataType], dataTypeToName[GPUTypeMap<T>::value]));
        }
        if(repeatUnitId_ >= rContainer_.repeatCount)
        {
			throw std::runtime_error(std::format("Layout: Repeat Unit {} is out of bounds.", repeatUnitId_));
        }
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        T data;
        rContainer_.read(repeatUnitOffset + rMemberBlueprint.firstByteId, &data, rMemberBlueprint.size);
        return data;
    };
};
}