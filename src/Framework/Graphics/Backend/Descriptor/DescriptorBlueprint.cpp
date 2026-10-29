#include "DescriptorBlueprint.h"
#include "Api.h"
#include <format>
#include <stdexcept>

namespace Graphics 
{
    /*
        Adds a member to the ReflectionLayout every derived descriptor is sized from.
        A set whose memory was already allocated would need a new, bigger MemoryEntry and a GPU copy of the old bytes (which needs the GPU to be idle), so that case isn't supported.
        Sets that only exist on the CPU side pick the new size up in DescriptorSet::allocateDescriptorsMemory.
    */
    PoolId DescriptorBlueprint::addMember(const char* name_, DataTypeEnum dataType_) 
    {
        for(const PoolId& rSetId : this->derivedDescriptors) 
        {
            if(getDescriptorsManager().descriptorSet[rSetId].descriptorsMemoryAllocated)
            {
                throw std::runtime_error(std::format("Descriptor Blueprint {}: Can't add member {}. Memory of a derived set was already allocated, copying its data to a bigger entry isn't supported.", this->name, name_));
            }
        }
        return getReflectionSystem().layoutsPerBufferType[this->bufferType - 1][this->layoutId].addMemberBlueprint(name_, dataType_); // - 1 due to INDEX layout not existing
    }
}
