// Cpu side dynamic row major matrix class. UI and Reflection System will use it. For generic use, and user matrix like 2d containers in GLSL (float[][] for example).
#pragma once
#include "Utilities.h"
#include "Array.h"
#include <vector>
#include <inttypes.h>
#include <string>
#include <stdexcept>
#include <cstring>

namespace Graphics
{
template<typename T>
struct Matrix
{
private:
	std::vector<T> objects;
	uint8_t alignmentRule = 1;
	uint32_t elementStride = 1; // Distance between two elements of a row, in T units (includes padding)
	uint32_t rowStride = 1; // Distance between two rows, in T units (includes padding)
	uint32_t rowCount = 0; // Due to shaders only supporting 32 bit values
	uint32_t columnCount = 0; // Amount of elements in one row
	bool wasInitialized = false;

public:

	bool init(uint32_t rowCount_, uint32_t columnCount_, STDAlignmentLayoutEnum stdAlignmentLayout_)
	{
		if (rowCount_ < 2 || columnCount_ < 2)
		{
			printf("Error: Can't create a Matrix with less than 2 rows or columns\n");
			return false; // Didn't initialize
		}

		/*
			A GLSL float[R][C] is an array of arrays: every element starts at a multiple of the layout's array
			alignment (std140 rounds it up to 16, std430 keeps the element's own alignment), and every inner
			array (a row) starts at a multiple of the same alignment too. The padding is stored as real T
			slots, which keeps the vector byte-identical to the GPU layout.
		*/
		this->alignmentRule = resolveSTDPaddingRule(stdAlignmentLayout_, GPUTypeMap<T>::value, true);
		uint32_t elementStrideBytes = static_cast<uint32_t>(alignUp(sizeof(T), this->alignmentRule));
		uint32_t rowStrideBytes = static_cast<uint32_t>(alignUp(columnCount_ * elementStrideBytes, this->alignmentRule));
		if (elementStrideBytes % sizeof(T) != 0 || rowStrideBytes % sizeof(T) != 0)
		{
			printf("Error: Element stride of %u bytes and row stride of %u bytes must be multiples of the element size.\n", elementStrideBytes, rowStrideBytes);
			return false;
		}

		this->elementStride = elementStrideBytes / sizeof(T);
		this->rowStride = rowStrideBytes / sizeof(T);
		this->objects.resize(rowCount_ * this->rowStride);
		this->rowCount = rowCount_;
		this->columnCount = columnCount_;
		this->wasInitialized = true;
		return true;
	}

	Matrix() = default;
	~Matrix() = default;

	Matrix(uint32_t rowCount_, uint32_t columnCount_, STDAlignmentLayoutEnum stdAlignmentLayout_)
	{
		this->wasInitialized = init(rowCount_, columnCount_, stdAlignmentLayout_);
		if (!this->wasInitialized)
		{
			printf("Warning: Matrix failed to initialize on creation. Initialize it manually in order to use it.\n");
		}
	}

	Matrix(uint32_t rowCount_, uint32_t columnCount_, STDAlignmentLayoutEnum stdAlignmentLayout_, std::initializer_list<T> list_)
	{
		this->wasInitialized = init(rowCount_, columnCount_, stdAlignmentLayout_);
		if (!this->wasInitialized)
		{
			printf("Warning: Matrix failed to initialize on creation. Initialize it manually in order to use it.\n");
			return;
		}
		this->set(list_);
	}


	void set(std::initializer_list<T> list_)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't assign uninitialized matrix.\n");
			return;
		}

		if (list_.size() != this->size())
		{
			printf("Error: Vector must match matrix layout for assignment to work. (matrix size: %lu, passed size: %lu\n", this->size(), list_.size());
			return;
		}

		for (uint32_t row = 0; row < this->rowCount; row++)
		{
			for (uint32_t column = 0; column < this->columnCount; column++)
			{
				this->objects[row * this->rowStride + column * this->elementStride] = *(list_.begin() + row * this->columnCount + column);
			}
		}
	}


	void editRow(std::vector<T>& rSrcRow_, uint32_t rowId_, uint32_t dstFirstElementId_, uint32_t srcFirstElementId_, uint32_t elementAmountToCopy_) // Full or partial replacement of row(its stored and edited vertically due to how GPU expects matrix format)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		if (rowId_ >= this->rowCount)
		{
			printf("Error: This matrix has only %u rows.\n", static_cast<unsigned int>(this->rowCount));
			return;
		}

		uint32_t srcLastElementId = srcFirstElementId_ + elementAmountToCopy_ - 1;
		if (srcLastElementId > rSrcRow_.size() - 1)
		{
			printf("Error: Source matrix has only %zu elements.\n", rSrcRow_.size());
			return;
		}

		uint32_t dstLastElementId = dstFirstElementId_ + elementAmountToCopy_ - 1;
		if (dstLastElementId > this->columnCount - 1)
		{
			printf("Error: This matrix has only %u elements in row.\n", static_cast<unsigned int>(this->columnCount));
			return;
		}

		for (uint32_t i = 0; i < elementAmountToCopy_; i++)
		{
			this->objects[rowId_ * this->rowStride + (dstFirstElementId_ + i) * this->elementStride] = rSrcRow_[srcFirstElementId_ + i];
		}
	}


	void editColumn(std::vector<T>& rSrcRow_, uint32_t columnId_, uint32_t dstFirstElementId_, uint32_t srcFirstElementId_, uint32_t elementAmountToCopy_)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		if (columnId_ >= this->columnCount)
		{
			printf("Error: This matrix has only %u columns.\n", static_cast<unsigned int>(this->columnCount));
			return;
		}

		uint32_t dstLastElementId = dstFirstElementId_ + elementAmountToCopy_ - 1;
		if (dstLastElementId > this->rowCount - 1)
		{
			printf("Error: This matrix has only %u elements in a column.\n", static_cast<unsigned int>(this->rowCount));
			return;
		}

		uint32_t srcLastElementId = srcFirstElementId_ + elementAmountToCopy_ - 1;
		if (srcLastElementId > rSrcRow_.size() - 1)
		{
			printf("Error: Source matrix has only %zu elements.\n", rSrcRow_.size());
			return;
		}

		for (uint32_t i = 0; i < elementAmountToCopy_; i++)
		{
			this->objects[(i + dstFirstElementId_) * this->rowStride + columnId_ * this->elementStride] = rSrcRow_[srcFirstElementId_ + i];
		}
	}




	void setRow(Matrix<T>& rSrcMatrix_, uint32_t dstRowId_, uint32_t srcRowId_) // Full replacement of a row
	{
		if (!this->wasInitialized || !rSrcMatrix_.wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		if (this->columnCount != rSrcMatrix_.columnCount)
		{
			printf("Error: This matrix and Source matrix have different amount of elements in 1 Row.\n");
			return;
		}

		if (srcRowId_ >= rSrcMatrix_.rowCount)
		{
			printf("Error: Source matrix has only %u rows.\n", static_cast<unsigned int>(rSrcMatrix_.rowCount));
			return;
		}

		if (dstRowId_ >= this->rowCount)
		{
			printf("Error: This matrix has only %u rows.\n", static_cast<unsigned int>(this->rowCount));
			return;
		}

		/*
			A row is contiguous in memory. When both matrices use the same element stride (same alignment rule
			and layout), the whole row, padding included, is identical in both, so it is copied in one block.
			Otherwise the padding differs and the elements are copied one by one.
		*/
		if (this->alignmentRule == rSrcMatrix_.alignmentRule && this->elementStride == rSrcMatrix_.elementStride)
		{
			std::memcpy(&this->objects[dstRowId_ * this->rowStride], &rSrcMatrix_.objects[srcRowId_ * rSrcMatrix_.rowStride], this->columnCount * this->elementStride * sizeof(T));
			return;
		}

		for (uint32_t i = 0; i < this->columnCount; i++)
		{
			this->setElement(dstRowId_, i, rSrcMatrix_.getElement(srcRowId_, i));
		}
	}

	void setColumn(Matrix<T>& rSrcMatrix_, uint32_t dstColumnId_, uint32_t srcColumnId_)
	{
		if (!this->wasInitialized || !rSrcMatrix_.wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		if (this->rowCount != rSrcMatrix_.rowCount)
		{
			printf("Error: This matrix and Source matrix have different amount of elements in 1 Column.\n");
			return;
		}

		if (srcColumnId_ >= rSrcMatrix_.columnCount)
		{
			printf("Error: Source matrix has only %u columns.\n", static_cast<unsigned int>(rSrcMatrix_.columnCount));
			return;
		}

		if (dstColumnId_ >= this->columnCount)
		{
			printf("Error: This matrix has only %u columns.\n", static_cast<unsigned int>(this->columnCount));
			return;
		}

		/*
			A column is spread over one element per row, so it can't be copied as a block even when the strides match.
		*/
		for (uint32_t i = 0; i < this->rowCount; i++)
		{
			this->setElement(i, dstColumnId_, rSrcMatrix_.getElement(i, srcColumnId_));
		}
	}

	void setRow(Array<T>& rSrcArray_, uint32_t dstRowId_) // Full replacement of a row by an Array
	{
		if (!this->wasInitialized || !rSrcArray_.wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix or array.\n");
			return;
		}

		if (this->columnCount != rSrcArray_.elementCount)
		{
			printf("Error: Source array has %u elements, a row of this matrix has %u.\n", rSrcArray_.elementCount, static_cast<unsigned int>(this->columnCount));
			return;
		}

		if (dstRowId_ >= this->rowCount)
		{
			printf("Error: This matrix has only %u rows.\n", static_cast<unsigned int>(this->rowCount));
			return;
		}

		/*
			A row and an Array with the same alignment rule and element stride have identical memory layouts,
			so the row is copied in one block. Otherwise the elements are copied one by one.
		*/
		if (this->alignmentRule == rSrcArray_.alignmentRule && this->elementStride == rSrcArray_.elementStride)
		{
			std::memcpy(&this->objects[dstRowId_ * this->rowStride], &rSrcArray_.object[0], this->columnCount * this->elementStride * sizeof(T));
			return;
		}

		for (uint32_t i = 0; i < this->columnCount; i++)
		{
			this->setElement(dstRowId_, i, rSrcArray_.getElement(i));
		}
	}

	void setColumn(Array<T>& rSrcArray_, uint32_t dstColumnId_) // Full replacement of a column by an Array
	{
		if (!this->wasInitialized || !rSrcArray_.wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix or array.\n");
			return;
		}

		if (this->rowCount != rSrcArray_.elementCount)
		{
			printf("Error: Source array has %u elements, a column of this matrix has %u.\n", rSrcArray_.elementCount, static_cast<unsigned int>(this->rowCount));
			return;
		}

		if (dstColumnId_ >= this->columnCount)
		{
			printf("Error: This matrix has only %u columns.\n", static_cast<unsigned int>(this->columnCount));
			return;
		}

		/*
			A column is spread over one element per row, so it can't be copied as a block.
		*/
		for (uint32_t i = 0; i < this->rowCount; i++)
		{
			this->setElement(i, dstColumnId_, rSrcArray_.getElement(i));
		}
	}

	std::vector<T>& get()
	{
		return this->objects;
	}

	T getElement(uint32_t rowId_, uint32_t columnId_) const
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't read uninitialized matrix.\n");
			return T{};
		}

		if (rowId_ >= this->rowCount || columnId_ >= this->columnCount)
		{
			printf("Error: Element [%u][%u] is out of bounds for this Matrix.\n", rowId_, columnId_);
			return T{};
		}

		return this->objects[rowId_ * this->rowStride + columnId_ * this->elementStride];
	}

	void setElement(uint32_t rowId_, uint32_t columnId_, T value_)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		if (rowId_ >= this->rowCount || columnId_ >= this->columnCount)
		{
			printf("Error: Element [%u][%u] is out of bounds for this Matrix.\n", rowId_, columnId_);
			return;
		}

		this->objects[rowId_ * this->rowStride + columnId_ * this->elementStride] = value_;
	}

	const uint32_t getRowCount() const
	{
		return this->rowCount;
	}

	const uint32_t getColumnCount() const
	{
		return this->columnCount;
	}


	const size_t size() const// return amount of elements
	{
		return this->rowCount * this->columnCount;
	}


	T* data()
	{
		return &this->objects[0];
	}

	bool isRangeInColumnsBound(uint32_t firstColumnId_, uint32_t columnCount_) const
	{

		if (!this->wasInitialized)
		{
			printf("Error: Can't check uninitialized matrix.\n");
			return false;
		}

		uint32_t lastElementId = firstColumnId_ + columnCount_ - 1;
		if (this->columnCount < firstColumnId_ + columnCount_)
		{
			printf("Error: Id range [%u - %u] is out of bounds for Columns amount of this Matrix.\n", firstColumnId_, lastElementId);
			return false;
		}
		return true;
	}

	bool isRangeInRowsBound(uint32_t firstRowId_, uint32_t rowCount_) const
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't check uninitialized matrix.\n");
			return false;
		}

		uint32_t lastRowId = firstRowId_ + rowCount_ - 1;
		if (this->rowCount < firstRowId_ + rowCount_)
		{
			printf("Error: Id range [%u - %u] is out of bounds for Rows amount in this Matrix(No reflection).\n", static_cast<unsigned int>(firstRowId_), static_cast<unsigned int>(lastRowId));
			return false;
		}
		return true;
	}
};
}
