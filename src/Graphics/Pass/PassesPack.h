#pragma once
#include <vector>
#include "SubmissionBatch.h"
#include "Pass.h"
#include "SubmissionBatch.h"

// Im not using Pools here, due to Passes being not dynamically changable by design(can change) 
namespace Graphics
{
struct PassesPack { 
	std::array<std::vector<Pass>, 3> passesPerQueue; 
};
}
