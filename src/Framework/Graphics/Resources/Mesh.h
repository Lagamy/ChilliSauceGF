#pragma once
#include "PoolMap.h"
#include "Utilities.h"
#include "Vertex.h"
#include "MemoryEntryId.h"
#include "GPUDataContainer.h"
#include "ReadbackRequest.h"
#include "Layout.h"
#include "MemoryAccessParams.h"
#include "EntryCopyRequest.h"
#include "Residency.h"
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
		AllocatorTypeEnum allocatorType = DYNAMIC_ALLOCATOR; // as given to queueUpload, a new vertex entry (restride) is made the same way
		MemoryAccessParams access;
		ResidencyParams residencyParams; // as given to queueUpload (a mesh queued without them never unloads)
		Residency residency; // load / unload state of the vertex and index entries together, see unload and load
		std::vector<uint32_t> indices;

		MemoryEntryId vbMemoryUploadId; // Handle to mem entry in the GPUMemoryManager
		MemoryEntryId ibMemoryUploadId;

		/*
			Writes one variable of one vertice (variableSelector_: the id addVariableToVerticeLayout returned, or the orderPosition the variable was added at, see VariableSelector). Only valid after queueUpload. Before commitUpload it fills the initial data, after it it's an update (throws if the mesh was queued readOnly).
			The functions below write a part of a MATRIX variable the same way. Each GPU_LOCAL write is its own staging range and update, covering only the bytes it replaces (an element, a row), so a column is written element by element.
			A variable declared with repeatCountPerDimension (int a[2][3]) takes repeatIdPerDimension_ in every function, one id per dimension, outermost first (empty for a variable that isn't repeated).
		*/
		template<typename T>
		void setVerticeVariable(VariableSelector variableSelector_, uint32_t verticeId_, T data_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_), rVariable.elementSize, [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setMemberScalar(rContainer_, variableId_, verticeId_, data_, repeatIdPerDimension_);
			});
		}

		template<typename T>
		void setVerticeVariableMatrixElement(VariableSelector variableSelector_, uint32_t verticeId_, uint32_t rowId_, uint32_t columnId_, T data_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, columnId_), rVariable.getElementStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setElementInMemberMatrix(rContainer_, variableId_, verticeId_, rowId_, columnId_, data_, repeatIdPerDimension_);
			});
		}

		/* Replaces a whole row. The Array must have one element per column of the variable. */
		template<typename T>
		void setVerticeVariableMatrixRow(VariableSelector variableSelector_, uint32_t verticeId_, uint32_t rowId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setRowInMemberMatrix(rContainer_, variableId_, verticeId_, rowId_, rSrcArray_, repeatIdPerDimension_);
			});
		}

		/* Replaces a whole row with row srcRowId_ of a Matrix. The Matrix must have as many columns as the variable. */
		template<typename T>
		void setVerticeVariableMatrixRow(VariableSelector variableSelector_, uint32_t verticeId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
			const VariableBlueprint& rVariable = this->getVariable(variableId_);
			this->writeVariableRange(variableId_, verticeId_, rVariable.getRepeatOffset(repeatIdPerDimension_) + rVariable.getMatrixElementOffset(rowId_, 0), rVariable.getRowStride(), [&](ReflectionLayout& rLayout_, auto& rContainer_)
			{
				rLayout_.setRowInMemberMatrix(rContainer_, variableId_, verticeId_, rowId_, rSrcMatrix_, srcRowId_, repeatIdPerDimension_);
			});
		}

		/* Replaces a whole column. The Array must have one element per row of the variable. */
		template<typename T>
		void setVerticeVariableMatrixColumn(VariableSelector variableSelector_, uint32_t verticeId_, uint32_t columnId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
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
		void setVerticeVariableMatrixColumn(VariableSelector variableSelector_, uint32_t verticeId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
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
		ReadbackId requestVerticeVariableReadback(VariableSelector variableSelector_, uint32_t verticeId_)
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
			if(!this->residency.loaded)
			{
				throw std::runtime_error(std::format("Mesh {}: is unloaded, it can't be read back until it was loaded again.", this->name));
			}
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			const VariableBlueprint& rVariable = rLayout.variableBlueprints[variableId_];
			uint64_t inEntryOffset = verticeId_ * rLayout.size + rVariable.firstByteId;
			return requestMemoryEntryReadback(this->vbMemoryUploadId, inEntryOffset, rVariable.size);
		}

		/* Reads the variable a request made with requestVerticeVariableReadback covers. Only valid once the readback is ready. */
		template<typename T>
		T getVerticeVariable(ReadbackId readbackId_, VariableSelector variableSelector_, uint32_t verticeId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getMemberScalar<T>(container, variableId_, verticeId_, repeatIdPerDimension_);
		}

		/* Same as getVerticeVariable for one element of a MATRIX variable (the readback of the whole variable covers it). */
		template<typename T>
		T getVerticeVariableMatrixElement(ReadbackId readbackId_, VariableSelector variableSelector_, uint32_t verticeId_, uint32_t rowId_, uint32_t columnId_, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			PoolId variableId_ = this->getVariableId(variableSelector_);
			ReflectionLayout& rLayout = getVerticeLayout(this->verticeLayoutId);
			ReadbackDataContainer container = mapReadback(readbackId_);
			container.repeatCount = this->verticesCount;
			return rLayout.getElementInMemberMatrix<T>(container, variableId_, verticeId_, rowId_, columnId_, repeatIdPerDimension_);
		}

		/* The version without residency_ never unloads the mesh and keeps no CPU copy of its vertex bytes. */
		void queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, const MemoryAccessParams& access_);
		void queueUpload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_, const MemoryAccessParams& access_, const ResidencyParams& residency_);
		void commitUpload();

		/*
			On demand loading, like a descriptor (DescriptorSet::loadDescriptor): the vertex and index entries leave memory together (unload) and come back (load: new entries, the bytes from loadFromDisk or the CPU copy, then commitUpload).
			Only a DYNAMIC_ALLOCATOR mesh can unload. Before an unload the mesh has to be committed, and it needs a way to get its bytes back: loadFromDisk, an entry with a shadow (it moves into Residency::backup), or an absenceSecondsCount (the writes are mirrored into the backup), otherwise unload throws.
			The caller waits for the submits that used the mesh first (PassesGraph::waitForMeshUse; a buffer may not be destroyed while a pending command buffer uses it, VUID-vkDestroyBuffer-buffer-00922), ResourceManager::updateLoading and unloadMesh do. meshId_ is the id loadFromDisk gets as its ownerId_ (partId_ is UninitializedPoolId).
			markUsed restarts the absence time and requests a load of an unloaded mesh, isSettled is true once the mesh is loaded and the copy of the current frame of both entries was filled, so a draw may read it.
		*/
		void unload();
		void load(PoolId meshId_);
		void markUsed();
		void markDiskLayoutChanged(); // changeLayoutOnDisk was called for the mesh: loadFromDisk may be used again
		bool isSettled();
		bool canUnload() const;
		bool unloadsAutomatically() const;

		/*
			For ReflectionSystem::commitVertexLayoutUpdate. hasVertexEntry: queueUpload was called. throwIfCantReplaceVertexEntry: the entry can't be replaced (static, or reserved and not committed).
			restride: the vertex entry was made for rOldLayout_, afterwards it fits rNewLayout_. An entry that fits as it is stays. Otherwise a new entry takes its place: a GPU_LOCAL one without copies by a GPU side copy (MemoryManager::copyEntry),
			any other by rewriting the bytes on the CPU (their data is readable: CPU_SHARED memory, or the shadow of an entry with copies). The bytes of variables that are new are 0 (MemoryManager::zeroEntryRanges).
		*/
		bool hasVertexEntry() const;
		void throwIfCantReplaceVertexEntry();
		void restride(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_);

		Mesh(const char* name_, PoolId verticeLayoutId_, uint32_t repeatCount_);

	private:
		PoolId getVariableId(const VariableSelector& rVariable_)
		{
			return getVerticeLayout(this->verticeLayoutId).getVariableId(rVariable_);
		}

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
			if(!this->residency.loaded)
			{
				throw std::runtime_error(std::format("Mesh {}: is unloaded, it can't be written until it was loaded again (load, or a draw / pass that uses it).", this->name));
			}
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

			/* The same write goes into the CPU copy, which is what a load writes back (the entry's bytes are gone with the entry). Same offsets, the copy is as large as the entry. An entry with a shadow already is that copy. */
			if(this->mirrorsWrites())
			{
				this->residency.backup.repeatCount = this->verticesCount;
				write_(rLayout, this->residency.backup);
			}
		}

		bool hasCopies();
		bool mirrorsWrites() const; // canUnload, an absenceSecondsCount, no loadFromDisk and no shadow: every write is mirrored into Residency::backup
		void reserveVertexEntry(); // vbMemoryUploadId: the vertex entry reserved (no data yet), used by queueUpload and load
		void uploadIndexEntry(); // ibMemoryUploadId: the index entry uploaded from indices, locked when the mesh is readOnly, used by queueUpload and load
		void restoreBackup(); // a load writes the backup into the reserved vertex entry
		void remapBackup(const ReflectionLayout& rOldLayout_, const ReflectionLayout& rNewLayout_); // restride: the CPU copy moves to the new offsets
		StagingDataContainer getStagingContainer(uint64_t inEntryOffset_, uint64_t size_);
		SharedDataContainer getSharedContainer();
	};
}
