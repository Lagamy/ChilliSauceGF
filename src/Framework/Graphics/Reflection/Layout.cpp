#include "Layout.h"
#include <algorithm>

namespace Graphics
{
uint32_t ReflectionLayout::getFirstAvailableByte()
{
    uint32_t firstAvailableByte = 0;
    for(uint32_t i = 0; i < this->variableBlueprints.allocatedSize(); i++)
    {
        if(this->variableBlueprints.alive[i])
        {
            firstAvailableByte = std::max<uint32_t>(firstAvailableByte, this->variableBlueprints.objects[i].lastByteId + 1);
        }
    }
    return firstAvailableByte;
}

/*
    The layout ends where its last variable ends (or at headroom, if that is later), rounded up to the alignment of the whole layout (the largest variable alignment,
    and for std140 at least 16). That way repeat unit N+1 (repeatUnitId * size) starts where a struct array of this layout would put it.
    Everything is worked out from the live variables (not only the last added one), so it is also right after a removal.
*/
void ReflectionLayout::updateSize()
{
    this->structAlignment = (this->alignment == STD140) ? 16 : 1;
    for(uint32_t i = 0; i < this->variableBlueprints.allocatedSize(); i++)
    {
        if(this->variableBlueprints.alive[i])
        {
            this->structAlignment = std::max(this->structAlignment, this->variableBlueprints.objects[i].alignmentRule);
        }
    }
    this->size = alignUp(std::max<size_t>(this->getFirstAvailableByte(), this->headroom), this->structAlignment);
}

/*
    Places the live variables again in the order of their orderPosition. A slot freed by a removal is reused by the next add, so the slot order is not the declaration order.
    The slots and generations stay, so the ids of the variables are still valid.
*/
void ReflectionLayout::relayout()
{
    std::vector<uint32_t> order;
    for(uint32_t i = 0; i < this->variableBlueprints.allocatedSize(); i++)
    {
        if(this->variableBlueprints.alive[i])
        {
            order.push_back(i);
        }
    }
    std::sort(order.begin(), order.end(), [this](uint32_t a_, uint32_t b_) { return this->variableBlueprints.objects[a_].orderPosition < this->variableBlueprints.objects[b_].orderPosition; });

    size_t cursor = 0;
    for(uint32_t slot : order)
    {
        VariableBlueprint& rVariable = this->variableBlueprints.objects[slot];
        rVariable.firstByteId = alignUp(cursor, rVariable.alignmentRule);
        rVariable.lastByteId = rVariable.firstByteId + rVariable.size - 1;
        cursor = rVariable.lastByteId + 1;
    }
}

void ReflectionLayout::removeVariableBlueprint(PoolId variableBlueprintId_)
{
    this->variableBlueprints.remove(variableBlueprintId_);
    this->relayout();
    this->updateSize();
}

PoolId ReflectionLayout::getVariableId(const VariableSelector& rSelector_)
{
    if(rSelector_.kind == SELECT_BY_ID)
    {
        return rSelector_.id;
    }
    if(rSelector_.kind == SELECT_BY_STRUCT_POSITION)
    {
        throw std::runtime_error("Layout: a struct position can only select a variable of a descriptor or push constant layout.");
    }
    for(uint32_t i = 0; i < this->variableBlueprints.allocatedSize(); i++)
    {
        if(this->variableBlueprints.alive[i] && this->variableBlueprints.objects[i].orderPosition == rSelector_.position)
        {
            return PoolId(i, this->variableBlueprints.generation[i]);
        }
    }
    throw std::runtime_error(std::format("Layout: no variable at position {}.", rSelector_.position));
}

uint32_t ReflectionLayout::getNextOrderPosition()
{
    uint32_t next = 0;
    for(uint32_t i = 0; i < this->variableBlueprints.allocatedSize(); i++)
    {
        if(this->variableBlueprints.alive[i])
        {
            next = std::max(next, this->variableBlueprints.objects[i].orderPosition + 1);
        }
    }
    return next;
}

void ReflectionLayout::throwIfOrderPositionUsed(const char* name_, uint32_t orderPosition_)
{
    for(uint32_t i = 0; i < this->variableBlueprints.allocatedSize(); i++)
    {
        if(this->variableBlueprints.alive[i] && this->variableBlueprints.objects[i].orderPosition == orderPosition_)
        {
            throw std::runtime_error(std::format("Layout: Order position {} of variable {} is already used by {}.", orderPosition_, name_, this->variableBlueprints.names[i]));
        }
    }
}

PoolId ReflectionLayout::addVariableBlueprint(const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    return this->addVariableBlueprint(name_, this->getNextOrderPosition(), dataType_, repeatCountPerDimension_);
}

PoolId ReflectionLayout::addVariableBlueprint(const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    return this->addVariableBlueprint(name_, this->getNextOrderPosition(), dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
}

/* The variable is created at the end and relayout moves it (and the ones after it) to where its position says. */
PoolId ReflectionLayout::addVariableBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    this->throwIfOrderPositionUsed(name_, orderPosition_);
    PoolId id = this->variableBlueprints.add(name_, dataType_, this->alignment, this->getFirstAvailableByte(), repeatCountPerDimension_);
    this->variableBlueprints[id].orderPosition = orderPosition_;
    this->relayout();
    this->updateSize();
    return id;
}

PoolId ReflectionLayout::addVariableBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
    this->throwIfOrderPositionUsed(name_, orderPosition_);
    PoolId id = this->variableBlueprints.add(name_, dataType_, rowCount_, columnCount_, this->alignment, this->getFirstAvailableByte(), repeatCountPerDimension_);
    this->variableBlueprints[id].orderPosition = orderPosition_;
    this->relayout();
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
