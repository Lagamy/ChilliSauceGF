#pragma once
#include "PoolMap.h"
#include "DataContainerConcepts.h"
#include "VariableBlueprint.h"
#include "Utilities.h"
#include "Array.h"
#include "Matrix.h"
#include "RepeatShape.h"
#include <cstring>
#include <span>
#include <vector>
#include <stdexcept>
#include <format>

namespace Graphics {
struct ReflectionLayout
{
    STDAlignmentLayoutEnum alignment = NONE;
    PoolMap<VariableBlueprint> variableBlueprints = ("Member Blueprints");
    size_t size = 0; // Includes the padding at the end, see updateSize
    uint8_t structAlignment = 1;

    ReflectionLayout(STDAlignmentLayoutEnum alignment_ = NONE) : alignment(alignment_) {};

    /* repeatCountPerDimension_: element count per dimension, outermost first ({2, 3} = a[2][3]), empty = not repeated. */
    PoolId addVariableBlueprint(const char* name_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Scalar
    PoolId addVariableBlueprint(const char* name_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_ = {}); // Matrix
    PoolId getMemberIdFromName(const char* name_);

    /*
        Shared checks of every set/get below: the member has to be of type T's GPU type and of the requested shape, and the repeat unit has to exist.
        Returns the member.
    */
    VariableBlueprint& getCheckedMember(PoolId variableBlueprintId_, DataTypeEnum dataType_, DataShapeEnum shape_, uint32_t repeatUnitId_, uint32_t repeatCount_);

    /* The shape of a struct repeated countPerDimension_ times per layer (outermost first) of this layout, see RepeatShape::fromCounts. */
    RepeatShape makeRepeatShape(std::span<const uint32_t> repeatCountPerDimension_) const
    {
        return RepeatShape::fromCounts(repeatCountPerDimension_, this->size);
    }

    /* Offset of repeat unit repeatUnitId_ out of repeatCount_ in the one-layer case of computeRepeatOffset (the same routine every shape uses). */
    size_t getRepeatUnitOffset(uint32_t repeatCount_, uint32_t repeatUnitId_, uint64_t repeatStride_, uint64_t repeatBaseOffset_ = 0) const
    {
        RepeatLayer layer = {repeatCount_, repeatStride_ == 0 ? this->size : repeatStride_};
        return repeatBaseOffset_ + computeRepeatOffset(std::span<const RepeatLayer>(&layer, 1), std::span<const uint32_t>(&repeatUnitId_, 1));
    }

    /*
        Every set / get below takes repeatIdPerDimension_, one id per dimension of the variable (outermost first, empty for a variable that isn't repeated), on top of repeatUnitId_, which picks the repeat unit of the container.
        A variable of dimensions {2, 3} (a[2][3]) is written as a[repeatIdPerDimension_[0]][repeatIdPerDimension_[1]].
    */

    /*
        Writes one value of type T at firstByte. A matrix type is stored column by column and every column starts at a multiple of the member's
        alignment rule (std140 pads a mat3 column to 16 bytes), while the C++ matrix is packed. Scalars and vectors are one block.
    */
    template<typename T, WritableDataContainer C>
    void writeTyped(C& rContainer_, const VariableBlueprint& rVariableBlueprint_, size_t firstByte_, const T& rData_)
    {
        uint32_t rowCount = rVariableBlueprint_.typeMetadata.rowCount;
        if(rowCount > 1)
        {
            size_t columnBytes = rowCount * 4;
            size_t columnStride = rVariableBlueprint_.getColumnStride();
            for(uint32_t columnId = 0; columnId < rVariableBlueprint_.typeMetadata.elementCount; columnId++)
            {
                rContainer_.write(firstByte_ + columnId * columnStride, reinterpret_cast<const char*>(&rData_) + columnId * columnBytes, columnBytes);
            }
            return;
        }
        rContainer_.write(firstByte_, &rData_, sizeof(T));
    };

    /* Same column layout as writeTyped. */
    template<typename T, ReadableDataContainer C>
    T readTyped(const C& rContainer_, const VariableBlueprint& rVariableBlueprint_, size_t firstByte_)
    {
        T data;
        uint32_t rowCount = rVariableBlueprint_.typeMetadata.rowCount;
        if(rowCount > 1)
        {
            size_t columnBytes = rowCount * 4;
            size_t columnStride = rVariableBlueprint_.getColumnStride();
            for(uint32_t columnId = 0; columnId < rVariableBlueprint_.typeMetadata.elementCount; columnId++)
            {
                rContainer_.read(firstByte_ + columnId * columnStride, reinterpret_cast<char*>(&data) + columnId * columnBytes, columnBytes);
            }
            return data;
        }
        rContainer_.read(firstByte_, &data, sizeof(T));
        return data;
    };

    template<typename T, WritableDataContainer C>
    void setMemberScalar(C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, T data_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, SCALAR, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        this->writeTyped(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_), data_);
    };

    template<typename T, ReadableDataContainer C>
    const T getMemberScalar(const C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, SCALAR, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        return this->readTyped<T>(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_));
    };

    template<typename T, WritableDataContainer C>
    void setElementInMemberMatrix(C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, uint32_t columnId_, T data_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        this->writeTyped(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_) + rVariableBlueprint.getMatrixElementOffset(rowId_, columnId_), data_);
    };

    template<typename T, ReadableDataContainer C>
    const T getElementInMemberMatrix(const C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, uint32_t columnId_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        return this->readTyped<T>(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_) + rVariableBlueprint.getMatrixElementOffset(rowId_, columnId_));
    };

    /* Replaces a whole row. The Array must have one element per column of the member. */
    template<typename T, WritableDataContainer C>
    void setRowInMemberMatrix(C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, const Array<T>& rSrcArray_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcArray_.size() != rVariableBlueprint.shapeMetadata.elementCount)
        {
            throw std::runtime_error(std::format("Layout: Array with {} elements can't replace a row of {} elements in Member {}.",
            rSrcArray_.size(), rVariableBlueprint.shapeMetadata.elementCount, this->variableBlueprints.getName(variableBlueprintId_)));
        }
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        for(uint32_t columnId = 0; columnId < rSrcArray_.size(); columnId++)
        {
            T element = rSrcArray_.getElement(columnId);
            this->writeTyped(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_) + rVariableBlueprint.getMatrixElementOffset(rowId_, columnId), element);
        }
    };

    /* Replaces a whole column. The Array must have one element per row of the member. */
    template<typename T, WritableDataContainer C>
    void setColumnInMemberMatrix(C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, uint32_t columnId_, const Array<T>& rSrcArray_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcArray_.size() != rVariableBlueprint.shapeMetadata.rowCount)
        {
            throw std::runtime_error(std::format("Layout: Array with {} elements can't replace a column of {} elements in Member {}.",
            rSrcArray_.size(), rVariableBlueprint.shapeMetadata.rowCount, this->variableBlueprints.getName(variableBlueprintId_)));
        }
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        for(uint32_t rowId = 0; rowId < rSrcArray_.size(); rowId++)
        {
            T element = rSrcArray_.getElement(rowId);
            this->writeTyped(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_) + rVariableBlueprint.getMatrixElementOffset(rowId, columnId_), element);
        }
    };

    /* Replaces a whole row with row srcRowId_ of a Matrix. The Matrix must have as many columns as the member. */
    template<typename T, WritableDataContainer C>
    void setRowInMemberMatrix(C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcMatrix_.getColumnCount() != rVariableBlueprint.shapeMetadata.elementCount || srcRowId_ >= rSrcMatrix_.getRowCount())
        {
            throw std::runtime_error(std::format("Layout: Row {} of a {}x{} Matrix can't replace a row of {} elements in Member {}.",
            srcRowId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rVariableBlueprint.shapeMetadata.elementCount, this->variableBlueprints.getName(variableBlueprintId_)));
        }
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        for(uint32_t columnId = 0; columnId < rSrcMatrix_.getColumnCount(); columnId++)
        {
            T element = rSrcMatrix_.getElement(srcRowId_, columnId);
            this->writeTyped(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_) + rVariableBlueprint.getMatrixElementOffset(rowId_, columnId), element);
        }
    };

    /* Replaces a whole column with column srcColumnId_ of a Matrix. The Matrix must have as many rows as the member. */
    template<typename T, WritableDataContainer C>
    void setColumnInMemberMatrix(C& rContainer_, PoolId variableBlueprintId_, uint32_t repeatUnitId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_, std::span<const uint32_t> repeatIdPerDimension_ = {})
    {
        VariableBlueprint& rVariableBlueprint = this->getCheckedMember(variableBlueprintId_, GPUTypeMap<T>::value, MATRIX, repeatUnitId_, rContainer_.repeatCount);
        if(rSrcMatrix_.getRowCount() != rVariableBlueprint.shapeMetadata.rowCount || srcColumnId_ >= rSrcMatrix_.getColumnCount())
        {
            throw std::runtime_error(std::format("Layout: Column {} of a {}x{} Matrix can't replace a column of {} elements in Member {}.",
            srcColumnId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rVariableBlueprint.shapeMetadata.rowCount, this->variableBlueprints.getName(variableBlueprintId_)));
        }
        size_t repeatUnitOffset = this->getRepeatUnitOffset(rContainer_.repeatCount, repeatUnitId_, rContainer_.repeatStride, rContainer_.repeatBaseOffset);
        for(uint32_t rowId = 0; rowId < rSrcMatrix_.getRowCount(); rowId++)
        {
            T element = rSrcMatrix_.getElement(rowId, srcColumnId_);
            this->writeTyped(rContainer_, rVariableBlueprint, repeatUnitOffset + rVariableBlueprint.firstByteId + rVariableBlueprint.getRepeatOffset(repeatIdPerDimension_) + rVariableBlueprint.getMatrixElementOffset(rowId, columnId_), element);
        }
    };

private:
    uint32_t getFirstAvailableByte();
    void updateSize();
};
}