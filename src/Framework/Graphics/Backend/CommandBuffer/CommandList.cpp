#include "CommandList.h"
#include "Api.h"
#include "Globals.h"
#include "MemoryManager.h"
#include <format>
#include <stdexcept>

namespace Graphics
{
	void CommandList::addBeginPresentationRenderPass()
	{
		this->order.push_back({CMD_BEGIN_PRESENTATION_RENDER_PASS, UninitializedId});
	}

	void CommandList::addEndRenderPass()
	{
		this->order.push_back({CMD_END_RENDER_PASS, UninitializedId});
	}

	void CommandList::addBindGraphicsPipeline(PoolId graphicsPipelineId_)
	{
		this->order.push_back({CMD_BIND_GRAPHICS_PIPELINE, static_cast<uint32_t>(this->bindGraphicsPipelineParams.size())});
		this->bindGraphicsPipelineParams.push_back({graphicsPipelineId_});
	}

	void CommandList::addBindDescriptorSet(PoolId descriptorSetId_, std::span<const uint32_t> dynamicUnitIds_)
	{
		this->order.push_back({CMD_BIND_DESCRIPTOR_SET, static_cast<uint32_t>(this->bindDescriptorSetParams.size())});
		this->bindDescriptorSetParams.push_back({descriptorSetId_, std::vector<uint32_t>(dynamicUnitIds_.begin(), dynamicUnitIds_.end())});
	}

	void CommandList::addDrawMeshIndexed(PoolId meshId_, uint32_t instanceCount_, uint32_t firstInstance_)
	{
		this->order.push_back({CMD_DRAW_MESH_INDEXED, static_cast<uint32_t>(this->drawMeshIndexedParams.size())});
		this->drawMeshIndexedParams.push_back({meshId_, instanceCount_, firstInstance_});
	}

	void CommandList::addUploadQueuedCopies()
	{
		this->order.push_back({CMD_UPLOAD_QUEUED_COPIES, UninitializedId});
	}

	void CommandList::clear()
	{
		this->order.clear();
		this->bindGraphicsPipelineParams.clear();
		this->bindDescriptorSetParams.clear();
		this->drawMeshIndexedParams.clear();
	}

	/*
		Replays the commands in order through the same Api functions a hand written record function calls, so the draw skipping and descriptor locking of drawMeshIndexed apply unchanged. The Vulkan command buffer is begun and ended here, a list only holds the work between.
		A bind descriptor set command uses the pipeline bound before it, the one drawMeshIndexed reads too, so it needs a bind graphics pipeline command first.
	*/
	void CommandList::record(VkCommandBuffer& cmdBuffer_) const
	{
		beginCommandBuffer(cmdBuffer_);
		for(const CmdEntry& rEntry : this->order)
		{
			switch(rEntry.type)
			{
				case CMD_BEGIN_PRESENTATION_RENDER_PASS:
				{
					beginPresentationRenderPass(cmdBuffer_);
					break;
				}
				case CMD_END_RENDER_PASS:
				{
					endRenderPass(cmdBuffer_);
					break;
				}
				case CMD_BIND_GRAPHICS_PIPELINE:
				{
					bindGraphicsPipeline(this->bindGraphicsPipelineParams[rEntry.paramsId].pipelineId, cmdBuffer_);
					break;
				}
				case CMD_BIND_DESCRIPTOR_SET:
				{
					if(Globals::renderer.recordingPipelineId == UninitializedPoolId)
					{
						throw std::runtime_error("Command List: a bind descriptor set command needs a bind graphics pipeline command before it.");
					}
					const BindDescriptorSetParams& rParams = this->bindDescriptorSetParams[rEntry.paramsId];
					bindDescriptorSet(rParams.descriptorSetId, Globals::renderer.recordingPipelineId, cmdBuffer_, rParams.dynamicUnitIds);
					break;
				}
				case CMD_DRAW_MESH_INDEXED:
				{
					const DrawMeshIndexedParams& rParams = this->drawMeshIndexedParams[rEntry.paramsId];
					drawMeshIndexed(rParams.meshId, rParams.instanceCount, cmdBuffer_, rParams.firstInstance);
					break;
				}
				case CMD_UPLOAD_QUEUED_COPIES:
				{
					uploadQueuedCopiesCMDs(cmdBuffer_);
					break;
				}
			}
		}
		endCommandBuffer(cmdBuffer_);
	}
	/*
		Replays the commands like record does, but only follows what is bound: the pipeline of the last bind graphics pipeline command and the set per set index. Every draw adds the descriptors its vertex and fragment shader declared (Shader::addDescriptor) in the sets bound at their set indices, the same lookup drawMeshIndexed makes while recording.
		PassesGraph::enablePass uses it to hold a ONESHOT pass until all of them are loaded: a draw skipped inside a ONESHOT buffer would be lost for good, since the buffer is recorded and submitted only once. Doesn't throw for an unbound set index, record does that.
	*/
	std::vector<BoundDescriptor> CommandList::collectDescriptorUses() const
	{
		std::vector<BoundDescriptor> uses;
		PoolId pipelineId = UninitializedPoolId;
		std::vector<PoolId> setIds;
		for(const CmdEntry& rEntry : this->order)
		{
			switch(rEntry.type)
			{
				case CMD_BIND_GRAPHICS_PIPELINE:
				{
					pipelineId = this->bindGraphicsPipelineParams[rEntry.paramsId].pipelineId;
					break;
				}
				case CMD_BIND_DESCRIPTOR_SET:
				{
					PoolId setId = this->bindDescriptorSetParams[rEntry.paramsId].descriptorSetId;
					uint32_t setIndex = getDescriptorsManager().layouts[getDescriptorSet(setId).setLayout].setIndex;
					if(setIds.size() <= setIndex)
					{
						setIds.resize(setIndex + 1, UninitializedPoolId);
					}
					setIds[setIndex] = setId;
					break;
				}
				case CMD_DRAW_MESH_INDEXED:
				{
					if(pipelineId == UninitializedPoolId)
					{
						break;
					}
					GraphicsPipeline& rPipeline = getGraphicsPipeline(pipelineId);
					for(PoolId shaderId : {rPipeline.vertexShaderId, rPipeline.fragmentShaderId})
					{
						for(const ShaderDescriptorUse& rUse : getShader(shaderId).descriptors)
						{
							if(rUse.setIndex < setIds.size() && setIds[rUse.setIndex] != UninitializedPoolId)
							{
								uses.push_back({setIds[rUse.setIndex], rUse.descriptorId});
							}
						}
					}
					break;
				}
				default:
				{
					break;
				}
			}
		}
		return uses;
	}
}
