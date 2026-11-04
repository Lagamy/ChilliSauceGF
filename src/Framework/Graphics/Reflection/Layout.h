#pragma once
#include "PoolMap.h"
#include "DataContainerConcepts.h"
#include "MemberBlueprint.h"
#include "Utilities.h"
#include "Array.h"
#include "Matrix.h"
#include <cstring>
#include <stdexcept>
#include <format>

namespace Graphics {
struct ReflectionLayout
{
    STDAlignmentLayoutEnum alignment = NONE;
    PoolMap<MemberBlueprint> memberBlueprints = ("Member Blueprints");
    size_t size = 0; // Includes the padding at the end, see updateSize
    uint8_t structAlignment = 1;

    ReflectionLayout(STDAlignmentLayoutEnum alignment_ = NONE) : alignment(alignment_) {};

    PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_); // Scalar
    PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_, uint32_t elementCount_); // Array
    PoolId addMemberBlueprint(const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_); // Matrix
    PoolId getMemberIdFromName(const char* name_);

    /*
        Shared checks of every set/get below: the member has to be of type T's GPU type and of the requested shape, and the repeat unit has to exist.
        Returns the member.
    */
    MemberBlueprint& getCheckedMember(PoolId memberBlueprintId_, DataTypeEnum dataType_, DataShapeEnum shape_, uint32_t repeatUnitId_, uint32_t repeatCount_);

    /*
        Writes one value of type T at firstByte. A matrix type is stored column by column and every column starts at a multiple of the member's
        alignment rule (std140 pads a mat3 column to 16 bytes), while the C++ matrix is packed. Scalars and vectors are one block.
    */
    template<typename T, WritableDataContainer C>
    void writeTyped(C& rContainer_, const MemberBlueprint& rMemberBlueprint_, size_t firstByte_, const T& rData_)
    {
        uint32_t rowCount = rMemberBlueprint_.typeMetadata.rowCount;
        if(rowCount > 1)
        {
            size_t columnBytes = rowCount * 4;
            size_t columnStride = alignUp(columnBytes, rMemberBlueprint_.alignmentRule);
            for(uint32_t columnId = 0; columnId < rMemberBlueprint_.typeMetadata.elementCount; columnId++)
            {
                rContainer_.write(firstByte_ + columnId * columnStride, reinterpret_cast<const char*>(&rData_) + columnId * columnBytes, columnBytes);
            }
            return;
        }
        rContainer_.write(firstByte_, &rData_, sizeof(T));
    };

    /* Same column layout as writeTyped. */
    template<typename T, ReadableDataContainer C>
    T readTyped(const C& rContainer_, const MemberBlueprint& rMemberBlueprint_, size_t firstByte_)
    {
        T data;
        uint32_t rowCount = rMemberBlueprint_.typeMetadata.rowCount;
        if(rowCount > 1)
        {
            size_t columnBytes = rowCount * 4;
            size_t columnStride = alignUp(columnBytes, rMemberBlueprint_.alignmentRule);
            for(uint32_t columnId = 0; columnId < rMemberBlueprint_.typeMetadata.elementCount; columnId++)
            {
                rContainer_.read(firstByte_ + columnId * columnStride, reinterpret_cast<char*>(&data) + columnId * columnBytes, columnBytes);
            }
            return data;
        }
        rContainer_.read(firstByte_, &data, sizeof(T));
        return data;
    };

    template<typename T, WritableDataContainer C>
    void setMemberScalar(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, T data_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, SCALAR, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        this->writeTyped(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId, data_);
    };

    template<typename T, ReadableDataContainer C>
    const T getMemberScalar(const C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, SCALAR, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        return this->readTyped<T>(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId);
    };

    template<typename T, WritableDataContainer C>
    void setElementInMemberArray(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t elementId_, T data_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, ARRAY, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        this->writeTyped(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getArrayElementOffset(elementId_), data_);
    };

    template<typename T, WritableDataContainer C>
    void setElementInMemberMatrix(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, uint32_t columnId_, T data_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        this->writeTyped(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getMatrixElementOffset(rowId_, columnId_), data_);
    };

    template<typename T, ReadableDataContainer C>
    const T getElementInMemberArray(const C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t elementId_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, ARRAY, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        return this->readTyped<T>(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getArrayElementOffset(elementId_));
    };

    template<typename T, ReadableDataContainer C>
    const T getElementInMemberMatrix(const C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, uint32_t columnId_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        return this->readTyped<T>(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getMatrixElementOffset(rowId_, columnId_));
    };

    /* Replaces a whole row. The Array must have one element per column of the member. */
    template<typename T, WritableDataContainer C>
    void setRowInMemberMatrix(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, const Array<T>& rSrcArray_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcArray_.size() != rMemberBlueprint.shapeMetadata.elementCount)
        {
            throw std::runtime_error(std::format("Layout: Array with {} elements can't replace a row of {} elements in Member {}.",
            rSrcArray_.size(), rMemberBlueprint.shapeMetadata.elementCount, this->memberBlueprints.getName(memberBlueprintId_)));
        }
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        for(uint32_t columnId = 0; columnId < rSrcArray_.size(); columnId++)
        {
            T element = rSrcArray_.getElement(columnId);
            this->writeTyped(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getMatrixElementOffset(rowId_, columnId), element);
        }
    };

    /* Replaces a whole column. The Array must have one element per row of the member. */
    template<typename T, WritableDataContainer C>
    void setColumnInMemberMatrix(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t columnId_, const Array<T>& rSrcArray_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcArray_.size() != rMemberBlueprint.shapeMetadata.rowCount)
        {
            throw std::runtime_error(std::format("Layout: Array with {} elements can't replace a column of {} elements in Member {}.",
            rSrcArray_.size(), rMemberBlueprint.shapeMetadata.rowCount, this->memberBlueprints.getName(memberBlueprintId_)));
        }
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        for(uint32_t rowId = 0; rowId < rSrcArray_.size(); rowId++)
        {
            T element = rSrcArray_.getElement(rowId);
            this->writeTyped(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getMatrixElementOffset(rowId, columnId_), element);
        }
    };

    /* Replaces a whole row with row srcRowId_ of a Matrix. The Matrix must have as many columns as the member. */
    template<typename T, WritableDataContainer C>
    void setRowInMemberMatrix(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcMatrix_.getColumnCount() != rMemberBlueprint.shapeMetadata.elementCount || srcRowId_ >= rSrcMatrix_.getRowCount())
        {
            throw std::runtime_error(std::format("Layout: Row {} of a {}x{} Matrix can't replace a row of {} elements in Member {}.",
            srcRowId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rMemberBlueprint.shapeMetadata.elementCount, this->memberBlueprints.getName(memberBlueprintId_)));
        }
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        for(uint32_t columnId = 0; columnId < rSrcMatrix_.getColumnCount(); columnId++)
        {
            T element = rSrcMatrix_.getElement(srcRowId_, columnId);
            this->writeTyped(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getMatrixElementOffset(rowId_, columnId), element);
        }
    };

    /* Replaces a whole column with column srcColumnId_ of a Matrix. The Matrix must have as many rows as the member. */
    template<typename T, WritableDataContainer C>
    void setColumnInMemberMatrix(C& rContainer_, PoolId memberBlueprintId_, uint32_t repeatUnitId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_)
    {
        MemberBlueprint& rMemberBlueprint = this->getCheckedMember(memberBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcMatrix_.getRowCount() != rMemberBlueprint.shapeMetadata.rowCount || srcColumnId_ >= rSrcMatrix_.getColumnCount())
        {
            throw std::runtime_error(std::format("Layout: Column {} of a {}x{} Matrix can't replace a column of {} elements in Member {}.",
            srcColumnId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rMemberBlueprint.shapeMetadata.rowCount, this->memberBlueprints.getName(memberBlueprintId_)));
        }
        size_t repeatUnitOffset = repeatUnitId_ * this->size;
        for(uint32_t rowId = 0; rowId < rSrcMatrix_.getRowCount(); rowId++)
        {
            T element = rSrcMatrix_.getElement(rowId, srcColumnId_);
            this->writeTyped(rContainer_, rMemberBlueprint, repeatUnitOffset + rMemberBlueprint.firstByteId + rMemberBlueprint.getMatrixElementOffset(rowId, columnId_), element);
        }
    };

private:
    uint32_t getFirstAvailableByte();
    void updateSize();
};
}