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
    ReflectionSystem,
    DescriptorsManager
    FrameResources            Per-frame-in-flight cmd pools + sync objects
    Backend/Base/             Thin Vulkan object wrappers (Instance, Device, Swapchain,
                              Buffer, MemoryBlock, RenderPass, SubPass, Framebuffer,
                              GraphicsPipeline, PipelineLayout, Shader, Surface)
    Backend/CommandBuffer/    FrameCommandPool, OneShotCommandPool, CmdPoolsPack
    Backend/Synchronisation/  Fence, Semaphore
    Backend/Image/            Image (+ views), Sampler
    Backend/Descriptor/       DescriptorSetLayout, DescriptorBlueprint, DescriptorSet, Descriptor,
                              DescriptorPool
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
  `pipelinesManager`, `descriptorsManager`, `reflectionSystem`, `passesGraph`, `gpuSceneManager`,
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
   framebuffers → GPUSceneManager `define*` (then `changed` is reset) → descriptor memory allocation →
   static allocator upload → descriptor layouts + sets creation → pipelines (pipeline layouts first) →
   command pools (allocated from tasks registered by `define*`).
3. `Renderer::draw` per frame (skipped while `Globals::resizing`):
   if `GPUSceneManager::changed`, run `define*` again and reset it → resolve
   pending memory ops → GPUSceneManager `update*` → `passesGraph.waitForFrame(currentFrame)`
   (waits on the frame's fence of every `FRAME` pass) → `descriptorsManager.syncToFrame(currentFrame)` (copies changed descriptor shadows into the frame's entries) → `vkAcquireNextImageKHR` (signals the frame's
   `imageAcquiredSemaphore`) → re-record enabled command buffers →
   `passesGraph.compileIfDirty()` → `resolveSync_SubmitToGPU()` →
   `presentToScreen()` (waits on the swapchain image's
   `imageUseFinishedSemaphore`) → advance `currentFrame`.
4. `Renderer::shutdown` calls `vkDeviceWaitIdle`, then destroys in reverse
   dependency order (sync → frame resources → one-shot pools → pipelines (and pipeline layouts) →
   descriptor sets and layouts → render passes → framebuffers → shaders → memory → swapchain → device →
   surface → instance).
5. Window resize (`Core::windowSizeCallback`): `vkDeviceWaitIdle`, destroy and
   recreate swapchain + framebuffers.

Frames vs images: per-frame-in-flight objects live in `FrameResources`
(indexed by `currentFrame`); per-swapchain-image objects live on the swapchain
`Image` (indexed by `imageIndex`). Framebuffers are indexed by `imageIndex`. Per-frame descriptor copies (`Descriptor::memoryEntryIds`, `DescriptorSet::vkHandles`) are indexed by `currentFrame`.

## Queues

`QueueFamilyEnum` = `GRAPHICS(0)`, `TRANSFER(1)`, `COMPUTE(2)`.
`Device::queues` and `QueueFamilyIndices::indices` have 4 slots; slot
`PresentationQueueId` (3) is the present queue. `CmdPoolsPack` holds one command
pool per queue family (3).

`Device::getQueueFamilies` prefers the graphics family for `TRANSFER` and `COMPUTE` (a graphics family always supports transfer; compute only if it reports it), so normally all three are one `VkQueue`
(queue 0 of the family). A dedicated family is only the fallback. When the families differ, `MemoryManager::queueTransferSync` (`QueueTransferSync`, set up first in `MemoryManager::setup`) makes
`GPU_LOCAL` buffers `CONCURRENT` (every distinct family listed; `CPU_SHARED` and staging buffers stay `EXCLUSIVE`) and creates one "done" semaphore per distinct family (graphicsDone, transferDone, computeDone).
`PassesGraph::resolveSync_SubmitToGPU` makes the first submit of every batch wait (`ALL_COMMANDS`) for each done semaphore that is signaled and not waited on yet (also its own queue's, since a binary semaphore
has to be waited on before it is signaled again) and the last submit of the batch signal its queue's done semaphore. So every submit is ordered after everything submitted before it on the other queues (this also covers a copy not overwriting bytes
an earlier draw reads). The extra semaphores live in `SubmissionMetadata::submit*`, rebuilt per submit; the batch's own semaphores are untouched. With one queue nothing is added.

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
- Every `FRAME` pass owns one fence per frame in flight (`Pass::frameFenceIds`, created signaled in `addPass`). The framework picks
  `frameFenceIds[currentFrame]` for the pass's submit and resets it right before `vkQueueSubmit` (a pass that is disabled that frame
  keeps its fence signaled). `PassesGraph::waitForFrame(frame)` waits on all of them. `addPass` throws if a `FRAME` pass is given a
  fence; `ONESHOT` passes still take one.
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
  batch) and a final `TRANSFER_WRITE -> HOST_READ` barrier when the batch has readbacks. The start barrier's source scope also has the graphics and compute stages, so a copy never overwrites bytes an earlier frame's draw still reads, and a batch with update copies ends with a `TRANSFER_WRITE ->` vertex/index/uniform/shader read barrier, so the draws submitted after it see the copies. Both barriers rely on one queue, which is the normal case (see Queues). With separate families the batch's start barrier has `TRANSFER` as its only source stage (a transfer-only family doesn't support graphics or compute stages), the final read barrier is skipped, and the done semaphores of `QueueTransferSync` order the batch against the other queues instead.
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
  `addEntry`, `allocateEntry` and `reserveEntry` (and their `Api.h` forwarders) take `readOnly_` (default false), stored in
  `MemoryEntry::readOnly`. `MemoryManager::lockEntry` sets `MemoryEntry::locked` (throws if the entry is not `readOnly`). Writes are legal
  until then; once locked, `updateEntry`, `getStagingContainer` and `getSharedContainer` throw. Readbacks (`getReadbackContainer`
  builds its shared container without the check) and removal stay allowed. `Mesh` (readOnly meshes) and `DescriptorSet::lockReadOnlyDescriptor` lock entries.
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

## Descriptors (static buffer descriptors)

Uniform and storage buffer descriptors only. No dynamic buffers, bindless, images or samplers (the
`DescriptorSetLayout` constructor throws for the latter two).

- `DescriptorsManager` (`Graphics/`) owns `PoolMap<DescriptorSetLayout> layouts` and
  `Pool<DescriptorSet> descriptorSet`.
- `DescriptorSetLayout` has one `DescriptorTypeEnum` and one set of shader stages for all its bindings.
  `addDescriptorBlueprint(name)` adds a binding: it creates the blueprint plus its own uniform/storage
  `ReflectionLayout` (in `ReflectionSystem`), both named `<layout name>/<name>` so blueprint names only have to be unique inside one layout, the blueprint slot index is the binding number (dead slots are skipped,
  numbers can be sparse; removing a blueprint also removes its `ReflectionLayout`). Members are added with `DescriptorBlueprint::addMember` (forwarded to that layout). The API function `addDescriptorBlueprint(layoutName, blueprintName)` finds the layout by name.
  `create()` builds the `VkDescriptorSetLayout`.
- `deriveDescriptorSet(layoutId, allocatorType, memoryVisability)` creates a `DescriptorSet` with one `Descriptor` per
  blueprint slot (same slot index) and adds the set id to each blueprint's `derivedDescriptors`. Blueprints can't be
  added, removed or get members after memory of a derived set was allocated, and can't be added/removed once sets are derived.
- A `DescriptorSet` owns its `DescriptorPool` (1 pool : 1 set object, destroyed with the set; destroying the pool
  frees the `VkDescriptorSet`s). The set holds `vkHandles`: one `VkDescriptorSet` per frame in flight if any descriptor `isPerFrame()` (`!readOnly && CPU_SHARED`), otherwise one. The pool gets `maxSets` = copies and per-type count = descriptors x copies; `get()` returns the copy of `currentFrame` (or the only one). Two phases: `allocateDescriptorsMemory()` calls `allocateMemoryEntry` per descriptor and copy (`Descriptor::memoryEntryIds`: one entry per frame in flight if `isPerFrame()`, otherwise one; `getEntryId(frame)` picks; size = layout size,
  one repeat unit, `Descriptor::readOnly` passed as `readOnly_`; `setDescriptorReadOnly(blueprintId, readOnly_)` sets it and throws once the memory was allocated; static entries have to be added before `StaticAllocator::allocateAndUpload`), `create()` allocates the
  `VkDescriptorSet` copies and writes every binding of every copy with one `vkUpdateDescriptorSets` (copy f points at `getEntryId(f)`) (offset 0 for dynamic entries,
  `inBufferFirstByte` for static ones). `MemoryEntry::descriptorSet` points back at the set (on every copy's entry).
- `DescriptorSet::setDescriptorMemberValue<T>(blueprintId, memberId, value)` writes through
  `getStagingContainer` (`GPU_LOCAL`) or `getSharedContainer` (`CPU_SHARED`, readOnly descriptors) and `ReflectionLayout::setMember`, like `Mesh`.
  A per-frame descriptor (`isPerFrame()`) instead writes `Descriptor::shadow` (a `CPUDataContainer`, one repeat unit, zeroed in `allocateDescriptorsMemory`) and bumps `Descriptor::version` (starts at 1);
  `Descriptor::copyVersions[frame]` (starts at 0) is the version frame's entry holds.
- `Renderer::setup` calls `allocateAllDescriptorsMemory()` before the static upload and `createAll()` (layouts, then sets) after it and
  before `createAllPipelines()`. `Renderer::shutdown` calls `descriptorsManager.destroy()` (sets, then layouts) after the pipelines.
- Pipelines: `PipelinesManager::pipelineLayouts` (`Pool<PipelineLayout>`) owns the pipeline layouts, a `GraphicsPipeline`
  holds a `pipelineLayoutId`. `PipelineLayout::usedByGraphicsPipelineIds` and `DescriptorSetLayout::usedByPipelineLayoutIds`
  are filled by `PipelinesManager::addGraphicsPipeline` / `addDescriptorSetLayoutToPipelineLayout`.
  `createAllPipelines` creates the layouts first and skips pipelines and layouts that already exist. `bindDescriptorSet` always records `vkCmdBindDescriptorSets` and remembers the set per index in `Renderer::recordingSetIds` (cleared by `beginCMDsRecording`; `bindGraphicsPipeline` stores the bound pipeline in `Renderer::recordingPipelineId`; recording is single-threaded, one command buffer at a time). A `Shader` lists the descriptors it reads (`Shader::addDescriptor(setIndex, blueprintId)`, `Api.h` `addDescriptorToShader`; a shader may have none), see the sync paragraph below.
- Editor-time cascade: adding/removing a blueprint of a created `DescriptorSetLayout` calls `recreate()`: destroy + create the
  layout, then `PipelineLayout::recreate()` for each layout using it (destroy + create the layout and its graphics pipelines).
  `PipelineLayout::addDescriptorSetLayout` on a created layout recreates it too. Nothing waits for the GPU, the caller has to
  make sure nothing submitted uses them.
- CPU/GPU sync of `CPU_SHARED` writes: a per-frame descriptor's writes only touch its CPU shadow, so they never wait and are legal at any time (also while recording).
  `DescriptorsManager::syncToFrame(currentFrame)` runs in `Renderer::draw` right after `waitForFrame(currentFrame)` and, per set, copies every shadow whose
  `version` differs from `copyVersions[frame]` into that frame's entry (whole entry, `MemoryManager::writeShared`) and records the version. The fence wait is the write-after-read
  dependency (the frame's previous submission read that copy), the memory is coherent so no flush, and the host write before `vkQueueSubmit` is visible to the submission. Other frames' copies are never touched.
  Non-per-frame `CPU_SHARED` descriptors (readOnly) are written straight into their single entry and don't wait: nothing tracks which frames bind a set anymore, so they are only safe to write before the GPU first reads them. `drawMeshIndexed` looks up the descriptors the vertex and fragment shader of the bound pipeline declared in the sets bound at their indices (`getRecordingSet`; it throws when nothing is bound there) and skips the draw while one of them has a write pending (`DescriptorSet::isDescriptorSettled`: a `GPU_LOCAL` descriptor, readOnly or not, whose first write is still pending (`MemoryManager::hasPendingWrite`, i.e. upload queued or in flight, or an `UPDATE` waiting); once a check saw it settled, `Descriptor::firstDataSettled` stays true and later updates are not waited for), so a draw never reads a descriptor before its first data was copied (the batch barriers above order the later updates). Descriptors the shaders don't declare don't matter, even in the same set, and the set is bound anyway (the declaration is what lets the draw wait for the descriptors, so it is a contract: nothing checks the SPIR-V, so a shader reading an undeclared descriptor can read it before its data was copied): every binding was written by `create` and a shader only reads the bindings it uses. A skipped draw is recorded again on a later frame, since the command buffer is re-recorded every frame. For a recorded draw `drawMeshIndexed` queues the declared descriptors (`DescriptorsManager::queueLockOnSubmit`) and `Renderer::draw` calls `DescriptorsManager::lockBoundDescriptors` right after `resolveSync_SubmitToGPU`, which runs `DescriptorSet::lockReadOnlyDescriptor` (once per descriptor, `Descriptor::readOnlyLocked`): it `lockEntry`s every copy of the descriptor if it is readOnly, so a write after the submit that first recorded a draw reading it throws (also `GPU_LOCAL`); descriptors no draw read stay writable. `GPU_LOCAL` writes go through
  staging and don't wait.
- Removing (`removeDescriptorBlueprint`, `removeDescriptorSet`, `removeDescriptorSetLayout`, forwarded by `Api.h`): a blueprint can only be
  removed while no set is derived from its layout (it also removes the blueprint's `ReflectionLayout`). `removeDescriptorSet` waits (`PassesGraph::waitForFrame`) for
  every frame in flight, removes all entries (every copy) of a `DYNAMIC` set (`GPU_LOCAL` ones through the deferred `REMOVE` operation), unregisters the set from its blueprints and destroys it (its pool).
  Entries of a `STATIC` set can't be removed and stay in the static heap until shutdown. Like writes, it must not be called while recording.
  `removeDescriptorSetLayout` throws if sets are derived or a `PipelineLayout` still uses the layout (nothing detaches a layout from a pipeline layout yet), otherwise it
  destroys the layout and removes its blueprints.
- Scene change: `Renderer::draw`'s re-define branch calls `allocateAllDescriptorsMemory`, `createAll` and `createAllPipelines` after `define*`. All three skip what already exists (a set counts as created once it has `vkHandles`, with all its per-frame copies). A new `STATIC` set
  throws there (static entries have to be added before `StaticAllocator::allocateAndUpload`), so sets defined by a scene have to be `DYNAMIC`. Passes defined by the scene
  have the limits described in `docs/OptionalPolishNotes.md`.
- Not handled: `ReflectionLayout` adds no std140/std430 padding, static entries are packed without
  `minUniformBufferOffsetAlignment` / `minStorageBufferOffsetAlignment`.

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

- `Mesh::queueUpload(allocatorType, visibility, readOnly_ = false)` reserves the vertex entry
  (`MemoryManager::reserveEntry`, no data yet) and uploads the index entry
  (`vbMemoryUploadId`, `ibMemoryUploadId`). With `readOnly_` both entries are created `readOnly`: the index entry is locked
  at the end of `queueUpload`, the vertex entry at the end of `commitUpload` (`MemoryManager::lockEntry`), so later
  `setVerticeMember` calls throw. Readbacks still work.
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
  `p` (`pRenderPass`, `pMapped`).
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
