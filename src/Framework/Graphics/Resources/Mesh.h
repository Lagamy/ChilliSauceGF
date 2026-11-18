#pragma once
#include "PoolMap.h"
#include "Utilities.h"
#include "Vertex.h"
#include "MemoryEntryId.h"
#include "GPUDataContainer.h"
#include "ReadbackRequest.h"
#include "Layout.h"
#include "MemoryAccessParams.h"
#include <cstdint>
#include <format>
#include <stdexcept>
#include <vector>
#include <glm/glm.hpp>

namespace Graphics
{
	ReflectionLayout& getVerticeLayout(PoolId layoutId); // Forward declaration of function. Implementation is in API
	ReadbackId requestMemoryEntryReadback(MemoryEntryId entryId_, uint64_t inEntryOffset_, uint64_t size_); // Implementation is in API
	ReadbackDataContainer mapReadback(ReadbackId readbackId_); // Implementation is in API

	/* The mesh keeps no copy of its vertex bytes. Every set writes straight into the memory the entry lives in (a staging range or the CPU_SHARED block), see MemoryManager::getStagingContainer. */
	struct Mesh {
		std::string name;
		PoolId verticeLayoutId;
		uint32_t verticesCount;
		MemoryVisabilityEnum memoryVisability;
		bool readOnly = false;
		std::vector<uint32_t> indices;

		MemoryEntryId vbMemoryUploadId; // Handle to mem entry in the GPUMemoryManager
		MemoryEntryId ibMemoryUploadId;

		/*
			Writes one variable of one vertice. Only valid after queueUpload. Before commitUpload it fills the initial data, after it it's an update (throws if the mesh was queued readOnly).
			The functions below write a part of a MATRIX variable the same way. Each GPU_LOCAL write is its own staging range and update, covering only the bytes it replaces (an element, a row), so a column is written element by element.
			A variable declared with repeatCountPerDimension (int a[2][3]) takes repeatIdPerDimension_ in every function, one id per dimension, outermost first (empty for a variable that isn't repeated).
		*/
		template<typename T>
		void setVerticeVariable(PoolId variableId_, uint32_t verticeId_, T data_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_), rVariable.elementSize, [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setMemberScalar(rContainer_, variableId_, verticeId_, data_, repeatIdPerDimension_);
			});
		}

		template<typename T>
		void setVerticeVariableMatrixElement(PoolId variableId_, uint32_t verticeId_, uint32_t rowId_, uint32_t columnId_, T data_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, columnId_), rVariable.getElementStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setElementInMemberMatrix(rContainer_, variableId_, verticeId_, rowId_, columnId_, data_, repeatIdPerDimension_);
			});
		}

		/* Replaces a whole row. The Array must have one element per column of the variable. */
		template<typename T>
		void setVerticeVariableMatrixRow(PoolId variableId_, uint32_t verticeId_, uint32_t rowId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setRowInMemberMatrix(rContainer_, variableId_, verticeId_, rowId_, rSrcArray_, repeatIdPerDimension_);
			});
		}

		/* Replaces a whole row with row srcRowId_ of a Matrix. The Matrix must have as many columns as the variable. */
		template<typename T>
		void setVerticeVariableMatrixRow(PoolId variableId_, uint32_t verticeId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setRowInMemberMatrix(rContainer_, variableId_, verticeId_, rowId_, rSrcMatrix_, srcRowId_, repeatIdPerDimension_);
			});
		}

		/* Replaces a whole column. The Array must have one element per row of the variable. */
		template<typename T>
		void setVerticeVariableMatrixColumn(PoolId variableId_, uint32_t verticeId_, uint32_t columnId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			if(rSrcArray_.size() != rVariable.shapeMetadata.rowCount)
			{
				throw std::runtime_error(std::format("Mesh {}: Array with {} elements can't replace a column of {} elements.", this->name, rSrcArray_.size(), rVariable.shapeMetadata.rowCount));
			}
			for(uint32_t rowId = 0; rowId < rSrcArray_.size(); rowId++)
			{
				this->setVerticeVariableMatrixElement(variableId_, verticeId_, rowId, columnId_, rSrcArray_.getElement(rowId), repeatIdPerDimension_);
			}
		}

		/* Replaces a whole column with column srcColumnId_ of a Matrix. The Matrix must have as many rows as the variable. */
		template<typename T>
		void setVerticeVariableMatrixColumn(PoolId variableId_, uint32_t verticeId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			if(rSrcMatrix_.getRowCount() != rVariable.shapeMetadata.rowCount || srcColumnId_ >= rSrcMatrix_.getColumnCount())
			{
				throw std::runtime_error(std::format("Mesh {}: Column {} of a {}x{} Matrix can't replace a column of {} elements.", this->name, srcColumnId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rVariable.shapeMetadata.rowCount));
			}
			for(uint32_t rowId = 0; rowId < rSrcMatrix_.getRowCount(); rowId++)
			{
				this->setVerticeVariableMatrixElement(variableId_, verticeId_, rowId, columnId_, rSrcMatrix_.getElement(rowId, srcColumnId_), repeatIdPerDimension_);
			}
		}

		/*
			Reading is asynchronous for GPU_LOCAL meshes: the request queues a copy into a readback range, and the variable can be read once isReadbackReady(readbackId) is true.
			For CPU_SHARED meshes the request is ready immediately and reads the mapped memory. The caller releases the request with releaseReadback after reading.
		*/
		ReadbackId requestVerticeVariableReadback(PoolId variableId_, uint32_t verticeId_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			const VariableBlueprint& rVariable = rLayout.variableBlueprints[variableId_];
			uint64_t inEntryOffset = verticeId_ * rLayout.size + rVariable.firstByteId;
			return requestMemoryEntryReadback(this->vbMemoryUploadId, inEntryOffset, rVariable.size);
		}

		/* Reads the variable a request made with requestVerticeVariableReadback covers. Only valid once the readback is ready. */
		template<typename T>
		T getVerticeVariable(ReadbackId readbackId_, PoolId variableId_, uint32_t verticeId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getMemberScalar<T>(container, variableId_, verticeId_, repeatIdPerDimension_);
		}

		/* Same as getVerticeVariable for one element of a MATRIX variable (the readback of the whole variable covers it). */
		template<typename T>
		T getVerticeVariableMatrixElement(ReadbackId readbackId_, PoolId variableId_, uint32_t verticeId_, uint32_t rowId_, uint32_t columnId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getElementInMemberMatrix<T>(container, variableId_, verticeId_, rowId_, columnId_, repeatIdPerDimension_);
		}

		void queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, const MemoryAccessParams& access_);
		void commitUpload();

		Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_);

	private:
		const VariableBlueprint& getVariable(PoolId variableId_)
		{
			return getVerticeLayout(this->verticeLayoutId).variableBlueprints[variableId_];
		}

		/*
			Calls write_(layout, container) with the container the vertice is written through (a staging range or the CPU_SHARED block), covering the bytes
			[firstByteId + inMemberOffset_, + size_) of the member of vertice verticeId_.
		*/
		template<typename F>
		void writeVariableRange(PoolId variableId_, uint32_t verticeId_, uint64_t inMemberOffset_, uint64_t size_, F&& write_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			uint64_t inEntryOffset = verticeId_ * rLayout.size + rLayout.variableBlueprints[variableId_].firstByteId + inMemberOffset_;

			/* A vertex entry with copies is written through its shadow (the shared container), also when it is GPU_LOCAL. */
			if(this->memoryVisability == GPU_LOCAL && !this->hasCopies())
			{
				StagingDataContainer container = this->getStagingContainer(inEntryOffset, size_);
				container.repeatCount = this->verticesCount;
				write_(rLayout, container);
			}
			else
			{
				SharedDataContainer container = this->getSharedContainer();
				container.repeatCount = this->verticesCount;
				container.validFirstOffset = inEntryOffset;
				container.validSize = size_;
				write_(rLayout, container);
			}
		}

		bool hasCopies();
		StagingDataContainer getStagingContainer(uint64_t inEntryOffset_, uint64_t size_);
		SharedDataContainer getSharedContainer();
	};
}
