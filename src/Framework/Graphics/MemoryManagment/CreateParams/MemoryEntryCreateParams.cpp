#include "MemoryEntryCreateParams.h"
#include <format>
#include <stdexcept>

namespace Graphics
{
	void MemoryEntryCreateParams::validate() const
	{
		if(this->name == nullptr)
		{
			throw std::runtime_error("Memory Entry Create Params: name is required.");
		}
		if(this->size == 0)
		{
			throw std::runtime_error(std::format("Memory Entry Create Params: Entry {} has size 0.", this->name));
		}
		if(this->allocatorType == ALLOCATOR_TYPE_UNSET)
		{
			throw std::runtime_error(std::format("Memory Entry Create Params: Entry {} has no allocatorType.", this->name));
		}
		if(this->memoryVisability == MEMORY_VISABILITY_UNSET)
		{
			throw std::runtime_error(std::format("Memory Entry Create Params: Entry {} has no memoryVisability.", this->name));
		}
		if(this->bufferType == BUFFER_TYPE_UNSET)
		{
			throw std::runtime_error(std::format("Memory Entry Create Params: Entry {} has no bufferType.", this->name));
		}
		this->access.validate(this->name, this->memoryVisability);
	}
}
