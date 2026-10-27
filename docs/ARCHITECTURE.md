# Architecture

Describes what the code currently does. Memory management (`Graphics/MemoryManagment/`)
is work in progress; parts of it are not wired up yet.

## Build layout

- Single CMake executable target `ChilliSauceGF` (C++20). Every `.cpp` is listed
  explicitly in `add_executable` in `CMakeLists.txt`.
- Headers are included by bare filename (`#include "Buffer.h"`). This works because
  each source directory is listed in `target_include_directories`. A new `.cpp`
  must be added to `add_executable`; a new directory must be added to the include
  directories.
- Dependencies: Vulkan (`find_package`), GLFW (prebuilt static lib in
  `third-party/GLFW`), GLM (header-only in `third-party/glm`).
- `ENGINE_DEBUG` is defined for the Debug config. It enables validation layers
  and debug-only checks.
- Executable goes to `build/bin/`. Shaders live in `build/bin/Assets/shaders/`
  (SPIR-V compiled by `compile.sh` there) and are loaded relative to the
  executable via `Disk::executablePath`.
- Not compiled: `src/Demo/Managers/`, `src/Demo/Assets/`,
  `src/Framework/Graphics/Backend/DescriptorSet disabled/`.

## Directories

```
src/Demo/                     Demo app: main.cpp, Triangle (example GPU scene)
src/Framework/
  Core                        Window (GLFW), main loop, resize callback
  Api.h/.cpp                  Public free-function API (namespace Graphics)
  Globals.h                   Global state: window, settings, the Renderer instance
  Parameters.h                Tunables (device extensions, chunk sizes)
  Utilities.h/.cpp            Shared enums, lookup tables, helper functions, Disk::
  Misc/                       Pool, PoolMap, PoolId, Bool, Matrix
  Graphics/
    Renderer                  Owns everything GPU-side; setup/draw/shutdown
    *Manager, GPUScene,       Containers grouping resources of one kind
    PassesGraph, SyncManager,
    ReflectionSystem
    FrameResources            Per-frame-in-flight cmd pools + sync objects
    Backend/Base/             Thin Vulkan object wrappers (Instance, Device, Swapchain,
                              Buffer, MemoryBlock, RenderPass, SubPass, Framebuffer,
                              GraphicsPipeline, PipelineLayout, Shader, Surface)
    Backend/CommandBuffer/    FrameCommandPool, OneShotCommandPool, CmdPoolsPack
    Backend/Synchronisation/  Fence, Semaphore
    Backend/Image/            Image (+ views), Sampler
    Backend/ValidationLayers/ DebugMessenger, layer/extension lists
    Pass/                     Pass, Task, PassId, SubmissionBatch, SubmissionMetadata
    Reflection/               ReflectionLayout, MemberBlueprint, CPUDataContainer,
                              GPUDataContainer, DataContainerConcepts
    Resources/                Mesh, Vertex, Texture
    MemoryManagment/          Static/Dynamic allocators, Heaps/, Pages/ (WIP)
```

## Global access and the API

- `Globals::renderer` (`Graphics::Renderer`) is a single global that owns all
  subsystems as value members: `instance`, `mainDevice`, `surface`, `swapchain`,
  `memoryManager`, `syncManager`, `shadersManager`, `resourcesManager`,
  `pipelinesManager`, `reflectionSystem`, `passesGraph`, `gpuSceneManager`,
  `framesResources`, `oneShotCommandPools`, `presentationRenderPass`.
- `Api.h` declares free functions in `namespace Graphics` (`getX`, `addX`,
  `removeX`, `setX`, command-recording helpers). Each is a thin forwarder into
  `Globals::renderer`. Framework code also calls these internally (e.g.
  `getMainDevice().logicalDevice` everywhere a `VkDevice` is needed).
- Demo code talks to the framework only through `Api.h`.

## Startup, frame loop, shutdown

1. `main` calls `Triangle::setGPUSceneToTriangle()`, which registers function
   pointers on `GPUSceneManager` (`defineLayouts`, `defineResources`,
   `definePasses`, and per-frame `update*`).
2. `Core::setup` creates the GLFW window, then `Renderer::setup`: instance →
   surface → device → swapchain (sets `framesAtFlightCount` = swapchain image
   count) → `FrameResources` → `MemoryManager::setup` → presentation render pass →
   framebuffers → GPUSceneManager `define*` (then `changed` is reset) → static allocator upload → pipelines →
   command pools (allocated from tasks registered by `define*`).
3. `Renderer::draw` per frame (skipped while `Globals::resizing`):
   if `GPUSceneManager::changed`, run `define*` again and reset it → resolve
   pending memory ops → GPUSceneManager `update*` → wait + reset the frame's
   `frameAvailableFence` → `vkAcquireNextImageKHR` (signals the frame's
   `imageAcquiredSemaphore`) → re-record enabled command buffers →
   `passesGraph.compileIfDirty()` → `resolveSync_SubmitToGPU()` →
   `presentToScreen()` (waits on the swapchain image's
   `imageUseFinishedSemaphore`) → advance `currentFrame`.
4. `Renderer::shutdown` calls `vkDeviceWaitIdle`, then destroys in reverse
   dependency order (sync → frame resources → one-shot pools → pipelines →
   render passes → framebuffers → shaders → memory → swapchain → device →
   surface → instance).
5. Window resize (`Core::windowSizeCallback`): `vkDeviceWaitIdle`, destroy and
   recreate swapchain + framebuffers.

Frames vs images: per-frame-in-flight objects live in `FrameResources`
(indexed by `currentFrame`); per-swapchain-image objects live on the swapchain
`Image` (indexed by `imageIndex`). Framebuffers are indexed by `imageIndex`.

## Queues

`QueueFamilyEnum` = `GRAPHICS(0)`, `TRANSFER(1)`, `COMPUTE(2)`.
`Device::queues` and `QueueFamilyIndices::indices` have 4 slots; slot
`PresentationQueueId` (3) is the present queue. `CmdPoolsPack` holds one command
pool per queue family (3).

## Passes, tasks, submission

- `Pass` = named group of `Task`s on one queue family with a `CmdLifetimeEnum`:
  - `FRAME`: one command buffer per frame-in-flight; the frame's pool is reset
    (`vkResetCommandPool`) and enabled buffers re-recorded every frame.
  - `ONESHOT`: one command buffer; pool created with
    `RESET_COMMAND_BUFFER_BIT`; recorded once when enabled, then the pass
    disables itself after submission.
- `Task` = one `CmdBufferFunc` (`void(*)(VkCommandBuffer&)`) that records one
  command buffer, plus wait/signal semaphores.
- Passes are stored in `PassesGraph::passesPerCmdType[lifetime].passesPerQueue[queue]`
  (plain vectors, not pools) and addressed by `PassId {cmdLifetime, queueFamily, id}`.
  `passesOrder` keeps insertion order, which is submission order.
- `enablePass` / `disableFramePass` mark the graph dirty; `compileIfDirty`
  rebuilds `SubmissionBatch`es (one `vkQueueSubmit` per enabled pass, one
  `VkSubmitInfo` per task).
- Static sync = `PoolId` of a semaphore/fence resolved at compile time.
  Dynamic sync = a `SyncRetrivalFunc` (`PoolId(*)()`) called at every submit
  (e.g. `getCurrentFrameImageIsAcquiredSemaphore`), for objects that change per
  frame or per image.

## Ownership and lifetime

- No `new`/`delete` or smart pointers. Everything is owned by value: as a
  `Renderer` member, in a `std::vector`/`std::array`, or in a `Pool`/`PoolMap`.
- Cross-object references are `PoolId` handles (`{id, generation}`), plus
  `PassId` and `MemoryEntryId {allocatorType, PoolId}`. Raw pointers appear only
  as non-owning back-references (e.g. `GraphicsPipeline::pRenderPass`).
- `Pool<T>` / `PoolMap<T>` (`Misc/`): slot vectors with generation counters and a
  free list. `PoolMap` also maps names → ids and its `add` takes a name first.
  `remove`/`clear` call `T::destroy()` if `T` has one. Under `ENGINE_DEBUG`,
  `operator[]` validates the id (uninitialized, dead slot, stale generation) and
  throws. `getInternal(index)` skips validation.
- Vulkan wrappers hold `vkHandle` (init `VK_NULL_HANDLE`), expose `get()`, and
  have explicit `create()`/`destroy()` (sometimes `setup()`/`init()`).
  `destroy()` checks for `VK_NULL_HANDLE`, destroys, and resets the handle, so it
  is safe to call twice.
- `Buffer`, `MemoryBlock` and `StagingHeap` have no destructor: whoever owns
  them must call `destroy()` (directly, or through `Pool::remove`/`clear`).
  `Shader` still destroys in its destructor and has move ops and deleted copy.
- Sync objects are created through `SyncManager` (`addFence(createSignaled)`,
  `addSemaphore()`), returning `PoolId`s; `getFence`/`getSemaphore` return the
  raw `VkFence&`/`VkSemaphore&`.
- Swapchain images are borrowed (`Image::setImage`); only their views are
  destroyed by us.

## Memory management (WIP)

- `MemoryManager` owns a `StaticAllocator` and a `DynamicAllocator`, plus
  a `StagingAllocator` (update staging) and a queue of pending `UPDATE`/`REMOVE` operations that
  `resolvePendingOperations()` applies each frame once the entry's upload fence
  is signaled.
- `StaticAllocator`: entries are added before allocation (throws if already
  allocated). `allocateAndUpload()` creates one `GPULocalHeap` (one memory block,
  one `VkBuffer` per `BufferTypeEnum`, bound at aligned offsets) and one
  `CPUSharedHeap`, then uploads every entry through `MemoryEntry::upload`:
  `CPU_SHARED` entries are written into the mapped heap, `GPU_LOCAL` entries get
  their own `StagingAllocator` range and a queued copy in the normal upload batch
  (no dedicated pass, fence or semaphore). An entry bigger than the biggest
  staging page throws. An entry added with a `nullptr` data pointer (`MemoryManager::allocateEntry`) only gets its
  place in the heap and is skipped by the upload loop.
- `DynamicAllocator`: pages configured via `addPage` before `init()`, sorted by
  upper-bound entry size. Each page (`BasePage` → `GPULocalPage` /
  `CPUSharedPage`) owns a `Pool<MemoryBlock>`, per-block `FreeSpace` interval
  lists, and one `VkBuffer` per entry bound into a block. `BasePage::addBuffer`
  creates the real buffer first and places it using its own
  `memoryReqs.size`/`alignment`, only through free intervals (inclusive
  `[start, end]`); a new block starts as one interval covering the whole block,
  and `MemoryBlock::freeSpace` is the total of free bytes in the block.
  `removeBuffer` frees the same extent, merges it with touching intervals,
  destroys the buffer, and removes the block when it is completely free.
- Initial uploads of `GPU_LOCAL` dynamic entries (`DynamicAllocator::reserveEntry`, used
  by `addAndUploadEntry`) take an open `StagingAllocator` range sized to the entry
  (`stagingData.inEntryOffset` is 0), then `MemoryEntry::upload` writes it and queues the copy.
- `StagingAllocator` serves every staging range: static init, dynamic init,
  updates and readbacks. It has one list of pages per `StagingDirectionEnum`
  (`UPLOAD`, `READBACK`). Pages are configured via `addPageForStagingUploads` /
  `addPageForStagingReadbacks` before `init()` (`StagingPageInfo`), sorted by the
  upper bound of the range size. The first page with `size <= upperBound` serves a
  range, otherwise it throws. A `StagingPage` owns `StagingHeap`s with one
  `FreeSpace` per heap; a range is placed first-fit in an alive heap or in a new
  heap (`StagingData::stagingPageId` identifies the page). A new range is open
  (`OpenBatchId`) and is never freed; `MemoryEntry::queueStagedCopy` tags it with the
  batch id `queueCopy` returns (`StagingAllocator::tagBatch`). `updateUploadBatches()` calls
  `releaseBatch(batchId)` for each slot whose fence was observed signaled, which frees the ranges tagged with that
  id (all live ranges are checked, since batch ids are stamped later and are not sorted)
  (merging intervals) and destroys dormant heaps above the page's
  `maxDormantHeaps`, oldest first.
- Readbacks (`READBACK` direction): `READBACK` heaps are `TRANSFER_DST` buffers in
  `HOST_VISIBLE | HOST_CACHED` memory; `StagingHeap::create` prefers a type that is also
  `HOST_COHERENT` and falls back to plain cached (`MemoryBlock::isCoherent` records which one
  was picked). Range sizes are always rounded up to `nonCoherentAtomSize`. `MemoryManager::requestReadback` returns a `ReadbackId` (a
  `Pool<ReadbackRequest>` handle). For `GPU_LOCAL` entries it allocates a readback range and
  queues a `READBACK_COPY` operation, so the copy is ordered after earlier `UPDATE`s and waits
  while the entry has an upload in flight; `resolvePendingOperations` then queues a
  `PendingCopy` with `isReadback` (entry buffer -> readback range) into the same batched
  `ONESHOT` `TRANSFER` pass. `CPU_SHARED` entries need no copy and are ready immediately; their readback is a live view of the mapped memory (no snapshot), so it shows whatever was written when it is read.
  `isReadbackReady` is `isBatchCompleted(batchId)`. `getReadbackContainer` returns a
  readable `ReadbackDataContainer` (`GPUDataContainer<true>` over the readback range; `MemoryManager::readStaging`
  invalidates the range first when the heap is not coherent; for `CPU_SHARED` it is the entry's
  `SharedDataContainer`). `releaseReadback` frees the range (only when ready); readback ranges
  are never freed by `releaseBatch`. A queued readback copy reads the entry's buffer, so `queueReadbackCopy`
  stores its batch in `MemoryEntry::readbackBatchId` and a pending `UPDATE` or `REMOVE` also waits for `isPendingReadback()`
  (a readback requested before an update sees the bytes from before it; the order is decided on the CPU, the barriers below are a second guard). `uploadQueuedCopiesCMDs` records a `TRANSFER ->
  TRANSFER` barrier at the start of every batch (orders it after earlier batches; with one batch in flight the CPU already guarantees that) and where the copy kind switches (a readback read before an update write in one
  batch) and a final `TRANSFER_WRITE -> HOST_READ` barrier when the batch has readbacks.
  GPU_LOCAL buffers have `TRANSFER_SRC`.
- `Api.h` routes `addAndUploadMemoryEntry`, `updateMemoryEntry`, `removeUpload`,
  `getMemoryEntry` and `isUploadInGPU` to `MemoryManager::addEntry`,
  `updateEntry`, `removeDynamicEntry`, `getEntry` and `isUploadInGPU`;
  `allocateMemoryEntry` (size = layout size × repeat count, layout looked up in the `bufferType_` list of `ReflectionSystem`)
  and `allocateMemoryEntryRaw` (byte size, e.g. index buffers), both with an `allocatorType_`, to `MemoryManager::allocateEntry`;
  `requestMemoryEntryReadback`, `isReadbackReady`, `mapReadback` and `releaseReadback` to the
  readback functions above.
- Host-visible memory (`CPU_SHARED` blocks and all staging heaps) is mapped once when the `MemoryBlock` is created
  (`MemoryBlock::pMapped`) and stays mapped until `destroy()`. Nothing else calls `vkMapMemory` (it is invalid on
  already-mapped memory). `MemoryManager` is the only place that copies bytes in or out: `writeStaging` / `readStaging`
  (a `StagingData` range) and `writeShared` / `readShared` (a `CPU_SHARED` entry, via `getSharedPointer`). Offsets are
  entry byte offsets. `MemoryEntry::upload` and `GPUDataContainer` both go through them.
- `MemoryEntry` holds both static and dynamic fields.
- `MemoryManager::getBuffer(entry)` returns the `Buffer` an entry lives in: the
  page buffer for dynamic entries (whole buffer), or the static heap buffer for
  the entry's type and visibility (`StaticAllocator::getBuffer`; add
  `inBufferFirstByte`). `drawMeshIndexed` and `MemoryEntry::upload` use it.
- `MemoryManager::updateEntry` writes every `GPU_LOCAL` update (static or dynamic)
  into a fresh open staging range immediately and queues an `UPDATE` operation
  (`UpdateRequest` = entry id + `StagingData`); `resolvePendingOperations` queues the
  copy once the entry has no upload in flight. `CPU_SHARED` entries are written
  immediately.
- Write-through API (`MemoryManager`): `getStagingContainer(entryId, inEntryOffset,
  size)` returns a per-update `StagingDataContainer` over a new range (or the
  whole-entry range of a reserved, uncommitted entry); `getSharedContainer(entryId)`
  returns the persistent `SharedDataContainer` of a `CPU_SHARED` entry. Both map
  entry byte offsets. `reserveEntry` creates a dynamic entry without data (pending
  until `commitEntry` queues its copy). Static entries can't be reserved.
  `allocateEntry` also creates an entry without data, but takes no staging range and needs no commit:
  nothing is pending and later writes are normal updates (`GPU_LOCAL` bytes are undefined until the first update
  was copied). A dynamic entry's buffer exists right away. A static entry must be allocated before
  `StaticAllocator::allocateAndUpload` (same rule as `addEntry`), and its buffer exists only after that call.
- `GPU_ONLY` entry updates are batched by `MemoryManager`. `MemoryEntry::upload`
  writes the staging heap and calls `MemoryManager::queueCopy`, which stores a
  `PendingCopy` in the default upload slot and returns a batch id kept in `MemoryEntry::uploadBatchId`.
  A slot (`UploadBatch`) owns a fence, a `ONESHOT` `TRANSFER` pass, its command buffer and a
  queued/recorded copy list. `MemoryManager::setup` creates one slot (`DefaultUploadSlotId`), which takes every
  queued copy (initial uploads, `commitEntry`, deferred `UPDATE`/`READBACK_COPY`). A slot is free once its fence
  was observed signaled, so only one batch is in flight at a time and copies queue up behind it. `updateUploadBatches()` (called at the start and end of
  `resolvePendingOperations()`) marks a slot done when its fence is signaled, then starts the default slot when it is
  not in flight and has queued copies: it moves `queuedCopies` to `recordedCopies`, resets the fence and
  enables the pass. `uploadQueuedCopiesCMDs` finds its slot by command buffer handle and records
  one `vkCmdCopyBuffer` per copy. An entry is pending until `isBatchCompleted(uploadBatchId)`: no slot
  still carries that id (completion is per batch, not a watermark). No semaphore is used.
- `MemoryVisabilityEnum`: `GPU_ONLY` (device-local, uploaded through staging)
  and `CPU_SHARED` (host-visible|coherent, written with map/memcpy/unmap).

## Reflection

`ReflectionSystem::layoutsPerBufferType[bufferType - 1]` (no INDEX layout) holds
`ReflectionLayout`s. A layout is a pure blueprint: `MemberBlueprint`s (type, offset,
size) and the total `size`; it holds no data. A `CPUDataContainer` (byte vector ×
`repeatCount`, e.g. vertices) is owned by the instance that uses it.
`ReflectionLayout::setMember<T>` / `getMember<T>` take the container, a member id and
a repeat unit id, check `T` against the member's type through `GPUTypeMap<T>`, and
throw on mismatch or out-of-range repeat index. They are templated on the container
type, constrained by `WritableDataContainer` / `ReadableDataContainer`
(`DataContainerConcepts.h`): `repeatCount` plus `write`/`read(offset, ptr, size)`.
`GPUDataContainer<Readable>` owns no bytes and touches no Vulkan memory: it holds ids (a `StagingData` range, or a
`MemoryEntryId` when `isShared`) and forwards each write/read to `MemoryManager`
(`StagingDataContainer`, write-only, is a per-update handle; `SharedDataContainer` is for `CPU_SHARED` entries). Under
`ENGINE_DEBUG` a write/read outside `validFirstOffset`/`validSize` throws. `Mesh`
keeps no copy of its vertex bytes; it writes through these containers.

### Mesh updates

- `Mesh::queueUpload(allocatorType, visibility)` reserves the vertex entry
  (`MemoryManager::reserveEntry`, no data yet) and uploads the index entry
  (`vbMemoryUploadId`, `ibMemoryUploadId`).
- `Mesh::setVerticeMember<T>(memberId, verticeId, data)` computes
  `verticeId * layout.size + member.firstByteId`, gets a container for that range
  and writes through `ReflectionLayout::setMember`. For `GPU_LOCAL` the container is a
  `StagingDataContainer` from `MemoryManager::getStagingContainer`; for `CPU_SHARED`
  it is the entry's `SharedDataContainer`. It throws under `ENGINE_DEBUG` if the mesh
  was never uploaded.
- Before `Mesh::commitUpload()` the `GPU_LOCAL` writes fill the entry's whole-entry
  upload staging range, so the initial data goes straight to staging.
  `commitUpload()` calls `MemoryManager::commitEntry`, which queues the copy. After it,
  every `setVerticeMember` is an update: a fresh staging range sized to the member, and
  an `UPDATE` operation.
- There is no dirty tracking (`DirtyInMesh`, `verticeToDirtyId`, `markVerticeDirty`,
  `queueUpdate` were removed): each write has its own range, so there is nothing to merge.
- Reading: `Mesh::requestVerticeMemberReadback(memberId, verticeId)` calls
  `requestMemoryEntryReadback` for the member's byte range and returns a `ReadbackId`. Once
  `isReadbackReady` is true, `Mesh::getVerticeMember<T>(readbackId, memberId, verticeId)` maps it
  (`mapReadback`) and reads through `ReflectionLayout::getMember`. `GPU_LOCAL` meshes copy into a
  readback range, `CPU_SHARED` meshes are ready immediately and read the mapped memory. The caller
  calls `releaseReadback` afterwards. There is no synchronous `getVertice`.

## Error handling

- Failures throw `std::runtime_error`, usually with `std::format` and a
  component prefix: `"{} Pool: ..."`, `"Static Allocator: ..."`,
  `"Dynamic Allocator: ..."`. `main` catches `std::runtime_error`, prints
  `what()` to `stderr`, returns 1.
- `VkResult` is checked (throw on `!= VK_SUCCESS`) for object creation calls
  (`vkCreate*`, `vkAllocateCommandBuffers`) and `vkQueuePresentKHR`. Other calls
  (`vkQueueSubmit`, `vkAcquireNextImageKHR`, `vkMapMemory`, `vkBindBufferMemory`,
  begin/end command buffer) are not checked.
- Misuse checks (pool id validity, allocator state, page config) are wrapped in
  `#ifdef ENGINE_DEBUG` and compiled out in release.
- Validation layers + debug messenger are enabled only under `ENGINE_DEBUG`.

## Naming and style

- `struct` for all types (`class` only in `Misc/Matrix.h`); public members by
  default, `private:` used rarely.
- Types `PascalCase`; functions, members, locals `camelCase`.
- Function parameters end with `_` (`size_`, `rRenderpass_`).
- Reference variables/params prefixed `r` (`rEntry`, `rPass`); pointers prefixed
  `p` (`pRenderPass`, `pCurrentFrameAvailable`).
- Members always accessed through `this->`.
- Ids: `xxxId` (`PoolId meshId`, `PassId uploadPassId`, `uint32_t taskId`).
  Invalid id sentinels: `UninitializedId`, `UninitializedPoolId`.
- Enums are unscoped, named `XxxEnum`, values `UPPER_CASE` (`GRAPHICS`, `FRAME`,
  `GPU_LOCAL`, `VERTEX`). Lookup tables named `xToY` (`BufferTypeToUsage`,
  `dataTypeToName`); counts `XxxCount` (`BufferTypesCount`).
- Vulkan create-info structs zero-initialized with `= {}` and filled field by
  field.
- `#pragma once`; everything framework-side in `namespace Graphics` (except
  `Globals`, `Disk`, `Misc/` containers).
- Tabs, Allman braces.
