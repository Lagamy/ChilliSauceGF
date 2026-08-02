// Cpu side dynamic row major matrix class. UI and Reflection System will use it. For generic use, and user matrix like 2d containers in GLSL (float[][] for example). 
#pragma once 
#include <vector>
#include <inttypes.h>
#include <string>
#include <stdexcept>
#include <cstring>

template<typename T>
struct Matrix
{
private:
	std::vector<T> object;
	uint32_t columnCount; // Due to shaders only supporting 32 bit values  
	uint32_t rowCount;
	bool wasInitialized;

public:

	bool init(uint32_t rowCount_, uint32_t columnCount_)
	{
		if (rowCount_ < 2 || columnCount_ < 2)
		{
			printf("Error: Can't create a Matrix with less than 2 rows or columns\n");
			return false; // Didn't initialize 
		}



		this->object.resize(columnCount_ * rowCount_);
		this->columnCount = rowCount_;
		this->rowCount = columnCount_;
		return true;
	}

	Matrix() = default;
	~Matrix() = default;

	Matrix(uint32_t rowCount_, uint32_t columnCount_)
	{
		this->wasInitialized = init(rowCount_, columnCount_);
		if (!this->wasInitialized)
		{
			printf("Warning: Matrix failed to initialize on creation. Initialize it manually in order to use it.\n");
		}
	}

	Matrix(uint32_t rowCount_, uint32_t columnCount_, std::initializer_list<T> list_)
	{
		this->wasInitialized = init(rowCount_, columnCount_);
		if (!this->wasInitialized)
		{
			printf("Warning: Matrix failed to initialize on creation. Initialize it manually in order to use it.\n");
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

		this->object = list_;
	}

	void editRow(std::vector<T>& rSrcRow_, uint32_t rowId_, uint32_t dstFirstElementId_, uint32_t srcFirstElementId_, uint32_t elementAmountToCopy_) // Full or partial replacement of row(its stored and edited vertically due to how GPU expects matrix format)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		uint32_t srcLastElementId = srcFirstElementId_ + elementAmountToCopy_ - 1;
		if (srcLastElementId > rSrcRow_.size() - 1)
		{
			printf("Error: Source matrix has only %zu elements.\n", rSrcRow_.size());
			return;
		}

		uint32_t dstLastElementId = dstFirstElementId_ + elementAmountToCopy_ - 1;
		if (dstLastElementId > this->rowCount - 1)
		{
			printf("Error: This matrix has only %u elements in row.\n", static_cast<unsigned int>(this->rowCount));
			return;
		}
		std::memcpy(&this->object[rowId_ * this->rowCount + dstFirstElementId_], rSrcRow_.data() + srcFirstElementId_, elementAmountToCopy_ * sizeof(T));

		
	}


	void editColumn(std::vector<T>& rSrcRow_, uint32_t columnId_, uint32_t dstFirstElementId_, uint32_t srcFirstElementId_, uint32_t elementAmountToCopy_)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		uint32_t dstLastElementId = dstFirstElementId_ + elementAmountToCopy_ - 1;
		if (dstLastElementId > this->columnCount - 1)
		{
			printf("Error: This matrix has only %u elements in a column.\n", static_cast<unsigned int>(this->columnCount));
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
			this->object[(i + dstFirstElementId_) * this->rowCount + columnId_] = rSrcRow_[srcFirstElementId_ + i];
		}
	}




	void swapRow(Matrix<T>& rSrcMatrix_, uint32_t dstRowId_, uint32_t srcRowId_) // Full or partial replacement of row(its stored and edited vertically due to how GPU expects matrix format)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}


		if (this->rowCount != rSrcMatrix_.rowCount)
		{
			printf("Error: This matrix and Source matrix have different amount of elements in 1 Row.\n");
			return;
		}

		if (srcRowId_ > rSrcMatrix_.columnCount - 1)
		{
			printf("Error: Source matrix has only %u rows.\n", static_cast<unsigned int>(rSrcMatrix_.columnCount));
			return;
		}

		if (dstRowId_ > this->columnCount - 1)
		{
			printf("Error: This matrix has only %u rows.\n", static_cast<unsigned int>(this->columnCount));
			return;
		}

		for (uint32_t i = 0; i < this->rowCount; i++)
		{
			this->object[i * this->columnCount + dstRowId_] = rSrcMatrix_.get()[i * this->columnCount + srcRowId_];
		}
	}

	void swapColumn(Matrix<T>& rSrcMatrix_, uint32_t dstColumnId_, uint32_t srcColumnId_)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized matrix.\n");
			return;
		}

		if (this->columnCount != rSrcMatrix_.columnCount)
		{
			printf("Error: This matrix and Source matrix have different amount of elements in 1 Column.\n");
			return;
		}

		if (srcColumnId_ > rSrcMatrix_.rowCount - 1)
		{
			printf("Error: Source matrix has only %u columns.\n", static_cast<unsigned int>(rSrcMatrix_.columnCount));
			return;
		}

		if (dstColumnId_ > this->rowCount - 1)
		{
			printf("Error: This matrix has only %u columns.\n", static_cast<unsigned int>(this->columnCount));
			return;
		}


		std::memcpy(&this->object[dstColumnId_ * columnCount], &rSrcMatrix_.get()[srcColumnId_ * columnCount], this->columnCount * sizeof(T));
	}

	std::vector<T>& get() 
	{
		return this->object;
	}

	const T& getElement (uint32_t rowId_, uint32_t columnId_) const
	{
		return this->object[rowId_ * this->rowCount + columnId_];
	}

	const T* getElementAdress (uint32_t rowId_, uint32_t columnId_) const
	{
		return &this->object[rowId_ * this->rowCount + columnId_];
	}

	const uint32_t getRowCount() const
	{
		return this->columnCount;
	}

	const uint32_t getColumnCount() const
	{
		return this->rowCount;
	}


	const size_t size() const// return amount of elements 
	{
		return this->columnCount * this->rowCount;
	}


	T* data()
	{
		return &this->object[0];
	}

	bool isRangeInColumnsBound(uint32_t firstColumnId_, uint32_t columnCount_) const
	{

		if (!this->wasInitialized)
		{
			printf("Error: Can't check uninitialized matrix.\n");
			return false;
		}

		uint32_t lastElementId = firstColumnId_ + columnCount_ - 1;
		if (this->rowCount < firstColumnId_ + columnCount_)
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
		if (this->columnCount < firstRowId_ + rowCount_)
		{
			printf("Error: Id range [%u - %u] is out of bounds for Rows amount in this Matrix(No reflection).\n", static_cast<unsigned int>(firstRowId_), static_cast<unsigned int>(lastRowId));
			return false;
		}
		return true;
	}
};
