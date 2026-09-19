#include "UploadEntry.h"
#include "Api.h"
#include "Utilities.h"
#include <limits>
#include <vulkan/vulkan_core.h>



namespace Graphics 
{
    void uploadCPUSharedEntryCMDs(VkCommandBuffer& cmdBuffer_) 
    {
        // StaticAllocator& rStaticAllocator = getMemoryManager().staticAllocator; 
	    // #ifdef ENGINE_DEBUG
		//     if(!rStaticAllocator.allocated)
		//     {
		// 	    throw std::runtime_error("Static Allocator: Can't record CMD Buffer for unallocated memory.\n");
		//     }
	    // #endif

	    // size_t memoryBlockOffset = 0; 
	    // // Info to begin the command buffer record 
	    // VkCommandBufferBeginInfo beginInfo = {};
	    // beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	    // beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We are only using this command buffer once. So setup for 1 time submit. 	
	    // vkBeginCommandBuffer(cmdBuffer_, &beginInfo);

	
	    // void* pCPUSharedMemPoint; // Create an empty typeless pointer.
	    // for(uint32_t i = 0; i < rStaticAllocator.uploadEntryGroupPerMemVisability[CPU_SHARED].entriesPerBufferType.size(); i++)
	    // {

		//     std::vector<UploadEntry> entriesForCurrentBufferType = rStaticAllocator.uploadEntryGroupPerMemVisability[GPU_ONLY].entriesPerBufferType[i];  
		//     for(auto& rUpload : entriesForCurrentBufferType)
		//     {
		// 	    // Since GPUHeap is allocated -> i can now find and save each Uploads first byte position in it. 
		// 	    rUpload.inGPUFirstByte = rStaticAllocator.cpuSharedHeap.bufferOffsets[i] + rUpload.inBufferFirstByte; 
		// 	    vkMapMemory(getMainDevice().logicalDevice, rStaticAllocator.cpuSharedHeap.memoryBlock.get(), memoryBlockOffset + rUpload.inBufferFirstByte, rUpload.size, 0, &pCPUSharedMemPoint);  // Now void* data points to where vertex Buffer is on GPU/Shared Memory in RAM. So we could upload our vertex data to it. This is called Mapping.
		// 	    memcpy(pCPUSharedMemPoint, static_cast<const char*>(rUpload.data), rUpload.size);  // writes to *Staging/Shared memory in Ram* via CPU pointer
		// 	    vkUnmapMemory(getMainDevice().logicalDevice, rStaticAllocator.stagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
		//     }
	    // }

	    // for(uint32_t i = 0; i < rStaticAllocator.uploadEntryGroupPerMemVisability[GPU_ONLY].entriesPerBufferType.size(); i++)
	    // {
		//     std::vector<UploadEntry> entriesForCurrentBufferType = rStaticAllocator.uploadEntryGroupPerMemVisability[GPU_ONLY].entriesPerBufferType[i];  
		//     for(auto& rUpload : entriesForCurrentBufferType)
		//     {
		// 	    // Since GPUHeap is allocated -> i can now find and save each Uploads first byte position in it. 
		// 	    rUpload.inGPUFirstByte = rStaticAllocator.gpuHeap.bufferOffsets[i] + rUpload.inBufferFirstByte; 
		// 	    vkMapMemory(getMainDevice().logicalDevice, rStaticAllocator.stagingHeap.memoryBlock.get(), memoryBlockOffset + rUpload.inBufferFirstByte, rUpload.size, 0, &pCPUSharedMemPoint);
		// 	    memcpy(pCPUSharedMemPoint, static_cast<const char*>(rUpload.data), rUpload.size);  // writes to *Staging/Shared memory in Ram* via CPU pointer
		// 	    vkUnmapMemory(getMainDevice().logicalDevice, rStaticAllocator.stagingHeap.memoryBlock.get()); // Unmap vertexBufferMemory from data
		//     }

		//     // Region of data to copy from and to 
		//     VkBufferCopy bufferCopyRegion = {};
		//     bufferCopyRegion.srcOffset = memoryBlockOffset;
		//     memoryBlockOffset += rStaticAllocator.gpuHeap.bufferSizes[i];		
		//     bufferCopyRegion.dstOffset = 0; // its buffer local offset 
		//     bufferCopyRegion.size = rStaticAllocator.gpuHeap.bufferSizes[i];

		//     // Command to copy from srcBuffer to dstBuffer
		//     vkCmdCopyBuffer(cmdBuffer_, rStaticAllocator.stagingHeap.buffer.get(), rStaticAllocator.gpuHeap.buffers[i].get(), 1, &bufferCopyRegion);
	    // }
	

	    // rStaticAllocator.allocated = true; 
	    // vkEndCommandBuffer(cmdBuffer_);
    }

    void uploadGPULocalEntryCMDs(VkCommandBuffer& cmdBuffer_)
    {

    }

	UploadEntry::UploadEntry(const char* name_, const void* data_, VkDeviceSize size_, BufferTypeEnum bufferType_, VkDeviceSize currentBuffSize_) : name(name_), data(data_), size(size_), bufferType(bufferType_), inBufferFirstByte(currentBuffSize_) {}; 
    UploadEntry::UploadEntry(const char* name_, VkDeviceSize size_,  BufferTypeEnum bufferType_, MemoryVisabilityEnum memoryVisability_, uint32_t pageId_, PoolId bufferId_) : name(name_), size(size_), bufferType(bufferType_), pageId(pageId_), bufferId(bufferId_) 
    {
        this->isUploadedFenceId = addFence(true); 
        this->isUploadedSemaphoreId = addSemaphore(); 
        this->isForDynamic = true; 
        this->memoryVisability = memoryVisability_; 
    };

    UploadEntry::~UploadEntry()
    {
		vkWaitForFences(getMainDevice().logicalDevice, 1, &getFence(this->isUploadedFenceId), VK_TRUE, std::numeric_limits<uint64_t>::max()); 
        removeFence(this->isUploadedFenceId);
        removeSemaphore(this->isUploadedSemaphoreId);
    }
    
    bool UploadEntry::isPendingUpload()
    {
        VkResult result = vkGetFenceStatus(getMainDevice().logicalDevice, getFence(this->isUploadedFenceId));
        return result == VK_SUCCESS ? false : true;
    }

    void UploadEntry::upload(const void* data_, uint64_t size_, uint64_t offset_)
    {
	    this->uploadPassId = addPass("Entry Upload", ONESHOT,TRANSFER,this->isUploadedFenceId);
	    uint32_t taskId; 
        if(this->memoryVisability == CPU_SHARED)
        {
            taskId = addTaskToPass(this->uploadPassId, "Dynamic Upload", uploadCPUSharedEntryCMDs); 
        }
        else
        {
            taskId = addTaskToPass(this->uploadPassId, "Dynamic Upload", uploadGPULocalEntryCMDs); 
        }

        addSignalSemaphoreToTask(this->uploadPassId, taskId, this->isUploadedSemaphoreId);
    }
}