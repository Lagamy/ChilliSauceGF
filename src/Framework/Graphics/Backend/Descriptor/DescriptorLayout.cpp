#include "DescriptorLayout.h"
#include "Api.h"
#include <algorithm>
#include <format>
#include <stdexcept>

namespace Graphics 
{
    /*
        A set whose memory was already allocated would need a new, bigger MemoryEntry and a GPU copy of the old bytes (which needs the GPU to be idle), so that case isn't supported.
        Sets that only exist on the CPU side pick the new shape up in DescriptorSet::allocateDescriptorsMemory.
    */
    void DescriptorLayout::throwIfMemoryAllocated(const char* memberName_) const
    {
        for(const PoolId& rSetId : this->derivedDescriptors) 
        {
            if(getDescriptorsManager().descriptorSet[rSetId].descriptorsMemoryAllocated)
            {
                throw std::runtime_error(std::format("Descriptor Layout {}: Can't add {}. Memory of a derived set was already allocated, copying its data to a bigger entry isn't supported.", this->name, memberName_));
            }
        }
    }

    /* A member's variables are padded by the rule of the buffer: uniform blocks follow std140 and storage blocks std430. */
    PoolId DescriptorLayout::addVariableMember(const char* name_)
    {
        this->throwIfMemoryAllocated(name_);
        STDAlignmentLayoutEnum alignment = (this->bufferType == UNIFORM_BUFFER) ? STD140 : STD430;
        return this->members.add(name_, VARIABLE_MEMBER, alignment, std::vector<uint32_t>{});
    }

    PoolId DescriptorLayout::registerVariable(PoolId memberId_, PoolId variableId_)
    {
        return this->variables.add(memberId_, variableId_);
    }

    MemberBlueprint& DescriptorLayout::getStructMember(PoolId structMemberId_)
    {
        MemberBlueprint& rMember = this->members[structMemberId_];
        if(rMember.kind != STRUCT_MEMBER)
        {
            throw std::runtime_error(std::format("Descriptor Layout {}: Member {} is a variable, only a struct member can get variables with addVariable.", this->name, rMember.name));
        }
        return rMember;
    }

    MemberBlueprint& DescriptorLayout::getMember(PoolId memberId_)
    {
        return this->members[memberId_];
    }

    const VariableRef& DescriptorLayout::getVariable(PoolId variableId_)
    {
        return this->variables[variableId_];
    }

    PoolId DescriptorLayout::addMemberBlueprint(const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_) 
    {
        PoolId memberId = this->addVariableMember(name_);
        return this->registerVariable(memberId, this->members[memberId].layout.addVariableBlueprint(name_, dataType_, repeatCountPerDimension_));
    }

    PoolId DescriptorLayout::addMemberBlueprint(const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_) 
    {
        PoolId memberId = this->addVariableMember(name_);
        return this->registerVariable(memberId, this->members[memberId].layout.addVariableBlueprint(name_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_));
    }

    PoolId DescriptorLayout::addStructMemberBlueprint(const char* name_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        this->throwIfMemoryAllocated(name_);
        if(repeatCountPerDimension_.empty())
        {
            throw std::runtime_error(std::format("Descriptor Layout {}: Struct member {} needs at least one repeat layer, use {{1}} for a single struct.", this->name, name_));
        }
        for(uint32_t count : repeatCountPerDimension_)
        {
            if(count == 0)
            {
                throw std::runtime_error(std::format("Descriptor Layout {}: A repeat layer of {} can't have 0 elements.", this->name, name_));
            }
        }
        STDAlignmentLayoutEnum alignment = (this->bufferType == UNIFORM_BUFFER) ? STD140 : STD430;
        return this->members.add(name_, STRUCT_MEMBER, alignment, repeatCountPerDimension_);
    }

    PoolId DescriptorLayout::addVariable(PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        MemberBlueprint& rMember = this->getStructMember(structMemberId_);
        this->throwIfMemoryAllocated(name_);
        return this->registerVariable(structMemberId_, rMember.layout.addVariableBlueprint(name_, dataType_, repeatCountPerDimension_));
    }

    PoolId DescriptorLayout::addVariable(PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        MemberBlueprint& rMember = this->getStructMember(structMemberId_);
        this->throwIfMemoryAllocated(name_);
        return this->registerVariable(structMemberId_, rMember.layout.addVariableBlueprint(name_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_));
    }

    /*
        The block follows the buffer's rule (std140 / std430): a variable starts at a multiple of its alignment rule, a struct array at a multiple of the alignment of its largest variable (already 16 or more for std140),
        and a struct array takes its element count times the struct size (padded to that alignment). Members are placed in declaration order, which has to be the order of the shader's block.
        The block ends padded to the largest alignment (at least 16 for uniform buffers). A dynamic layer repeats the block with its stride padded, since every dynamic offset has to be a multiple of dynamicAlignment_.
    */
    DescriptorShape DescriptorLayout::makeShape(uint64_t dynamicAlignment_)
    {
        DescriptorShape shape;
        shape.placements.resize(this->members.allocatedSize());
        uint64_t cursor = 0;
        uint64_t blockAlignment = (this->bufferType == UNIFORM_BUFFER) ? 16 : 1;
        for(uint32_t i = 0; i < this->members.allocatedSize(); i++)
        {
            if(!this->members.alive[i])
            {
                continue;
            }

            const MemberBlueprint& rMember = this->members.objects[i];
            if(rMember.layout.size == 0)
            {
                throw std::runtime_error(std::format("Descriptor Layout {}: Member {} has no variables, a struct can't have size 0.", this->name, rMember.name));
            }

            uint64_t alignment;
            uint64_t size;
            if(rMember.kind == STRUCT_MEMBER)
            {
                shape.placements[i].shape = rMember.layout.makeRepeatShape(rMember.repeatCountPerDimension);
                alignment = rMember.layout.structAlignment;
                size = shape.placements[i].shape.getSize();
            }
            else
            {
                const VariableBlueprint& rVariable = rMember.layout.variableBlueprints.objects[0]; // a variable member holds exactly one
                alignment = rVariable.alignmentRule;
                size = rVariable.size;
            }
            shape.placements[i].firstByteId = alignUp(cursor, alignment);
            cursor = shape.placements[i].firstByteId + size;
            blockAlignment = std::max(blockAlignment, alignment);
        }

        shape.blockSize = alignUp(cursor, blockAlignment);
        if(shape.blockSize == 0)
        {
            throw std::runtime_error(std::format("Descriptor Layout {}: Has no members, a buffer can't have size 0.", this->name));
        }
        if(this->dynamic)
        {
            shape.dynamicLayer = {this->dynamicCount, alignUp(shape.blockSize, dynamicAlignment_)};
        }
        return shape;
    }
}
