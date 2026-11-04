#include "DescriptorBlueprint.h"
#include "Api.h"
#include <format>
#include <stdexcept>

namespace Graphics 
{
    /*
        Returns the ReflectionLayout every derived descriptor is sized from, to add a member to.
        A set whose memory was already allocated would need a new, bigger MemoryEntry and a GPU copy of the old bytes (which needs the GPU to be idle), so that case isn't supported.
        Sets that only exist on the CPU side pick the new size up in DescriptorSet::allocateDescriptorsMemory.
    */
    ReflectionLayout& DescriptorBlueprint::getLayoutForNewMember(const char* memberName_)
    {
        for(const PoolId& rSetId : this->derivedDescriptors) 
        {
            if(getDescriptorsManager().descriptorSet[rSetId].descriptorsMemoryAllocated)
            {
                throw std::runtime_error(std::format("Descriptor Blueprint {}: Can't add member {}. Memory of a derived set was already allocated, copying its data to a bigger entry isn't supported.", this->name, memberName_));
            }
        }
        return getReflectionSystem().layoutsPerBufferType[this->bufferType - 1][this->layoutId]; // - 1 due to INDEX layout not existing
    }

    PoolId DescriptorBlueprint::addMember(const char* name_, DataTypeEnum dataType_) 
    {
        return this->getLayoutForNewMember(name_).addMemberBlueprint(name_, dataType_);
    }

    PoolId DescriptorBlueprint::addMember(const char* name_, DataTypeEnum dataType_, uint32_t elementCount_) 
    {
        return this->getLayoutForNewMember(name_).addMemberBlueprint(name_, dataType_, elementCount_);
    }

    PoolId DescriptorBlueprint::addMember(const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_) 
    {
        return this->getLayoutForNewMember(name_).addMemberBlueprint(name_, dataType_, rowCount_, columnCount_);
    }
}
