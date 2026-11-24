#pragma once 
#include <string>
#include <vector> 
#include "DescriptorShape.h"
#include "MemberBlueprint.h"
#include "Pool.h"
#include "PoolId.h"
#include "Utilities.h"

namespace Graphics
{
    /*
        One binding of a DescriptorSetLayout: the block the shader sees, e.g. 
        buffer Objects { Object states[2]; mat4 model; }
        A block is a list of members, each either a variable (addMemberBlueprint) or a struct of variables repeated repeatCountPerDimension times (addStructMemberBlueprint + addVariable). 
        The dynamic layer wraps the whole block: dynamicCount blocks, one of them picked per bind.
    */
    struct DescriptorLayout 
    {
        std::string name; // for debug porpuses
        uint32_t binding; // The binding number the shader uses (layout(set = N, binding = binding)), chosen by the user when the descriptor layout is added, unique inside its DescriptorSetLayout. The pool slot is only the internal id.
        DescriptorTypeEnum type; // UNIFORM_DESCRIPTOR or STORAGE_DESCRIPTOR, chosen when the descriptor layout is added.
        bool dynamic = false; // The block is picked per bind (dynamic offset), set when the descriptor layout is added, buffers only (see DescriptorSetLayout::addDynamicDescriptorLayout).
        uint32_t dynamicCount = 0; // Blocks of the dynamic layer, 0 if not dynamic.
        BufferTypeEnum bufferType; // Uniform or storage, decides the std140 / std430 padding of the members.
        uint32_t headroom = 0; // The block (one unit) is never smaller than this many bytes, so members added later that fit in it don't change the size of the entry. 0 = exactly what the members take.
        Pool<MemberBlueprint> members; // In declaration order, the slot is the member id.
        Pool<VariableRef> variables; // Every variable of every member, the slot is the id the setters take.
        bool pending = false; // added by a SetLayoutUpdate that wasn't committed yet: the DescriptorSetLayout (bindings, pools) and derived sets ignore it until then
        bool updateOpen = false; // a DescriptorLayoutUpdate of it wasn't committed yet, only one at a time
        std::vector<PoolId> derivedDescriptors; // Ids of the DescriptorSets derived from it. The descriptor in each of them lives in the same slot as this descriptor layout in its DescriptorSetLayout.

        DescriptorLayout(const char* name_, uint32_t binding_, DescriptorTypeEnum type_, uint32_t dynamicCount_, BufferTypeEnum bufferType_, uint32_t headroom_ = 0) : name(name_), binding(binding_), type(type_), dynamic(dynamicCount_ > 0), dynamicCount(dynamicCount_), bufferType(bufferType_), headroom(headroom_) {};
        VkDescriptorType getVkType() const { return getVkDescriptorType(this->type, this->dynamic); }

        /* Edited through a DescriptorLayoutUpdate (on its staged copy), which writes the result back with commitDescriptorLayoutUpdate. */
        /* repeatCountPerDimension_ is the element count per dimension of the variable, outermost first ({2, 3} = a[2][3]), empty = not repeated. */
        /* orderPosition_ is the place in the declaration order of the shader's block (can be sparse), unique among the members, throws otherwise. For a variable of a struct it is the place inside the struct, unique there. */
        PoolId addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Scalar variable in the block. Returns the variable id.
        PoolId addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
        PoolId addStructMemberBlueprint(const char* name_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_ = 0); // Returns the member id, the variables of the struct are added with addVariable. headroom_: the struct is never smaller than this many bytes, so variables added later that fit in it don't change its size (the stride of its elements).
        PoolId addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Scalar variable of a struct. Returns the variable id.
        PoolId addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
        void removeMemberBlueprint(PoolId memberId_); // a whole member and its variables
        void removeVariableFromStruct(PoolId variableId_); // one variable of a struct member, the variables after it move up. A variable directly in the block goes with removeMemberBlueprint.

        /*
            Offsets of every member in the block, placed in the order of their orderPosition (std140 for uniform, std430 for storage: a member starts at its alignment, a struct at the alignment of its largest variable, and the block ends padded to the largest one, at least 16 for uniform)
            and the dynamic layer, whose stride is the block size padded to dynamicAlignment_. Throws on a block without members or a member without variables.
        */
        DescriptorShape makeShape(uint64_t dynamicAlignment_); // not const: Pool::allocatedSize isn't
        const VariableRef& getVariable(const VariableSelector& rVariable_); // by id or by position, see VariableSelector
        MemberBlueprint& getMember(PoolId memberId_);

    private:
        PoolId addVariableMember(const char* name_, uint32_t orderPosition_);
        void throwIfMemberPositionUsed(const char* name_, uint32_t orderPosition_);
        MemberBlueprint& getStructMember(PoolId structMemberId_);
        PoolId registerVariable(PoolId memberId_, PoolId variableId_);
    };
}
