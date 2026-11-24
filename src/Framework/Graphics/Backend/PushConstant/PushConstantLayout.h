#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "CPUDataContainer.h"
#include "DescriptorShape.h"
#include "Layout.h"
#include "MemberBlueprint.h"
#include "Pool.h"
#include "VariableSelector.h"
#include "PoolId.h"
#include "Utilities.h"

namespace Graphics
{
    /*
        A PushConstantLayout attached to a pipeline for some shader stages. The offset is where PushConstantStorage placed the block inside the push constant range of the pipeline layout, it is only filled in PipelineLayout::pushConstantUses once the pipeline layout was created
        (the shader declares the block at that offset: layout(push_constant, offset = N)). stages is passed to vkCmdPushConstants as it is.
    */
    struct PushConstantUse
    {
        PoolId layoutId;
        VkShaderStageFlags stages = 0;
        uint32_t offset = 0;
    };

    /* Which part of a variable a push is about: all of it (a scalar, vector or GLSL matrix type), one element, a row or a column of a MATRIX shaped variable (a grid of elements). */
    enum PushConstantPartEnum
    {
        PUSH_CONSTANT_WHOLE_VARIABLE,
        PUSH_CONSTANT_MATRIX_ELEMENT,
        PUSH_CONSTANT_MATRIX_ROW,
        PUSH_CONSTANT_MATRIX_COLUMN
    };

    struct PushConstantWrite;

    /*
        Writes the value pWrite.pData points at into rContainer_ the way the layout stores it (ReflectionLayout::setMemberScalar / setElementInMemberMatrix / setRowInMemberMatrix / setColumnInMemberMatrix for the T the command was added with). A plain function pointer so a command can keep it without knowing T.
    */
    using PushConstantWriteFunc = void(*)(const PushConstantWrite& rWrite_, ReflectionLayout& rLayout_, CPUDataContainer& rContainer_);

    /*
        Everything needed to write one push: the variable (an id or a position, see VariableSelector; ids as in the setters of a descriptor: one id per repeat layer of its struct, none for a variable directly in the block, one per dimension of the variable), the part of it (rowId / columnId for an element, rowId for a row,
        columnId for a column, srcId is the row / column of a source Matrix) and the source, an Array, Matrix or value of the type the variable has, behind pData.
    */
    struct PushConstantWrite
    {
        VariableSelector variable; // looked up in the layout every time the bytes are made, so a position follows layout updates
        std::vector<uint32_t> idsPerLayer;
        std::vector<uint32_t> repeatIdPerDimension;
        PushConstantPartEnum part = PUSH_CONSTANT_WHOLE_VARIABLE;
        uint32_t rowId = 0;
        uint32_t columnId = 0;
        uint32_t srcId = 0;
        const void* pData = nullptr;
        PushConstantWriteFunc pWrite = nullptr;
        PoolId variableId = UninitializedPoolId; // the id of the variable in its member's layout, filled by PushConstantLayout::makeBytes right before pWrite is called
    };

    /* A run of bytes of the block and where it starts. */
    struct PushConstantBytes
    {
        uint64_t inBlockOffset = 0;
        std::vector<char> bytes;
    };

    /*
        The block of a push constant range the shader sees, e.g. layout(push_constant) uniform Constants { mat4 model; vec4 tint; }. Built like a DescriptorLayout (members, struct members, variables) without the binding, type, dynamic layer and headroom: it is
        one block, always std430 (the GLSL default for push constants). Where the block sits in the push constant range of a pipeline is decided when the pipeline layout is created (PushConstantStorage), the values are pushed by a CommandList command.
    */
    struct PushConstantLayout
    {
        std::string name; // for debug porpuses
        Pool<MemberBlueprint> members; // In declaration order, the slot is the member id.
        Pool<VariableRef> variables; // Every variable of every member, the slot is the id the commands take.
        bool updateOpen = false; // a PushConstantLayoutUpdate of it wasn't committed yet, only one at a time

        PushConstantLayout(const char* name_) : name(name_) {};

        /* repeatCountPerDimension_ is the element count per dimension of the variable, outermost first ({2, 3} = a[2][3]), empty = not repeated. */
        /* orderPosition_ is the place in the declaration order of the shader's block (can be sparse), unique among the members, throws otherwise. For a variable of a struct it is the place inside the struct, unique there. */
        PoolId addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Scalar variable in the block. Returns the variable id.
        PoolId addMemberBlueprint(const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
        PoolId addStructMemberBlueprint(const char* name_, uint32_t orderPosition_, const std::vector<uint32_t>& repeatCountPerDimension_, uint32_t headroom_ = 0); // Returns the member id, the variables of the struct are added with addVariable. headroom_: the struct is never smaller than this many bytes (the stride of its elements).
        PoolId addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Scalar variable of a struct. Returns the variable id.
        PoolId addVariable(const char* name_, PoolId structMemberId_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
        void removeMemberBlueprint(PoolId memberId_); // a whole member and its variables
        void removeVariableFromStruct(PoolId variableId_); // one variable of a struct member, the variables after it move up. A variable directly in the block goes with removeMemberBlueprint.

        /*
            Offsets of every member in the block, placed in the order of their orderPosition with the std430 rules, same as DescriptorLayout::makeShape (the block ends padded to the largest alignment). Throws on a block without members or a member without variables.
        */
        DescriptorShape makeShape(); // not const: Pool::allocatedSize isn't
        uint32_t getAlignment(); // the largest alignment of the members, at least 4: the block starts at a multiple of it inside the push constant range
        const VariableRef& getVariable(const VariableSelector& rVariable_); // by id or by position, see VariableSelector
        MemberBlueprint& getMember(PoolId memberId_);

        /*
            Writes the source into a block sized scratch container through rWrite_.pWrite (so the padding of matrices is applied by the same code as descriptors) and returns the bytes it touched: one run for a variable, an element or a row, one run per element for a column (the elements of a column are apart, the bytes between
            them belong to other elements and must not be pushed). Throws on a wrong type, shape or ids.
            Called when a command is added (to throw there, and to keep the bytes of a value passed by value) and every time a command with a pointer is recorded (to read the current value).
        */
        std::vector<PushConstantBytes> makeBytes(const PushConstantWrite& rWrite_);

    private:
        PoolId addVariableMember(const char* name_, uint32_t orderPosition_);
        void throwIfMemberPositionUsed(const char* name_, uint32_t orderPosition_);
        MemberBlueprint& getStructMember(PoolId structMemberId_);
        PoolId registerVariable(PoolId memberId_, PoolId variableId_);
    };
}
