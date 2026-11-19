#pragma once
#include "PoolId.h"
#include "Utilities.h"
#include <cstdint>
#include <vector>

namespace Graphics
{
	/* Every command a CommandList can record. A command with parameters has a params struct below and a vector of them in CommandList; the ones without parameters carry no id. */
	enum CmdTypeEnum
	{
		CMD_BEGIN_PRESENTATION_RENDER_PASS,
		CMD_END_RENDER_PASS,
		CMD_BIND_GRAPHICS_PIPELINE,
		CMD_BIND_DESCRIPTOR_SET,
		CMD_DRAW_MESH_INDEXED,
	CMD_UPLOAD_QUEUED_COPIES
	};

	struct BindGraphicsPipelineParams
	{
		PoolId pipelineId = UninitializedPoolId;
	};

	/* Binds at the set index of the set's layout, with the graphics pipeline bound before it (Renderer::recordingPipelineId). dynamicUnitIds is what bindDescriptorSet takes: one unit id per dynamic descriptor, empty = unit 0. */
	struct BindDescriptorSetParams
	{
		PoolId descriptorSetId = UninitializedPoolId;
		std::vector<uint32_t> dynamicUnitIds;
	};

	struct DrawMeshIndexedParams
	{
		PoolId meshId = UninitializedPoolId;
		uint32_t instanceCount = 1;
		uint32_t firstInstance = 0;
	};
}
