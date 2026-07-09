// Containes data type and alignemnt rules of a Member 
#include "Utilities.h"
#include <cstddef>
#include <cstdint> 
#include <functional>

namespace Graphics
{ 
struct MemberBlueprint 
{ 
	DataTypeEnum dataType; 
	uint32_t columnCount; // Doubles as count if not matrix  
	uint32_t rowCount; // Relevant only if matrix. 1 by default, if bigger than this member is matrix. if bigger than 1, padding is applied per row, and not per element.  
	
	size_t firstByteId; 
	size_t lastByteId; 
	size_t size; 
	
	MemberBlueprint(DataTypeEnum dataType_, uint32_t firstByteId_); 
};
} 

