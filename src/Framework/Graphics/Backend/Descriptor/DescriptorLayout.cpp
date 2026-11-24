#include "DescriptorLayout.h"
#include "Api.h"
#include <algorithm>
#include <format>
#include <stdexcept>

namespace Graphics 
{
    /* A member's variables are padded by the rule of the buffer: uniform blocks follow std140 and storage blocks std430. */
    void DescriptorLayout::throwIfMemberPositionUsed(const char* name_, uint32_t orderPosition_)
    {
        for(uint32_t i = 0; i < this->members.allocatedSize(); i++)
        {
            if(this->members.alive[i] && this->members.objects[i].orderPosition == orderPosition_)
            {
                throw std::runtime_error(std::format("Descriptor Layout {}: Order position {} of member {} is already used by {}.", this->name, orderPosition_, name_, this->members.objects[i].name));
            }
        }
    }

    PoolId DescriptorLayout::addVariableMember(const char* name_, uint32_t orderPosition_)
    {
        this->throwIfMemberPositionUsed(name_, orderPosition_);
        STDAlignmentLayoutEnum alignment = (this->bufferType == UNIFORM_BUFFER) ? STD140 : STD430;
        return this->members.add(name_, VARIABLE_MEMBER, alignment, orderPosition_, std::vector<uint32_t>{});
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

    /* The member and the variable ids of its variables go. The offsets of the other members come from makeShape, so nothing else has to move. */
    void DescriptorLayout::removeMemberBlueprint(PoolId memberId_)
    {
        this->members.remove(memberId_);
        for(uint32_t i = 0; i < this->variables.allocatedSize(); i++)
        {
            if(this->variables.alive[i] && this->variables.objects[i].memberId.id == memberId_.id)
            {
                this->variables.removeInternal(i);
            }
        }
    }

    /* The member's layout moves the variables after it up, the ids (slot + generation) of the others stay. Its size can change, the offsets in the block come from makeShape. */
    void DescriptorLayout::removeVariableFromStruct(PoolId variableId_)
    {
        const VariableRef& rRef = this->variables[variableId_];
        MemberBlueprint& rMember = this->getStructMember(rRef.memberId);
        rMember.layout.removeVariableBlueprint(rRef.variableId);
        this->variables.remove(variableId_);
    }

    MemberBlueprint& DescriptorLayout::getMember(PoolId memberId_)
    {
        return this->members[memberId_];
    }

    const VariableRef& DescriptorLayout::getVariable(const VariableSelector& rVariable_)
    {
        return resolveVariable(this->members, this->variables, rVariable_, this->name.c_str());
    }

    PoolId DescriptorLayout::addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_) 
    {
        PoolId memberId = this->addVariableMember(name_, orderPosition_);
        return this->registerVariable(memberId, this->members[memberId].layout.addVariableBlueprint(name_, dataType_, repeatCountPerDimension_));
    }

    PoolId DescriptorLayout::addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_) 
    {
        PoolId memberId = this->addVariableMember(name_, orderPosition_);
        return this->registerVariable(memberId, this->members[memberId].layout.addVariableBlueprint(name_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_));
    }

    PoolId DescriptorLayout::addStructMemberBlueprint(const char* name_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_)
    {
        this->throwIfMemberPositionUsed(name_, orderPosition_);
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
        return this->members.add(name_, STRUCT_MEMBER, alignment, orderPosition_, repeatCountPerDimension_, headroom_);
    }

    PoolId DescriptorLayout::addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        MemberBlueprint& rMember = this->getStructMember(structMemberId_);
        return this->registerVariable(structMemberId_, rMember.layout.addVariableBlueprint(name_, orderPosition_, dataType_, repeatCountPerDimension_));
    }

    PoolId DescriptorLayout::addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        MemberBlueprint& rMember = this->getStructMember(structMemberId_);
        return this->registerVariable(structMemberId_, rMember.layout.addVariableBlueprint(name_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_));
    }

    /*
        The block follows the buffer's rule (std140 / std430): a variable starts at a multiple of its alignment rule, a struct array at a multiple of the alignment of its largest variable (already 16 or more for std140),
        and a struct array takes its element count times the struct size (padded to that alignment). Members are placed in the order of their orderPosition, which has to be the order of the shader's block.
        The block ends padded to the largest alignment (at least 16 for uniform buffers). A dynamic layer repeats the block with its stride padded, since every dynamic offset has to be a multiple of dynamicAlignment_.
    */
    DescriptorShape DescriptorLayout::makeShape(uint64_t dynamicAlignment_)
    {
        DescriptorShape shape;
        shape.placements.resize(this->members.allocatedSize());
        uint64_t cursor = 0;
        uint64_t blockAlignment = (this->bufferType == UNIFORM_BUFFER) ? 16 : 1;
        std::vector<uint32_t> order;
        for(uint32_t i = 0; i < this->members.allocatedSize(); i++)
        {
            if(this->members.alive[i])
            {
                order.push_back(i);
            }
        }
        std::sort(order.begin(), order.end(), [this](uint32_t a_, uint32_t b_) { return this->members.objects[a_].orderPosition < this->members.objects[b_].orderPosition; });

        for(uint32_t i : order)
        {
            const MemberBlueprint& rMember = this->members.objects[i];
            if(rMember.layout.variableBlueprints.nameToId.empty())
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

        shape.blockSize = alignUp(std::max<uint64_t>(cursor, this->headroom), blockAlignment);
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
