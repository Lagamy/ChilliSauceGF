#pragma once 
#include <vector>
#include <inttypes.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include "Utilities.h"

namespace Graphics 
{
template<typename T>
struct Array
{
private:
	template<typename U> friend struct Matrix; // Matrix::setRow/setColumn read the layout of a source Array

	std::vector<T> object;
	uint8_t alignmentRule = 1;
	uint32_t elementStride = 1; // Distance between two elements, in T units (includes padding)
	uint32_t elementCount = 0;
	bool wasInitialized = false;

public:

	bool init(uint32_t elementCount_, STDAlignmentLayoutEnum stdAlignmentLayout_)
	{
		if (elementCount_ < 2)
		{
			printf("Error: Can't create an Array with less than 2 elements\n");
			return false; // Didn't initialize 
		}

		/*
			Elements of a GLSL array start at a multiple of the layout's array alignment (std140 rounds it up to 16,
			std430 keeps the element's own alignment), so the stride is sizeof(T) rounded up to it. The padding
			is stored as real T slots, which keeps the vector byte-identical to the GPU layout.
		*/
		this->alignmentRule = resolveSTDPaddingRule(stdAlignmentLayout_, GPUTypeMap<T>::value, true);
		uint32_t strideBytes = static_cast<uint32_t>(alignUp(sizeof(T), this->alignmentRule));
		if (strideBytes % sizeof(T) != 0)
		{
			printf("Error: Element stride of %u bytes is not a multiple of the element size.\n", strideBytes);
			return false;
		}

		this->elementStride = strideBytes / sizeof(T);
		this->object.resize(elementCount_ * this->elementStride);
		this->elementCount = elementCount_;
		this->wasInitialized = true;
		return true;
	}

	Array() = default;
	~Array() = default;

	Array(uint32_t elementCount_, STDAlignmentLayoutEnum stdAlignmentLayout_)
	{
		this->wasInitialized = init(elementCount_, stdAlignmentLayout_);
		if (!this->wasInitialized)
		{
			printf("Warning: Array failed to initialize on creation. Initialize it manually in order to use it.\n");
		}
	}

	Array(uint32_t elementCount_, STDAlignmentLayoutEnum stdAlignmentLayout_, std::initializer_list<T> list_)
	{
		this->wasInitialized = init(elementCount_, stdAlignmentLayout_);
		if (!this->wasInitialized)
		{
			printf("Warning: Array failed to initialize on creation. Initialize it manually in order to use it.\n");
			return;
		}
		this->set(list_);
	}


	void set(std::initializer_list<T> list_)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't assign uninitialized array.\n");
			return;
		}

		if (list_.size() != this->size())
		{
			printf("Error: Vector must match array layout for assignment to work. (array size: %lu, passed size: %lu\n", this->size(), list_.size());
			return;
		}

		for (uint32_t i = 0; i < this->elementCount; i++)
		{
			this->object[i * this->elementStride] = *(list_.begin() + i);
		}
	}

	std::vector<T>& get() 
	{
		return this->object;
	}

	T getElement(uint32_t elementId_) const
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't read uninitialized array.\n");
			return T{};
		}

		if (elementId_ >= this->elementCount)
		{
			printf("Error: Array has only %u elements.\n", this->elementCount);
			return T{};
		}

		return this->object[elementId_ * this->elementStride];
	}

	void setElement(uint32_t elementId_, T value_)
	{
		if (!this->wasInitialized)
		{
			printf("Error: Can't edit uninitialized array.\n");
			return;
		}

		if (elementId_ >= this->elementCount)
		{
			printf("Error: Array has only %u elements.\n", this->elementCount);
			return;
		}

		this->object[elementId_ * this->elementStride] = value_;
	}

	const size_t size() const// return amount of elements 
	{
		return this->elementCount;
	}

	T* data()
	{
		return &this->object[0];
	}

};
}