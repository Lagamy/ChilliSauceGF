// Contains data type, shape and alignment rules of a variable (a scalar or a matrix, each optionally repeated per dimension like int a[2][3])
#pragma once
#include "Utilities.h"
#include "RepeatShape.h"
#include <cstddef>
#include <cstdint> 
#include <functional>
#include <span>
#include <vector>

namespace Graphics
{ 
struct DataTypeMetadata 
{
	DataTypeEnum typeEnum;
	uint32_t elementCount = 1; // Doubles as columnCount if matrix  
	uint32_t rowCount = 1; // Relevant only if matrix. 1 by default, if bigger than this member is matrix. if bigger than 1, padding is applied per row, and not per element.  
};

struct DataShapeMetadata 
{
	DataShapeEnum shapeEnum; 
	uint32_t elementCount = 1; // Doubles as columnCount if matrix  
	uint32_t rowCount = 1; 
};

struct VariableBlueprint 
{ 
	DataTypeMetadata typeMetadata; 
	DataShapeMetadata shapeMetadata; 
	uint8_t alignmentRule; 
	std::vector<RepeatLayer> repeatLayers; // One layer per dimension, outermost first (int a[2][3]: {2, stride of a row}, {3, stride of an element}). Empty if the variable isn't repeated.

	size_t firstByteId; 
	size_t lastByteId; 
	size_t elementSize; // Bytes of one instance of the variable (what repeatLayers repeat), without the padding to its stride
	size_t size; // Bytes of all instances. Equals elementSize if the variable isn't repeated
	
	/*
		repeatCountPerDimension_ is the element count per dimension, outermost first ({2, 3} = a[2][3]). Empty = not repeated. Throws on a 0 count.
		A repeated variable aligns like an array (16 for std140) and every element starts at a multiple of the alignment rule. 
	*/
	// For Scalar (a scalar, a vector or a GLSL matrix type)
	VariableBlueprint(DataTypeEnum dataType_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_, const std::vector<uint32_t>& repeatCountPerDimension_); 
	// For Matrix (a grid of elements of dataType_)
	VariableBlueprint(DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_, const std::vector<uint32_t>& repeatCountPerDimension_); 

	// Byte offset from firstByteId. Both respect STD alignment rules and throw if an id is out of bounds
	// Offset of the instance picked by repeatIdPerDimension_ (one id per dimension, outermost first, empty for a variable that isn't repeated)
	size_t getRepeatOffset(std::span<const uint32_t> repeatIdPerDimension_) const; 
	// Offset inside one instance
	size_t getMatrixElementOffset(uint32_t rowId_, uint32_t columnId_) const;

	// Distance between two elements of a matrix row, and between two rows of a matrix (both include padding). Not meaningful for a scalar.
	// Distance between two columns of a GLSL matrix type (mat3, mat4...), the column's bytes rounded up to the alignment rule. Not meaningful for a type with 1 row.
	size_t getColumnStride() const;
	size_t getElementStride() const;
	size_t getRowStride() const;
};
}

