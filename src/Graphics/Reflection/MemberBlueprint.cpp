#include "MemberBlueprint.h"

namespace Graphics
{
MemberBlueprint::MemberBlueprint(DataTypeEnum dataType_, uint32_t firstByteId_) 
{ 
	this->dataType = dataType_; 
	this->columnCount = dataTypeToColumnsAndRows[dataType_].x; 
	this->rowCount = dataTypeToColumnsAndRows[dataType_].y; 

	this->firstByteId = firstByteId_; 
	this->size = dataTypeToElementCount[dataType_] * 4; 
	this->lastByteId = firstByteId_ + this->size - 1; 
} 
}