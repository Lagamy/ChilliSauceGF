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
        DescriptorTypeEnum type; // UNIFORM_DESCRIPTOR or STORAGE_DESCRIPTOR, chosen when the descriptor layout is added.
        bool dynamic = false; // The block is picked per bind (dynamic offset), set when the descriptor layout is added, buffers only (see DescriptorSetLayout::addBufferDynamicDescriptorLayout).
        uint32_t dynamicCount = 0; // Blocks of the dynamic layer, 0 if not dynamic.
        BufferTypeEnum bufferType; // Uniform or storage, decides the std140 / std430 padding of the members.
        Pool<MemberBlueprint> members; // In declaration order, the slot is the member id.
        Pool<VariableRef> variables; // Every variable of every member, the slot is the id the setters take.
        std::vector<PoolId> derivedDescriptors; // Ids of the DescriptorSets derived from it. The descriptor in each of them lives in the same slot as this descriptor layout in its DescriptorSetLayout.

        DescriptorLayout(const char* name_, DescriptorTypeEnum type_, uint32_t dynamicCount_, BufferTypeEnum bufferType_) : name(name_), type(type_), dynamic(dynamicCount_ > 0), dynamicCount(dynamicCount_), bufferType(bufferType_) {};
        VkDescriptorType getVkType() const { return getVkDescriptorType(this->type, this->dynamic); }

        /* Note: use in editor. All of them throw once the memory of a derived set was allocated. */
        /* repeatCountPerDimension_ is the element count per dimension of the variable, outermost first ({2, 3} = a[2][3]), empty = not repeated. */
        PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Scalar variable in the block. Returns the variable id.
        PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
        PoolId addStructMemberBlueprint(const char* name_, const std::vector<uint32_t>& repeatCountPerDimension_); // Returns the member id, the variables of the struct are added with addVariable.
        PoolId addVariable(PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Scalar variable of a struct. Returns the variable id.
        PoolId addVariable(PoolId structMemberId_, const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix

        /*
            Offsets of every member in the block (std140 for uniform, std430 for storage: a member starts at its alignment, a struct at the alignment of its largest variable, and the block ends padded to the largest one, at least 16 for uniform)
            and the dynamic layer, whose stride is the block size padded to dynamicAlignment_. Throws on a block without members or a member without variables.
        */
        DescriptorShape makeShape(uint64_t dynamicAlignment_); // not const: Pool::allocatedSize isn't
        const VariableRef& getVariable(PoolId variableId_);
        MemberBlueprint& getMember(PoolId memberId_);

    private:
        void throwIfMemoryAllocated(const char* memberName_) const;
        PoolId addVariableMember(const char* name_);
        MemberBlueprint& getStructMember(PoolId structMemberId_);
        PoolId registerVariable(PoolId memberId_, PoolId variableId_);
    };
}
