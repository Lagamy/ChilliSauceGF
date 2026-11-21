#pragma once
#include "Commands.h"
#include "Descriptor.h"
#include "Utilities.h"
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>
#include <vulkan/vulkan.h>

namespace Graphics
{
	/* One command of the list: its type and the index of its params in the vector of that type (UninitializedId for a command without params). */
	struct CmdEntry
	{
		CmdTypeEnum type;
		uint32_t paramsId;
	};

	/*
		A recipe for one command buffer, not a Vulkan object. A Task points at it by id (Task::commandListId) and it is replayed every time the task is recorded.
		Append only: removing a command from the middle would shift the params ids of every later command of its type, so a list is changed by clear() and adding the commands again.
	*/
	struct CommandList
	{
		std::vector<CmdEntry> order;
		std::vector<BindGraphicsPipelineParams> bindGraphicsPipelineParams;
		std::vector<BindDescriptorSetParams> bindDescriptorSetParams;
		std::vector<DrawMeshIndexedParams> drawMeshIndexedParams;
		std::vector<SetPushConstantMemberValueParams> setPushConstantMemberValueParams;

		void addBeginPresentationRenderPass();
		void addEndRenderPass();
		void addBindGraphicsPipeline(PoolId graphicsPipelineId_);
		void addBindDescriptorSet(PoolId descriptorSetId_, std::span<const uint32_t> dynamicUnitIds_ = {});
		void addDrawMeshIndexed(PoolId meshId_, uint32_t instanceCount_ = 1, uint32_t firstInstance_ = 0);
		/*
			Pushes a variable of a push constant layout with the pipeline bound before it (vkCmdPushConstants, only the bytes of the variable). Two forms of every function:
			- a pointer (const T*, const Array<T>*, const Matrix<T>*): the value is read from it every time the list is recorded (a FRAME pass every frame, a ONESHOT pass once), so it has to stay valid and at the same address while the list is used.
			- a value (const T&, ...): copied into the command when it is added, so pushing 17 or a temporary works and the command always pushes that same value.
			T has to be the type of the variable (throws otherwise, here and not when it is recorded). idsPerLayer_ has one id per repeat layer of the variable's struct (none for a variable directly in the block), repeatIdPerDimension_ one per dimension of the variable, as in the setters of a descriptor.
			The matrix functions are for MATRIX shaped variables: an element, a row (from an Array with one element per column, or row srcRowId_ of a Matrix) and a column (an Array with one element per row, or column srcColumnId_ of a Matrix).
		*/
		template<typename T>
		void addSetPushConstantMemberValue(PoolId pushConstantLayoutId_, VariableSelector variableId_, const T* pValue_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_WHOLE_VARIABLE, 0, 0, 0, pValue_, &writeValue<T>}, false);
		}

		template<typename T> requires (!std::is_pointer_v<T>)
		void addSetPushConstantMemberValue(PoolId pushConstantLayoutId_, VariableSelector variableId_, const T& rValue_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_WHOLE_VARIABLE, 0, 0, 0, &rValue_, &writeValue<T>}, true);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixElement(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t rowId_, uint32_t columnId_, const T* pValue_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_ELEMENT, rowId_, columnId_, 0, pValue_, &writeElement<T>}, false);
		}

		template<typename T> requires (!std::is_pointer_v<T>)
		void addSetPushConstantMemberMatrixElement(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t rowId_, uint32_t columnId_, const T& rValue_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_ELEMENT, rowId_, columnId_, 0, &rValue_, &writeElement<T>}, true);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixRow(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t rowId_, const Array<T>* pSrcArray_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_ROW, rowId_, 0, 0, pSrcArray_, &writeRowFromArray<T>}, false);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixRow(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t rowId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_ROW, rowId_, 0, 0, &rSrcArray_, &writeRowFromArray<T>}, true);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixRow(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t rowId_, const Matrix<T>* pSrcMatrix_, uint32_t srcRowId_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_ROW, rowId_, 0, srcRowId_, pSrcMatrix_, &writeRowFromMatrix<T>}, false);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixRow(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t rowId_, const Matrix<T>& rSrcMatrix_, uint32_t srcRowId_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_ROW, rowId_, 0, srcRowId_, &rSrcMatrix_, &writeRowFromMatrix<T>}, true);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixColumn(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t columnId_, const Array<T>* pSrcArray_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_COLUMN, 0, columnId_, 0, pSrcArray_, &writeColumnFromArray<T>}, false);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixColumn(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t columnId_, const Array<T>& rSrcArray_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_COLUMN, 0, columnId_, 0, &rSrcArray_, &writeColumnFromArray<T>}, true);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixColumn(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t columnId_, const Matrix<T>* pSrcMatrix_, uint32_t srcColumnId_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_COLUMN, 0, columnId_, srcColumnId_, pSrcMatrix_, &writeColumnFromMatrix<T>}, false);
		}

		template<typename T>
		void addSetPushConstantMemberMatrixColumn(PoolId pushConstantLayoutId_, VariableSelector variableId_, uint32_t columnId_, const Matrix<T>& rSrcMatrix_, uint32_t srcColumnId_, const std::vector<uint32_t>& idsPerLayer_ = {}, const std::vector<uint32_t>& repeatIdPerDimension_ = {})
		{
			this->addPushConstantValue(pushConstantLayoutId_, {variableId_, idsPerLayer_, repeatIdPerDimension_, PUSH_CONSTANT_MATRIX_COLUMN, 0, columnId_, srcColumnId_, &rSrcMatrix_, &writeColumnFromMatrix<T>}, true);
		}

		void addUploadQueuedCopies(); // the copies the MemoryManager queued for the upload slot of the task (MemoryManager::setup's list)
		void clear();
		void record(VkCommandBuffer& cmdBuffer_) const;
		std::vector<PoolId> collectMeshUses() const; // every mesh a draw of the list uses, without recording anything
		std::vector<BoundDescriptor> collectDescriptorUses() const; // every (set, descriptor) the shaders of the draws declare, without recording anything

	private:
		void addPushConstantValue(PoolId pushConstantLayoutId_, const PushConstantWrite& rWrite_, bool cache_); // makes the bytes once so a wrong type, shape or id throws here; keeps them when cache_, otherwise keeps the pointer

		/* The functions behind PushConstantWriteFunc: they cast the source back to the type the command was added with and write it through the layout, which checks the type and shape of the variable. */
		template<typename T>
		static void writeValue(const PushConstantWrite& rWrite_, ReflectionLayout& rLayout_, CPUDataContainer& rContainer_)
		{
			rLayout_.setMemberScalar(rContainer_, rWrite_.variableId, 0, *static_cast<const T*>(rWrite_.pData), rWrite_.repeatIdPerDimension);
		}

		template<typename T>
		static void writeElement(const PushConstantWrite& rWrite_, ReflectionLayout& rLayout_, CPUDataContainer& rContainer_)
		{
			rLayout_.setElementInMemberMatrix(rContainer_, rWrite_.variableId, 0, rWrite_.rowId, rWrite_.columnId, *static_cast<const T*>(rWrite_.pData), rWrite_.repeatIdPerDimension);
		}

		template<typename T>
		static void writeRowFromArray(const PushConstantWrite& rWrite_, ReflectionLayout& rLayout_, CPUDataContainer& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, rWrite_.variableId, 0, rWrite_.rowId, *static_cast<const Array<T>*>(rWrite_.pData), rWrite_.repeatIdPerDimension);
		}

		template<typename T>
		static void writeRowFromMatrix(const PushConstantWrite& rWrite_, ReflectionLayout& rLayout_, CPUDataContainer& rContainer_)
		{
			rLayout_.setRowInMemberMatrix(rContainer_, rWrite_.variableId, 0, rWrite_.rowId, *static_cast<const Matrix<T>*>(rWrite_.pData), rWrite_.srcId, rWrite_.repeatIdPerDimension);
		}

		template<typename T>
		static void writeColumnFromArray(const PushConstantWrite& rWrite_, ReflectionLayout& rLayout_, CPUDataContainer& rContainer_)
		{
			rLayout_.setColumnInMemberMatrix(rContainer_, rWrite_.variableId, 0, rWrite_.columnId, *static_cast<const Array<T>*>(rWrite_.pData), rWrite_.repeatIdPerDimension);
		}

		template<typename T>
		static void writeColumnFromMatrix(const PushConstantWrite& rWrite_, ReflectionLayout& rLayout_, CPUDataContainer& rContainer_)
		{
			rLayout_.setColumnInMemberMatrix(rContainer_, rWrite_.variableId, 0, rWrite_.columnId, *static_cast<const Matrix<T>*>(rWrite_.pData), rWrite_.srcId, rWrite_.repeatIdPerDimension);
		}
	};
}
