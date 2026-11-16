#include "VariableBlueprint.h"
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
size_t getColumnStride(DataTypeEnum dataType_, uint8_t alignmentRule_)
{
	return alignUp(dataTypeToColumnsAndRows[dataType_].y * 4, alignmentRule_);
}

size_t getTypeSize(DataTypeEnum dataType_, uint8_t alignmentRule_)
{
	uint32_t columnCount = dataTypeToColumnsAndRows[dataType_].x;
	uint32_t rowCount = dataTypeToColumnsAndRows[dataType_].y;
	if(rowCount > 1)
	{
		return columnCount * Graphics::getColumnStride(dataType_, alignmentRule_);
	}
	return dataTypeToInternalElementCount[dataType_] * 4;
}

/* Elements of a matrix row (and of a repeated variable) start at a multiple of the alignment rule. Same stride as Array::init. */
size_t getElementStride(DataTypeEnum dataType_, uint8_t alignmentRule_)
{
	return alignUp(getTypeSize(dataType_, alignmentRule_), alignmentRule_);
}

/*
	The layers of a repeated variable (outermost first). The innermost stride is the instance size rounded up to the alignment rule, every outer stride is the stride of the layer inside it times its count.
	Empty counts = not repeated. Throws on a 0 count.
*/
std::vector<RepeatLayer> makeRepeatLayers(const std::vector<uint32_t>& repeatCountPerDimension_, size_t elementSize_, uint8_t alignmentRule_)
{
	if(repeatCountPerDimension_.empty())
	{
		return {};
	}
	return RepeatShape::fromCounts(repeatCountPerDimension_, alignUp(elementSize_, alignmentRule_)).layers;
}

size_t getTotalSize(const std::vector<RepeatLayer>& repeatLayers_, size_t elementSize_)
{
	if(repeatLayers_.empty())
	{
		return elementSize_;
	}
	return repeatLayers_.front().count * repeatLayers_.front().stride;
}

/* Rows are arrays of columns. Same stride as Matrix::init. */
size_t getRowStride(DataTypeEnum dataType_, uint32_t columnCount_, uint8_t alignmentRule_)
{
	return alignUp(columnCount_ * getElementStride(dataType_, alignmentRule_), alignmentRule_);
}
}

VariableBlueprint::VariableBlueprint(DataTypeEnum dataType_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_, const std::vector<uint32_t>& repeatCountPerDimension_)
{ 
	this->alignmentRule = resolveSTDPaddingRule(alignmentLayout_, dataType_, !repeatCountPerDimension_.empty()); 
	this->typeMetadata.typeEnum = dataType_; 
	this->typeMetadata.elementCount = dataTypeToColumnsAndRows[dataType_].x; 
	this->typeMetadata.rowCount = dataTypeToColumnsAndRows[dataType_].y; 

	this->shapeMetadata.shapeEnum = SCALAR; 

	this->firstByteId = alignUp(firstAvailableByteId_, this->alignmentRule);
	this->elementSize = getTypeSize(dataType_, this->alignmentRule);
	this->repeatLayers = makeRepeatLayers(repeatCountPerDimension_, this->elementSize, this->alignmentRule);
	this->size = getTotalSize(this->repeatLayers, this->elementSize);
	this->lastByteId = this->firstByteId + this->size - 1; 
} 

VariableBlueprint::VariableBlueprint(DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_, const std::vector<uint32_t>& repeatCountPerDimension_)
{
	this->alignmentRule = resolveSTDPaddingRule(alignmentLayout_, dataType_, true); 
	this->typeMetadata.typeEnum = dataType_; 
	this->typeMetadata.elementCount = dataTypeToColumnsAndRows[dataType_].x; 
	this->typeMetadata.rowCount = dataTypeToColumnsAndRows[dataType_].y; 

	this->shapeMetadata.shapeEnum = MATRIX; 
	this->shapeMetadata.rowCount = rowCount_; 
	this->shapeMetadata.elementCount = columnCount_;

	this->firstByteId = alignUp(firstAvailableByteId_, this->alignmentRule);
	this->elementSize = rowCount_ * Graphics::getRowStride(dataType_, columnCount_, this->alignmentRule);
	this->repeatLayers = makeRepeatLayers(repeatCountPerDimension_, this->elementSize, this->alignmentRule);
	this->size = getTotalSize(this->repeatLayers, this->elementSize);
	this->lastByteId = this->firstByteId + this->size - 1;
};

size_t VariableBlueprint::getColumnStride() const
{
	return Graphics::getColumnStride(this->typeMetadata.typeEnum, this->alignmentRule);
}

size_t VariableBlueprint::getElementStride() const
{
	return Graphics::getElementStride(this->typeMetadata.typeEnum, this->alignmentRule);
}

size_t VariableBlueprint::getRowStride() const
{
	return Graphics::getRowStride(this->typeMetadata.typeEnum, this->shapeMetadata.elementCount, this->alignmentRule);
}

size_t VariableBlueprint::getRepeatOffset(std::span<const uint32_t> repeatIdPerDimension_) const
{
	return computeRepeatOffset(this->repeatLayers, repeatIdPerDimension_);
}

size_t VariableBlueprint::getMatrixElementOffset(uint32_t rowId_, uint32_t columnId_) const
{
	if(rowId_ >= this->shapeMetadata.rowCount || columnId_ >= this->shapeMetadata.elementCount)
	{
		throw std::runtime_error(std::format("Member Blueprint: Element [{}][{}] is out of bounds, the matrix has {} rows and {} columns.",
		rowId_, columnId_, this->shapeMetadata.rowCount, this->shapeMetadata.elementCount));
	}
	return rowId_ * this->getRowStride() + columnId_ * this->getElementStride();
}
}