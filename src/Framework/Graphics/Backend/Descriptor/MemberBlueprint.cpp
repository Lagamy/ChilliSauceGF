#include "MemberBlueprint.h"
#include <format>
#include <stdexcept>
#include "Utilities.h"

namespace Graphics
{
    const VariableRef& resolveVariable(Pool<MemberBlueprint>& rMembers_, Pool<VariableRef>& rVariables_, const VariableSelector& rSelector_, const char* layoutName_)
    {
        if(rSelector_.kind == SELECT_BY_ID)
        {
            return rVariables_[rSelector_.id];
        }

        bool inStruct = rSelector_.kind == SELECT_BY_STRUCT_POSITION;
        MemberKindEnum wantedKind = inStruct ? STRUCT_MEMBER : VARIABLE_MEMBER;
        uint32_t memberPosition = inStruct ? rSelector_.structPosition : rSelector_.position;
        for(uint32_t memberSlot = 0; memberSlot < rMembers_.allocatedSize(); memberSlot++)
        {
            if(!rMembers_.alive[memberSlot] || rMembers_.getInternal(memberSlot).kind != wantedKind || rMembers_.getInternal(memberSlot).orderPosition != memberPosition)
            {
                continue;
            }

            PoolId memberId = PoolId(memberSlot, rMembers_.generation[memberSlot]);
            PoolId inLayoutId = inStruct ? rMembers_.getInternal(memberSlot).layout.getVariableId(VariableSelector(rSelector_.position)) : UninitializedPoolId;
            for(uint32_t variableSlot = 0; variableSlot < rVariables_.allocatedSize(); variableSlot++)
            {
                if(rVariables_.alive[variableSlot] && rVariables_.getInternal(variableSlot).memberId == memberId && (!inStruct || rVariables_.getInternal(variableSlot).variableId == inLayoutId))
                {
                    return rVariables_.getInternal(variableSlot);
                }
            }
        }
        if(inStruct)
        {
            throw std::runtime_error(std::format("{}: no variable at position {} in the struct at position {}.", layoutName_, rSelector_.position, rSelector_.structPosition));
        }
        throw std::runtime_error(std::format("{}: no variable directly in the block at position {}.", layoutName_, rSelector_.position));
    }
}
