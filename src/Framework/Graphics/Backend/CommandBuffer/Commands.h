#pragma once
#include "PoolId.h"
#include "PushConstantLayout.h"
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
	CMD_UPLOAD_QUEUED_COPIES,
		CMD_SET_PUSH_CONSTANT_MEMBER_VALUE
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

	/*
		Pushes one variable (or an element, row or column of a matrix variable) of a push constant layout (vkCmdPushConstants) with the pipeline bound before it, see PushConstantWrite for what it holds.
		A command added with a pointer (cached false) keeps a non owning pointer to the application's variable: the value is read every time the command is recorded, so the application keeps the variable alive and at the same address while the list is replayed.
		A command added with a value (cached true) took the bytes when it was added (cachedBytes, one run per range to push) and never reads anything again, pWrite.pData is null.
	*/
	struct SetPushConstantMemberValueParams
	{
		PoolId pushConstantLayoutId = UninitializedPoolId;
		PushConstantWrite write;
		bool cached = false;
		std::vector<PushConstantBytes> cachedBytes;
	};
}
