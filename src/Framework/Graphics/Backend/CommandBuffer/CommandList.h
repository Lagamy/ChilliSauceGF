#pragma once
#include "CmdParams.h"
#include "Descriptor.h"
#include "Utilities.h"
#include <cstdint>
#include <span>
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

		void addBeginPresentationRenderPass();
		void addEndRenderPass();
		void addBindGraphicsPipeline(PoolId graphicsPipelineId_);
		void addBindDescriptorSet(PoolId descriptorSetId_, std::span<const uint32_t> dynamicUnitIds_ = {});
		void addDrawMeshIndexed(PoolId meshId_, uint32_t instanceCount_ = 1, uint32_t firstInstance_ = 0);
		void addUploadQueuedCopies(); // the copies the MemoryManager queued for the upload slot of the task (MemoryManager::setup's list)
		void clear();
		void record(VkCommandBuffer& cmdBuffer_) const;
		std::vector<PoolId> collectMeshUses() const; // every mesh a draw of the list uses, without recording anything
		std::vector<BoundDescriptor> collectDescriptorUses() const; // every (set, descriptor) the shaders of the draws declare, without recording anything
	};
}
