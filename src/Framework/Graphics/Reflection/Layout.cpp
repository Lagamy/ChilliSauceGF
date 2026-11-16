#include "Layout.h"
#include <algorithm>

namespace Graphics
{
uint32_t ReflectionLayout::getFirstAvailableByte()
{
    if(this->variableBlueprints.allocatedSize() == 0)
    {
        return 0;
    }
    return this->variableBlueprints.back().lastByteId + 1;
}

/*
    The member may start after padding, so the layout ends where its last member ends, rounded up to the alignment of the whole layout (the largest member alignment,
    and for std140 at least 16). That way repeat unit N+1 (repeatUnitId * size) starts where a struct array of this layout would put it.
*/
void ReflectionLayout::updateSize()
{
    this->structAlignment = std::max(this->structAlignment, this->variableBlueprints.back().alignmentRule);
    if(this->alignment == STD140)
    {
        this->structAlignment = std::max<uint8_t>(this->structAlignment, 16);
    }
    this->size = alignUp(this->variableBlueprints.back().lastByteId + 1, this->structAlignment);
}

PoolId ReflectionLayout::addVariableBlueprint(const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    PoolId id = this->variableBlueprints.add(name_, dataType_, this->alignment, this->getFirstAvailableByte(), repeatCountPerDimension_);
    this->updateSize();
    return id;
}

PoolId ReflectionLayout::addVariableBlueprint(const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    PoolId id = this->variableBlueprints.add(name_, dataType_, rowCount_, columnCount_, this->alignment, this->getFirstAvailableByte(), repeatCountPerDimension_);
    this->updateSize();
    return id;
}


PoolId ReflectionLayout::getMemberIdFromName(const char* name_)
{
    return this->variableBlueprints.getIdByName(name_);
};

VariableBlueprint& ReflectionLayout::getCheckedMember(PoolId variableBlueprintId_, DataTypeEnum dataType_, DataShapeEnum shape_, uint32_t repeatUnitId_, uint32_t repeatCount_)
{
    VariableBlueprint& rVariableBlueprint = this->variableBlueprints[variableBlueprintId_];
    if(dataType_ != rVariableBlueprint.typeMetadata.typeEnum || shape_ != rVariableBlueprint.shapeMetadata.shapeEnum)
    {
        throw std::runtime_error(std::format("Layout: Can't access Member {} of type {} ({}) as type {} ({}).",
        this->variableBlueprints.getName(variableBlueprintId_),
        dataTypeToName[rVariableBlueprint.typeMetadata.typeEnum], dataShapeToName[rVariableBlueprint.shapeMetadata.shapeEnum],
        dataTypeToName[dataType_], dataShapeToName[shape_]));
    }
    if(repeatUnitId_ >= repeatCount_)
    {
        throw std::runtime_error(std::format("Layout: Repeat Unit {} is out of bounds.", repeatUnitId_));
    }
    return rVariableBlueprint;
}
}
