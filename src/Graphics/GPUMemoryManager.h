// Represents resources that are in GPU currently
#pragma once 
#include "CommandBufferBlueprint.h"
#include "GPUMemoryEntry.h"
#include "MemoryBlock.h"
#include "Pool.h"
#include "SyncManager.h"
#include "Image.h"
#include "StaticAllocator.h"
#include "Utilities.h"
#include <vulkan/vulkan_core.h>

namespace Graphics
{
struct UpdateEntry 
{ 
	uint32_t entryId; 
	UploadTypeEnum uploadType; 
	size_t byteAmount = 0; 
	size_t heapStartingByte = 0; 
	size_t dstStartingByte = 0; 

	bool partialUpdate;

	UpdateEntry(uint32_t entryId_, UploadTypeEnum uploadType_, const void* data_, size_t heapStartingByte_); 
	UpdateEntry(uint32_t entryId_, UploadTypeEnum uploadType_, const void* data_, size_t heapStartingByte_, size_t byteAmount_, size_t srcStartingbyte_);
};


struct GPUMemoryManager { 
	StaticAllocator staticAllocator;
	std::vector<UpdateEntry> updateEntries; 
	bool updateNeeded = false;
	void submitTransferCmds();
	uint32_t updateCmdBufferId;
	uint32_t staticUploadCmdBufferId; 
	PoolId updateFinishedSemaphoreId; 
	PoolId updateFinishedFenceId; 
	PoolId staticUploadFinishedSemaphoreId; 
	PoolId staticUploadFinishedFenceId;

	void addUpdate(PoolId entryId_, const void* data_); // full upload
	void addUpdate(PoolId entryId_, const void* data_, size_t byteAmount_, size_t srcStartingbyte_, size_t dstStartingbyte_); // partial upload

	// For device local uploads
	void recordUpdatesCMDs(VkCommandBuffer& cmdBuffer_);
	void submitStaticUploadCmds(); 
	void submitUpdateCmdsIfNeeded();

	void create();
	void destroy();
};
}
