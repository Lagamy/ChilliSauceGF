#include "PushConstantLayout.h"
#include "Api.h"
#include <algorithm>
#include <format>
#include <stdexcept>

namespace Graphics
{
    void PushConstantLayout::throwIfMemberPositionUsed(const char* name_, uint32_t orderPosition_)
    {
        for(uint32_t i = 0; i < this->members.allocatedSize(); i++)
        {
            if(this->members.alive[i] && this->members.objects[i].orderPosition == orderPosition_)
            {
                throw std::runtime_error(std::format("Push Constant Layout {}: Order position {} of member {} is already used by {}.", this->name, orderPosition_, name_, this->members.objects[i].name));
            }
        }
    }

    PoolId PushConstantLayout::addVariableMember(const char* name_, uint32_t orderPosition_)
    {
        this->throwIfMemberPositionUsed(name_, orderPosition_);
        return this->members.add(name_, VARIABLE_MEMBER, STD430, orderPosition_, std::vector<uint32_t>{});
    }

    PoolId PushConstantLayout::registerVariable(PoolId memberId_, PoolId variableId_)
    {
        return this->variables.add(memberId_, variableId_);
    }

    MemberBlueprint& PushConstantLayout::getStructMember(PoolId structMemberId_)
    {
        MemberBlueprint& rMember = this->members[structMemberId_];
        if(rMember.kind != STRUCT_MEMBER)
        {
            throw std::runtime_error(std::format("Push Constant Layout {}: Member {} is a variable, only a struct member can get variables with addVariable.", this->name, rMember.name));
        }
        return rMember;
    }

    void PushConstantLayout::removeMemberBlueprint(PoolId memberId_)
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

    void PushConstantLayout::removeVariableFromStruct(PoolId variableId_)
    {
        const VariableRef& rRef = this->variables[variableId_];
        MemberBlueprint& rMember = this->getStructMember(rRef.memberId);
        rMember.layout.removeVariableBlueprint(rRef.variableId);
        this->variables.remove(variableId_);
    }

    MemberBlueprint& PushConstantLayout::getMember(PoolId memberId_)
    {
        return this->members[memberId_];
    }

    const VariableRef& PushConstantLayout::getVariable(const VariableSelector& rVariable_)
    {
        return resolveVariable(this->members, this->variables, rVariable_, this->name.c_str());
    }

    PoolId PushConstantLayout::addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        PoolId memberId = this->addVariableMember(name_, orderPosition_);
        return this->registerVariable(memberId, this->members[memberId].layout.addVariableBlueprint(name_, dataType_, repeatCountPerDimension_));
    }

    PoolId PushConstantLayout::addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        PoolId memberId = this->addVariableMember(name_, orderPosition_);
        return this->registerVariable(memberId, this->members[memberId].layout.addVariableBlueprint(name_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_));
    }

    PoolId PushConstantLayout::addStructMemberBlueprint(const char* name_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_)
    {
        this->throwIfMemberPositionUsed(name_, orderPosition_);
        if(repeatCountPerDimension_.empty())
        {
            throw std::runtime_error(std::format("Push Constant Layout {}: Struct member {} needs at least one repeat layer, use {{1}} for a single struct.", this->name, name_));
        }
        for(uint32_t count : repeatCountPerDimension_)
        {
            if(count == 0)
            {
                throw std::runtime_error(std::format("Push Constant Layout {}: A repeat layer of {} can't have 0 elements.", this->name, name_));
            }
        }
        return this->members.add(name_, STRUCT_MEMBER, STD430, orderPosition_, repeatCountPerDimension_, headroom_);
    }

    PoolId PushConstantLayout::addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        MemberBlueprint& rMember = this->getStructMember(structMemberId_);
        return this->registerVariable(structMemberId_, rMember.layout.addVariableBlueprint(name_, orderPosition_, dataType_, repeatCountPerDimension_));
    }

    PoolId PushConstantLayout::addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        MemberBlueprint& rMember = this->getStructMember(structMemberId_);
        return this->registerVariable(structMemberId_, rMember.layout.addVariableBlueprint(name_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_));
    }

    /*
        The std430 twin of DescriptorLayout::makeShape without the dynamic layer: a variable starts at a multiple of its alignment rule, a struct array at the alignment of its largest variable, members in the order of their orderPosition (which has to be the order of the shader's block),
        and the block ends padded to the largest alignment. The block is never smaller than 4 bytes of alignment, since push constant offsets and sizes have to be multiples of 4.
    */
    DescriptorShape PushConstantLayout::makeShape()
    {
        DescriptorShape shape;
        shape.placements.resize(this->members.allocatedSize());
        uint64_t cursor = 0;
        uint64_t blockAlignment = 4;
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
                throw std::runtime_error(std::format("Push Constant Layout {}: Member {} has no variables, a struct can't have size 0.", this->name, rMember.name));
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
        if(cursor == 0)
        {
            throw std::runtime_error(std::format("Push Constant Layout {}: Has no members, a push constant block can't have size 0.", this->name));
        }
        return shape;
    }

    uint32_t PushConstantLayout::getAlignment()
    {
        uint32_t alignment = 4;
        for(uint32_t i = 0; i < this->members.allocatedSize(); i++)
        {
            if(!this->members.alive[i])
            {
                continue;
            }
            const MemberBlueprint& rMember = this->members.objects[i];
            if(rMember.kind == STRUCT_MEMBER)
            {
                alignment = std::max<uint32_t>(alignment, rMember.layout.structAlignment);
            }
            else if(!rMember.layout.variableBlueprints.nameToId.empty())
            {
                alignment = std::max<uint32_t>(alignment, rMember.layout.variableBlueprints.objects[0].alignmentRule);
            }
        }
        return alignment;
    }

    /*
        The offset of the variable is the member's place in the block plus the struct element picked by idsPerLayer_ (one id per repeat layer of its struct, none for a variable directly in the block, same as DescriptorSet::writeMemberRange), plus the variable's place in its layout and its own repeat instance.
        The layout writes at unit 0 of the container, the element's offset travels in repeatBaseOffset.
    */
    std::vector<PushConstantBytes> PushConstantLayout::makeBytes(const PushConstantWrite& rWrite_)
    {
        if(rWrite_.pData == nullptr)
        {
            throw std::runtime_error(std::format("Push Constant Layout {}: the pointer to the value is null.", this->name));
        }
        DescriptorShape shape = this->makeShape();
        const VariableRef& rVariableRef = this->getVariable(rWrite_.variable);
        ReflectionLayout& rLayout = this->getMember(rVariableRef.memberId).layout;
        const VariableBlueprint& rVariable = rLayout.variableBlueprints[rVariableRef.variableId];
        const MemberPlacement& rPlacement = shape.placements[rVariableRef.memberId.id];

        uint64_t repeatOffset = rPlacement.firstByteId + rPlacement.shape.getOffset(rWrite_.idsPerLayer);
        CPUDataContainer container;
        container.data.resize(shape.blockSize);
        container.repeatCount = 1;
        container.repeatStride = rLayout.size;
        container.repeatBaseOffset = repeatOffset;
        PushConstantWrite write = rWrite_;
        write.variableId = rVariableRef.variableId;
        write.pWrite(write, rLayout, container);

        uint64_t variableOffset = repeatOffset + rVariable.firstByteId + rVariable.getRepeatOffset(rWrite_.repeatIdPerDimension);
        std::vector<std::pair<uint64_t, uint64_t>> ranges; // offset in the block, size
        switch(rWrite_.part)
        {
            case PUSH_CONSTANT_WHOLE_VARIABLE:
            {
                ranges.push_back({variableOffset, rVariable.elementSize});
                break;
            }
            case PUSH_CONSTANT_MATRIX_ELEMENT:
            {
                ranges.push_back({variableOffset + rVariable.getMatrixElementOffset(rWrite_.rowId, rWrite_.columnId), rVariable.getElementStride()});
                break;
            }
            case PUSH_CONSTANT_MATRIX_ROW:
            {
                ranges.push_back({variableOffset + rVariable.getMatrixElementOffset(rWrite_.rowId, 0), rVariable.getRowStride()});
                break;
            }
            case PUSH_CONSTANT_MATRIX_COLUMN:
            {
                for(uint32_t rowId = 0; rowId < rVariable.shapeMetadata.rowCount; rowId++)
                {
                    ranges.push_back({variableOffset + rVariable.getMatrixElementOffset(rowId, rWrite_.columnId), rVariable.getElementStride()});
                }
                break;
            }
        }

        std::vector<PushConstantBytes> result;
        for(const std::pair<uint64_t, uint64_t>& rRange : ranges)
        {
            PushConstantBytes bytes;
            bytes.inBlockOffset = rRange.first;
            bytes.bytes.assign(container.data.begin() + rRange.first, container.data.begin() + rRange.first + rRange.second);
            result.push_back(std::move(bytes));
        }
        return result;
    }
}
