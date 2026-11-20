# Optional Polish Notes

Ideas that are not needed for the current demo. Nothing here is implemented.

## Runtime creation of passes and tasks

Status: not supported. `addPass` / `addTaskToPass` only work before the command
pools are created in `Renderer::setup`. Today every call happens there
(`Demo::definePasses`, `MemoryManager` setup), so nothing breaks.

### What goes wrong after setup

- `PassesGraph::addPass` / `addTaskToPass` work on the CPU side: the pass and task are
  stored, `commandsToRecord` and `buffersToEnabled` grow for each frame in flight,
  and `enablePass` / `compileIfDirty` pick the pass up.
- The `VkCommandBuffer` handles are allocated once, in `allocateCmdBuffersFromPasses()`
  (`FrameCommandPool` and `OneShotCommandPool`), sized from `commandsToRecord.size()` at
  that moment. A task added later leaves `buffers` shorter than `commandsToRecord`.
- `rerecordEnabledCmdBuffers` and `getCommandBuffer` (at submit) would then index past the
  end of `buffers`. There is no exception, it is out-of-bounds access.
- Calling `allocateCmdBuffersFromPasses()` again is not a fix. It resizes `buffers` and
  allocates the whole count, which overwrites the handles it already has.

### Scene switching

`GPUSceneManager::changeGPUScene` sets `changed`, so `Renderer::draw` re-runs
`definePasses`. That calls `addPass` again and nothing removes the old passes (there is
no `removePass`). The old scene's passes stay enabled next to the new ones, and the
out-of-bounds problem above applies.

### Possible fix (small, additive)

- Pools allocate only the new range: `buffers.size()` .. `commandsToRecord.size()`.
- `addTaskToPass` calls that when the pools already exist (needs a "pools created" flag).
- Optional `removePass`, needed for scene switching: disable the pass, and release or
  reuse its command buffer slots.
- Vulkan rule to confirm before writing: `vkAllocateCommandBuffers` only needs host
  synchronization on the pool, so allocating while buffers from other frames are pending
  should be legal. Check the command pool section of the spec (host synchronization
  of `vkAllocateCommandBuffers`) first.

### Cost, if supported (not measured)

- Registering a pass and task: negligible.
- Allocation: one `vkAllocateCommandBuffers` per frame-in-flight pool, new buffers only.
  No GPU stall as long as pools are not reset or recreated.
- Graph rebuild: one full `compileIfDirty`, same as any `enablePass`.
- Standing cost: one extra re-record and one extra `vkQueueSubmit` per frame for each
  added pass.

## Note on recompiling

`enablePass` on an existing pass (e.g. the memory upload `ONESHOT` pass) sets `orderDirty`,
so the next frame rebuilds every batch with `compileIfDirty`, not just the new one. This
is cheap now (O(enabled passes + tasks)). Making it incremental only matters if
compilation gets more expensive (dependency sorting, barrier generation).

## Descriptor sets bound in ONESHOT passes

Status: partly covered. `removeDescriptorSet` and the layout update commits wait with `vkDeviceWaitIdle`, and the loads and unloads of `updateLoading` wait with `PassesGraph::waitForDescriptorUse` / `waitForMeshUse` (the FRAME frame of the last use plus the tracked `ONESHOT` submits, see ARCHITECTURE.md), so a set bound in a `ONESHOT` pass is safe there, and `PassesGraph::enablePass` holds a `ONESHOT` pass until the descriptors its draws use are loaded and settled.

Covered by the tracked `ONESHOT` submits (`PassesGraph::oneShotsInFlight`): the per-frame `CPU_SHARED` copy-in (`MemoryManager::syncCopiesToFrame`) waits for the in flight submits that read the entry (`PassesGraph::waitForEntryUse`), and the write wait of a `CUSTOM_COPY_COUNT_WITH_WAIT` single buffer waits for all of them (`PassesGraph::waitForOneShots`, the write doesn't know its entry id). Only submits that use meshes or descriptors are tracked, a `ONESHOT` pass that reads an entry some other way (e.g. a future compute pass) is not.

Possible rework, not needed now: the in flight `ONESHOT` lists (`OneShotFlight`) are scanned on every wait and `writeShared` of a `waitsBeforeWrite` entry falls back to `waitForOneShots` (every in flight submit), because it has no entry id the lists could match. Cleaner ownership would be for each `MemoryEntry` to keep `{fenceId, serial}` of the last submit that used it per queue family (the fence of a `FRAME` pass or the tracking fence of a `ONESHOT` one, the graph remembering which serial each fence is armed with, so a reused fence is not waited on for nothing). That would also replace `Renderer::frameNumber` / `lastUsedFrame` and the frame loops of `syncCopiesToFrame` and `writeShared`. The cost is stamping every used entry on every submit and a change across `MemoryEntry`, `PassesGraph`, `SyncManager`, `drawMeshIndexed` and both `markUsed`. Worth it once compute passes (more kinds of `ONESHOT` readers) arrive or a profile shows the scans or over-waits.

Not needed now: `ONESHOT` passes are used for `GPU_LOCAL` transfers. It matters once compute (or any `ONESHOT` pass that binds descriptor sets) is implemented. Then the entry needs to remember the fence of the `ONESHOT` submit that used it and the copy-in has to wait on it too (see also the compute note in `TODO.md`).

## Separate vertex streams

Status: not planned, only an alternative to look at if the vertex layout update cost turns out to hurt.

Vertices are interleaved, so adding a member to a vertex layout changes the stride and every vertex moves. The plan is a GPU to GPU re-stride copy for `GPU_LOCAL` entries (new entry with room for the new members, `headroom` as padding inside the stride so a member that fits needs no copy at all) and a rewrite of the mapped memory for `CPU_SHARED`.

Alternative: separate vertex streams, one buffer per group of members (Vulkan supports several vertex input bindings per pipeline). Adding a member group would then be a new buffer and a new binding, with no re-interleave and no copy of the existing vertices. Removing a group frees its buffer.

Cost: it is a bigger model change and a new abstraction (a mesh owns several vertex entries, the vertex layout is split into stream groups, pipelines get one binding per stream, `Mesh::setVerticeVariable` has to pick the stream of the variable). Fetching from several buffers can also be slower than one interleaved buffer. Only worth it if re-striking big meshes at runtime is measured to be too slow.
