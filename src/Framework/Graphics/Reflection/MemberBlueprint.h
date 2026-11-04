// Containes data type and alignemnt rules of a Member 
#pragma once
#include "Utilities.h"
#include <cstddef>
#include <cstdint> 
#include <functional>

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

struct MemberBlueprint 
{ 
	DataTypeMetadata typeMetadata; 
	DataShapeMetadata shapeMetadata; 
	uint8_t alignmentRule; 

	size_t firstByteId; 
	size_t lastByteId; 
	size_t size; 
	
	// For Scalar
	MemberBlueprint(DataTypeEnum dataType_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_); 
	// For Array
	MemberBlueprint(DataTypeEnum dataType_, uint32_t elementCount_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_); 
	// For Matrix 
	MemberBlueprint(DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, STDAlignmentLayoutEnum alignmentLayout_, uint32_t firstAvailableByteId_); 

	// Byte offset from firstByteId. Both respect STD alignment rules and throw if the id is out of bounds
	size_t getArrayElementOffset(uint32_t elementId_) const; 
	size_t getMatrixElementOffset(uint32_t rowId_, uint32_t columnId_) const;

	// Distance between two elements of an array / of a matrix row, and between two rows of a matrix (both include padding). Not meaningful for a scalar.
	size_t getElementStride() const;
	size_t getRowStride() const;
};
}

