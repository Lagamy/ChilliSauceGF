# Optional Polish Notes

Ideas that are not needed for the current demo. Nothing here is implemented.

## Runtime creation of passes and tasks

Status: not supported. `addPass` / `addTaskToPass` only work before the command
pools are created in `Renderer::setup`. Today every call happens there
(`Triangle::definePasses`, `MemoryManager` setup), so nothing breaks.

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
