#include "MemberBlueprint.h"
#include "Utilities.h"
#include <stdexcept>
#include <format>

namespace Graphics
{
namespace
{
/*
	A matrix type (rows > 1) is an array of column vectors, so every column starts at a multiple of the alignment rule.
	Scalars and vectors are packed. Columns/rows follow GLSL: matCxR has C columns of R components.
*/
size_t getTypeSize(DataTypeEnum dataType_, uint8_t alignmentRule_)
{
	uint32_t columnCount = dataTypeToColumnsAndRows[dataType_].x;
	uint32_t rowCount = dataTypeToColumnsAndRows[dataType_].y;
	if(rowCount > 1)
	{
		return columnCount * alignUp(rowCount * 4, alignmentRule_);
	}
	return dataTypeToInternalElementCount[dataType_] * 4;
}

/* Elements of an array (and of a matrix row) start at a multiple of the alignment rule. Same stride as Array::init. */
size_t getElementStride(DataTypeEnum dataType_, uint8_t alignmentRule_)
{
	return alignUp(getTypeSize(dataType_, alignmentRule_), alignmentRule_);
}

/* Rows are arrays of columns. Same stride as Matrix::init. */
size_t getRowStride(DataTypeEnum dataType_, uint32_t columnCount_, uint8_t alignmentRule_)
{
	return alignUp(columnCount_ * getElementStride(dataType_, alignmentRule_), alignmentRule_);
}
}

MemberBlueprint::MemberBlueprint(DataTypeEnum dataType_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_)
{ 
	this->alignmentRule = resolveSTDPaddingRule(alignmentLayout_, dataType_, false); 
	this->typeMetadata.typeEnum = dataType_; 
	this->typeMetadata.elementCount = dataTypeToColumnsAndRows[dataType_].x; 
	this->typeMetadata.rowCount = dataTypeToColumnsAndRows[dataType_].y; 

	this->shapeMetadata.shapeEnum = SCALAR; 

	this->firstByteId = alignUp(firstAvailableByteId_, this->alignmentRule);
	this->size = getTypeSize(dataType_, this->alignmentRule);
	this->lastByteId = this->firstByteId + this->size - 1; 
} 


MemberBlueprint::MemberBlueprint(DataTypeEnum dataType_, uint32_t elementCount_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_) 
{
	this->alignmentRule = resolveSTDPaddingRule(alignmentLayout_, dataType_, true); 
	this->typeMetadata.typeEnum = dataType_; 
	this->typeMetadata.elementCount = dataTypeToColumnsAndRows[dataType_].x; 
	this->typeMetadata.rowCount = dataTypeToColumnsAndRows[dataType_].y; 

	this->shapeMetadata.shapeEnum = ARRAY;
	this->shapeMetadata.elementCount = elementCount_; 
	

	this->firstByteId = alignUp(firstAvailableByteId_, this->alignmentRule);
	this->size = elementCount_ * Graphics::getElementStride(dataType_, this->alignmentRule);
	this->lastByteId = this->firstByteId + this->size - 1;
}; 

MemberBlueprint::MemberBlueprint(DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_)
{
	this->alignmentRule = resolveSTDPaddingRule(alignmentLayout_, dataType_, true); 
	this->typeMetadata.typeEnum = dataType_; 
	this->typeMetadata.elementCount = dataTypeToColumnsAndRows[dataType_].x; 
	this->typeMetadata.rowCount = dataTypeToColumnsAndRows[dataType_].y; 

	this->shapeMetadata.shapeEnum = MATRIX; 
	this->shapeMetadata.rowCount = rowCount_; 
	this->shapeMetadata.elementCount = columnCount_;

	this->firstByteId = alignUp(firstAvailableByteId_, this->alignmentRule);
	this->size = rowCount_ * Graphics::getRowStride(dataType_, columnCount_, this->alignmentRule);
	this->lastByteId = this->firstByteId + this->size - 1;
};

size_t MemberBlueprint::getElementStride() const
{
	return Graphics::getElementStride(this->typeMetadata.typeEnum, this->alignmentRule);
}

size_t MemberBlueprint::getRowStride() const
{
	return Graphics::getRowStride(this->typeMetadata.typeEnum, this->shapeMetadata.elementCount, this->alignmentRule);
}

size_t MemberBlueprint::getArrayElementOffset(uint32_t elementId_) const
{
	if(elementId_ >= this->shapeMetadata.elementCount)
	{
		throw std::runtime_error(std::format("Member Blueprint: Element {} is out of bounds, the array has {} elements.", elementId_, this->shapeMetadata.elementCount));
	}
	return elementId_ * this->getElementStride();
}

size_t MemberBlueprint::getMatrixElementOffset(uint32_t rowId_, uint32_t columnId_) const
{
	if(rowId_ >= this->shapeMetadata.rowCount || columnId_ >= this->shapeMetadata.elementCount)
	{
		throw std::runtime_error(std::format("Member Blueprint: Element [{}][{}] is out of bounds, the matrix has {} rows and {} columns.",
		rowId_, columnId_, this->shapeMetadata.rowCount, this->shapeMetadata.elementCount));
	}
	return rowId_ * this->getRowStride() + columnId_ * this->getElementStride();
}
}