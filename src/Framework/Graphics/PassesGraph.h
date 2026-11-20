#pragma once
#include "PassId.h"
#include "PoolMap.h"
#include "PassesPack.h"
#include "SubmissionBatch.h"
#include "CmdBuffers.h"
#include "Task.h"
#include "Utilities.h"
#include "Descriptor.h"
#include "MemoryEntryId.h"

namespace Graphics
{

/* One submit of a ONESHOT pass that uses meshes or descriptors, kept until its fence signaled (PassesGraph::oneShotsInFlight). */
struct OneShotFlight
{
    PoolId fenceId;
    std::vector<PoolId> meshIds;
    std::vector<BoundDescriptor> descriptors;
    std::vector<MemoryEntryId> entryIds; // the entries (copy 0) it reads: the vertex entry of its meshes and the entry of its descriptors, what waitForEntryUse looks for
};

struct PassesGraph 
{
    std::array<PassesPack, 2> passesPerCmdType; 
	std::vector<SubmissionBatch> submissionBatches;
    std::vector<PassId> passesOrder;
    uint32_t enabledPassesCount = 0; 
    std::vector<PassId> heldPasses; // ONESHOT passes enablePass was called for, waiting for the descriptors their command lists use, see releaseHeldPasses
    
    std::vector<OneShotFlight> oneShotsInFlight; // submitted ONESHOT passes that use meshes or descriptors and are not known to be finished, see waitForUse
    std::vector<PoolId> freeOneShotFenceIds; // tracking fences that were reset and can be used by the next submit
    
    bool orderDirty = false; 
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, PoolId signalFenceId_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_);
    PassId addPass(const char* name_, CmdLifetimeEnum cmdType_, QueueFamilyEnum queueFamily_, SyncRetrivalFunc singalFenceFunc_);

    Pass& getPass(PassId passId_);
    uint32_t addTaskToPass(PassId passId_, const char* name_, PoolId commandListId_); // the task is recorded from that CommandList
    void setTaskCommandList(PassId passId_, uint32_t taskId_, PoolId commandListId_); // Takes effect at the next recording, not while recording
    void registerTaskCmdBuffers(PassId passId_, uint32_t taskId_); // adds the task's command buffers to the pools (not allocated yet)
    void record(const CmdRecorder& rRecorder_, VkCommandBuffer& cmdBuffer_);
    void addDynamicWaitSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_, VkPipelineStageFlags pipelineStage_); 
    void addDynamicSignalSemaphoreToTask(PassId passId_, uint32_t taskId_, SyncRetrivalFunc semaphoreRetrivalFunc_);
    // uint32_t addWaitFence();
    // uint32_t addFenceToReset();
    
    void enablePass(PassId passId_); // a ONESHOT pass whose command lists use meshes or descriptors that are not loaded and settled yet is held back (heldPasses) instead
    void releaseHeldPasses(); // enablePass again for every held pass, the ones that are ready get enabled. Renderer::draw calls it after DescriptorsManager::updateLoading and ResourceManager::updateLoading
    bool areResourcesReady(PassId passId_); // marks the meshes and descriptors the command lists of the pass's tasks use as used, true when all are loaded and settled
    void disableFramePass(PassId passId_); // Since oneshot - self disables

    void waitForFrame(uint32_t frameIndex_);
    void trackOneShot(const SubmissionBatch& rBatch_); // after the submit of a ONESHOT batch: records what it uses with a tracking fence, nothing when it uses no mesh or descriptor
    void retireOneShots(); // forgets the in flight ONESHOT submits whose fence signaled and recycles the fences. Renderer::draw calls it after waitForFrame
    void waitForUse(uint64_t lastUsedFrame_); // blocks until the FRAME submit of that frame number finished, no wait when it is older than the frames in flight (those slots were waited on already) or NeverUsedFrame
    void waitForMeshUse(PoolId meshId_); // everything that submitted work using the mesh finished: its last FRAME use (Residency::lastUsedFrame) and the in flight ONESHOT submits that use it. Replaces a vkDeviceWaitIdle before an unload
    void waitForEntryUse(const MemoryEntryId& rEntryId_); // the in flight ONESHOT submits that read the entry finished (any copy of it). A host write into a CPU_SHARED copy has to wait for them, waitForFrame only covers the FRAME passes
    void waitForOneShots(); // every in flight ONESHOT submit finished, for the writes that don't know which entry they touch
    void waitForDescriptorUse(PoolId setId_, PoolId descriptorId_); // the same for a descriptor, before it is unloaded or its binding is rewritten by a load
    void waitForOneShotFlight(const OneShotFlight& rFlight_);
    void compileIfDirty();
    void resolveDynamicSync(SubmissionBatch& rSubmissionBatch_, SubmissionMetadata& rMetadata); 
    void resolveSync_SubmitToGPU();     

    // void resolveDynamicSync(SubmissionBatch& rSubmissionBatch_); 
    // void resolveSync_SubmitToGPU();     
};
}