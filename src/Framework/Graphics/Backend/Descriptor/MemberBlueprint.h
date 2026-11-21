#pragma once 
#include <string>
#include <vector> 
#include "Layout.h"
#include "Pool.h"
#include "VariableSelector.h"
#include "Utilities.h"

namespace Graphics
{
    enum MemberKindEnum : uint8_t
    {
        VARIABLE_MEMBER = 0, // one scalar / matrix (optionally repeated per dimension) directly in the block (mat4 model;)
        STRUCT_MEMBER = 1 // a struct of variables, repeated repeatCountPerDimension times (Object states[2];)
    };

    /* One member of the block of a DescriptorLayout, in the order the shader declares them. The offsets in the block are worked out in DescriptorLayout::makeShape. */
    struct MemberBlueprint 
    {
        std::string name; // for debug porpuses
        MemberKindEnum kind;
        uint32_t orderPosition; // Declaration order in the block, unique there (can be sparse). makeShape places the members in this order, not in the order of the slots.
        ReflectionLayout layout; // VARIABLE_MEMBER: exactly one variable. STRUCT_MEMBER: the variables of the struct.
        std::vector<uint32_t> repeatCountPerDimension; // STRUCT_MEMBER only: elements per layer of the struct array, outermost first ({2} = states[2], {1} = a single struct). Empty for a variable.

        MemberBlueprint(const char* name_, MemberKindEnum kind_, STDAlignmentLayoutEnum alignment_, uint32_t orderPosition_, std::vector<uint32_t> repeatCountPerDimension_, uint32_t headroom_ = 0) : name(name_), kind(kind_), orderPosition(orderPosition_), layout(alignment_, headroom_), repeatCountPerDimension(std::move(repeatCountPerDimension_)) {};
    };

    /* A variable of the descriptor: the member that holds it and its id in that member's layout. The id the user keeps (and the setters take) is the slot of this in DescriptorLayout::variables. */
    struct VariableRef
    {
        PoolId memberId;
        PoolId variableId;

        VariableRef(PoolId memberId_, PoolId variableId_) : memberId(memberId_), variableId(variableId_) {};
    };

    /*
        The variable a selector picks in the members and variables of a DescriptorLayout or PushConstantLayout (both keep them the same way): an id is the slot of rVariables_, a position is looked up by the orderPosition of the member (a variable directly in the block, SELECT_BY_POSITION)
        or of the struct member and of the variable inside it (SELECT_BY_STRUCT_POSITION). Throws, naming rLayoutName_, when there is no such variable.
    */
    const VariableRef& resolveVariable(Pool<MemberBlueprint>& rMembers_, Pool<VariableRef>& rVariables_, const VariableSelector& rSelector_, const char* layoutName_);
}
