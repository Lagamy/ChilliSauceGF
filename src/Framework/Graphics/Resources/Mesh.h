#pragma once
#include "PoolMap.h"
#include "Utilities.h"
#include "Vertex.h"
#include "MemoryEntryId.h"
#include "GPUDataContainer.h"
#include "ReadbackRequest.h"
#include "Layout.h"
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
			Writes one member of one vertice. Only valid after queueUpload. Before commitUpload it fills the initial data, after it it's an update (throws if the mesh was queued readOnly).
			The functions below write a part of an ARRAY / MATRIX member the same way. Each GPU_LOCAL write is its own staging range and update, covering only the bytes it replaces (an element, a row), so a column is written element by element.
		*/
		template<typename T>
		void setVerticeMember(PoolId memberId_, uint32_t verticeId_, T data_)
		{
			this->writeMemberRange(memberId_, verticeId_, 0, this->getMember(memberId_).size, [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setMemberScalar(rContainer_, memberId_, verticeId_, data_);
			});
		}

		template<typename T>
		void setVerticeMemberArrayElement(PoolId memberId_, uint32_t verticeId_, uint32_t elementId_, T data_)
		{
			const MemberBlueprint& rMember = this->getMember(memberId_);
			this->writeMemberRange(memberId_, verticeId_, rMember.getArrayElementOffset(elementId_), rMember.getElementStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setElementInMemberArray(rContainer_, memberId_, verticeId_, elementId_, data_);
			});
		}

		template<typename T>
		void setVerticeMemberMatrixElement(PoolId memberId_, uint32_t verticeId_, uint32_t rowId_, uint32_t columnId_, T data_)
		{
			const MemberBlueprint& rMember = this->getMember(memberId_);
			this->writeMemberRange(memberId_, verticeId_, rMember.getMatrixElementOffset(rowId_, columnId_), rMember.getElementStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setElementInMemberMatrix(rContainer_, memberId_, verticeId_, rowId_, columnId_, data_);
			});
		}

		/* Replaces a whole row. The Array must have one element per column of the member. */
		template<typename T>
		void setVerticeMemberMatrixRow(PoolId memberId_, uint32_t verticeId_, uint32_t rowId_, const Array<T>& rSrcArray_)
		{
			const MemberBlueprint& rMember = this->getMember(memberId_);
			this->writeMemberRange(memberId_, verticeId_, rMember.getMatrixElementOffset(rowId_, 0), rMember.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setRowInMemberMatrix(rContainer_, memberId_, verticeId_, rowId_, rSrcArray_);
			});
		}

		/* Replaces a whole row with row srcRowId_ of a Matrix. The Matrix must have as many columns as the member. */
		template<typename T>
		void setVerticeMemberMatrixRow(PoolId memberId_, uint32_t verticeId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_)
		{
			const MemberBlueprint& rMember = this->getMember(memberId_);
			this->writeMemberRange(memberId_, verticeId_, rMember.getMatrixElementOffset(rowId_, 0), rMember.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setRowInMemberMatrix(rContainer_, memberId_, verticeId_, rowId_, rSrcMatrix_, srcRowId_);
			});
		}

		/* Replaces a whole column. The Array must have one element per row of the member. */
		template<typename T>
		void setVerticeMemberMatrixColumn(PoolId memberId_, uint32_t verticeId_, uint32_t columnId_, const Array<T>& rSrcArray_)
		{
			const MemberBlueprint& rMember = this->getMember(memberId_);
			if(rSrcArray_.size() != rMember.shapeMetadata.rowCount)
			{
				throw std::runtime_error(std::format("Mesh {}: Array with {} elements can't replace a column of {} elements.", this->name, rSrcArray_.size(), rMember.shapeMetadata.rowCount));
			}
			for(uint32_t rowId = 0; rowId < rSrcArray_.size(); rowId++)
			{
				this->setVerticeMemberMatrixElement(memberId_, verticeId_, rowId, columnId_, rSrcArray_.getElement(rowId));
			}
		}

		/* Replaces a whole column with column srcColumnId_ of a Matrix. The Matrix must have as many rows as the member. */
		template<typename T>
		void setVerticeMemberMatrixColumn(PoolId memberId_, uint32_t verticeId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_)
		{
			const MemberBlueprint& rMember = this->getMember(memberId_);
			if(rSrcMatrix_.getRowCount() != rMember.shapeMetadata.rowCount || srcColumnId_ >= rSrcMatrix_.getColumnCount())
			{
				throw std::runtime_error(std::format("Mesh {}: Column {} of a {}x{} Matrix can't replace a column of {} elements.", this->name, srcColumnId_, rSrcMatrix_.getRowCount(), rSrcMatrix_.getColumnCount(), rMember.shapeMetadata.rowCount));
			}
			for(uint32_t rowId = 0; rowId < rSrcMatrix_.getRowCount(); rowId++)
			{
				this->setVerticeMemberMatrixElement(memberId_, verticeId_, rowId, columnId_, rSrcMatrix_.getElement(rowId, srcColumnId_));
			}
		}

		/*
			Reading is asynchronous for GPU_LOCAL meshes: the request queues a copy into a readback range, and the member can be read once isReadbackReady(readbackId) is true.
			For CPU_SHARED meshes the request is ready immediately and reads the mapped memory. The caller releases the request with releaseReadback after reading.
		*/
		ReadbackId requestVerticeMemberReadback(PoolId memberId_, uint32_t verticeId_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			const MemberBlueprint& rMember = rLayout.memberBlueprints[memberId_];
			uint64_t inEntryOffset = verticeId_ * rLayout.size + rMember.firstByteId;
			return requestMemoryEntryReadback(this->vbMemoryUploadId, inEntryOffset, rMember.size);
		}

		/* Reads the member a request made with requestVerticeMemberReadback covers. Only valid once the readback is ready. */
		template<typename T>
		T getVerticeMember(ReadbackId readbackId_, PoolId memberId_, uint32_t verticeId_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getMemberScalar<T>(container, memberId_, verticeId_);
		}

		/* Same as getVerticeMember for one element of an ARRAY / MATRIX member (the readback of the whole member covers it). */
		template<typename T>
		T getVerticeMemberArrayElement(ReadbackId readbackId_, PoolId memberId_, uint32_t verticeId_, uint32_t elementId_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getElementInMemberArray<T>(container, memberId_, verticeId_, elementId_);
		}

		template<typename T>
		T getVerticeMemberMatrixElement(ReadbackId readbackId_, PoolId memberId_, uint32_t verticeId_, uint32_t rowId_, uint32_t columnId_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getElementInMemberMatrix<T>(container, memberId_, verticeId_, rowId_, columnId_);
		}

		void queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, bool readOnly_ = false);
		void commitUpload();

		Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_);

	private:
		const MemberBlueprint& getMember(PoolId memberId_)
		{
			return getVerticeLayout(this->verticeLayoutId).memberBlueprints[memberId_];
		}

		/*
			Calls write_(layout, container) with the container the vertice is written through (a staging range or the CPU_SHARED block), covering the bytes
			[firstByteId + inMemberOffset_, + size_) of the member of vertice verticeId_.
		*/
		template<typename F>
		void writeMemberRange(PoolId memberId_, uint32_t verticeId_, uint64_t inMemberOffset_, uint64_t size_, F&& write_)
		{
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			uint64_t inEntryOffset = verticeId_ * rLayout.size + rLayout.memberBlueprints[memberId_].firstByteId + inMemberOffset_;

			if(this->memoryVisability == GPU_LOCAL)
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

		StagingDataContainer getStagingContainer(uint64_t inEntryOffset_, uint64_t size_);
		SharedDataContainer getSharedContainer();
	};
}
