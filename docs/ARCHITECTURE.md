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
src/Demo/                     Demo app: main.cpp, Demo (example GPU scene), Triangle (mesh helper)
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
    Backend/Descriptor/       DescriptorSetLayout, DescriptorLayout, MemberBlueprint, DescriptorShape,
                              DescriptorSet, Descriptor,
                              DescriptorPool
    Backend/ValidationLayers/ DebugMessenger, layer/extension lists
    Pass/                     Pass, Task, PassId, SubmissionBatch, SubmissionMetadata
    Reflection/               ReflectionLayout, VariableBlueprint, CPUDataContainer,
                              GPUDataContainer, DataContainerConcepts
    Resources/                Mesh, Vertex, Texture
    MemoryManagment/          Static/Dynamic allocators, Heaps/, Pages/, CreateParams/ (WIP)
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

1. `main` calls `Demo::setGPUSceneToDemo()`, which registers function
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
   (waits on the frame's fence of every `FRAME` pass) → `memoryManager.syncCopiesToFrame(currentFrame)` (copies changed shadows into the frame's copies of every entry with copies, descriptors included) → `vkAcquireNextImageKHR` (signals the frame's
   `imageAcquiredSemaphore`) → re-record enabled command buffers →
   `passesGraph.compileIfDirty()` → `resolveSync_SubmitToGPU()` →
   `presentToScreen()` (waits on the swapchain image's
   `imageUseFinishedSemaphore`) → advance `currentFrame`.
4. `Renderer::shutdown` calls `vkDeviceWaitIdle`, then destroys in reverse
   dependency order (sync → frame resources → one-shot pools → pipelines (and pipeline layouts) →
   descriptor sets and layouts → render passes → framebuffers → shaders → memory → swapchain → device →
   surface → instance).
5. `Renderer::switchDevice(id)` (`Graphics::switchDevice`) rebuilds the renderer on `suitableDevices[id]`, keeping the instance, surface and window (no-op for the current id, throws for an unknown one). It calls the `GPUSceneManager::destroy` callback, then `destroyDeviceObjects()` (the `shutdown` body up to the swapchain: `vkDeviceWaitIdle`, then the reverse-order destroy), `resetDeviceState()` (CPU-side state the destroy calls leave behind: `PassesGraph`, `MemoryManager` and its allocators, reflection layouts, mesh/texture/image pools, presentation render pass, swapchain target/framebuffer vectors, frame resources, one-shot pools, recording state), `Device::destroy`, `selectDevice(id)`, `createLogicalDevice`, `projectManager.setupEnvironment()` and `createDeviceObjects()` (the `setup` body from the swapchain on, which runs `define*` again). Ids the application kept (meshes, sets, passes) are invalid afterwards unless `define*` recreates them. It waits for the GPU, so don't call it while recording or inside `draw`.
6. Window resize (`Core::windowSizeCallback`): `vkDeviceWaitIdle`, destroy and
   recreate swapchain + framebuffers.

Frames vs images: per-frame-in-flight objects live in `FrameResources`
(indexed by `currentFrame`); per-swapchain-image objects live on the swapchain
`Image` (indexed by `imageIndex`). Framebuffers are indexed by `imageIndex`. Per-frame descriptor copies (the copies of `Descriptor::memoryEntryId`, `DescriptorSet::vkHandles`) are indexed by `currentFrame`.

## Device selection

`Device::getPhysicalDevice` (needs the surface) collects every physical device that passes `checkDeviceSuitable` into `Device::suitableDevices` (`SuitableDevice`: handle + `VkPhysicalDeviceProperties`), throws if there is none, and picks the first `DISCRETE_GPU` (otherwise index 0) through `Device::selectDevice(id)`. That sets `mainDeviceId`, `physicalDevice` and `Device::properties`. Code that needs a device limit reads `getMainDevice().properties.limits`.

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
  allocated). `addEntry` rounds the first byte of every uniform entry up to `minUniformBufferOffsetAlignment` and of every
  storage entry up to `minStorageBufferOffsetAlignment` (descriptor offsets must be multiples of them); other buffer types stay packed. `allocateAndUpload()` creates one `GPULocalHeap` (one memory block,
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
  `ONESHOT` `TRANSFER` pass. `CPU_SHARED` entries need no copy and are ready immediately; their readback is a live view (no snapshot) of the mapped memory, or of the shadow when the entry has copies, so it shows whatever was written when it is read.
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
- Entry creation takes a `MemoryEntryCreateParams` (`MemoryManagment/CreateParams/`, filled field by field like a Vulkan create info, start from `= {}`): `name`, `allocatorType`, `memoryVisability`, `bufferType`, `size`, `data` and
  `access` (`MemoryAccessParams`: `accessMode`, `copyCountMode`, `customCopyCount`). `validate()` throws (also in release) on a missing or contradicting field: `allocatorType`, `memoryVisability`, `bufferType` and `accessMode` must be set (each starts at its `..._UNSET` value: `ALLOCATOR_TYPE_UNSET`, `MEMORY_VISABILITY_UNSET`, `BUFFER_TYPE_UNSET`, `MemoryAccessModeEnum::UNSET`; the first three sit after the real values so existing indices don't move),
  `copyCountMode` must be `FOLLOW_FRAMES_IN_FLIGHT` or `CUSTOM_COPY_COUNT` for a `READ_AND_WRITE` `CPU_SHARED` entry, any mode for a `READ_AND_WRITE` `GPU_LOCAL` entry (`ONE` = one device buffer written through staging) and `CopyCountModeEnum::ONE` for a `READ_ONLY` one, `customCopyCount` must be >= 1 for
  `CUSTOM_COPY_COUNT` and 0 otherwise. `MemoryManager::addEntry` needs `data`, `allocateEntry` / `reserveEntry` need it to be `nullptr`; `Api.h` `allocateMemoryEntry` takes the params plus the layout and repeat count and needs `size` to be 0.
  `READ_ONLY` sets `MemoryEntry::readOnly`.
- Copies of `READ_AND_WRITE` entries, `CPU_SHARED` or `GPU_LOCAL` (`CopyCountModeEnum` in `Utilities.h`, the modes and their trade-offs are documented there). A `READ_ONLY` entry is a single instance every frame in flight reads. With more than one copy, `MemoryManager::createCopies`
  makes copy 0 the entry itself and the others hidden entries of the same allocator (`MemoryEntry::copyEntryIds`; static copies are added like any static entry, so before `allocateAndUpload`). Copies have the visability of the entry (a `GPU_LOCAL` entry gets N device buffers, so N times the VRAM plus one RAM shadow). Writes (`writeShared`, `updateEntry`, and the shared container of `getSharedContainer`, which is also legal for a `GPU_LOCAL` entry with copies) go to
  `MemoryEntry::shadow` and bump `version`, `readShared` (so readbacks of an entry with copies, which are ready at once and need no readback copy) reads the shadow. A `reserveEntry` of an entry with copies takes no staging range and has nothing pending, `commitEntry` only ends the reserve. `MemoryManager::syncCopiesToFrame(currentFrame)` runs in `Renderer::draw` after `waitForFrame` and copies a changed shadow into the entry's copy `frame % copyCount`
  (`copyVersions`, whole entry). A `GPU_LOCAL` copy is refreshed through `getStagingContainer` on the copy entry: a whole-entry staging range and an `UPDATE` operation, which belongs to no frame (it is queued once the copy entry has no upload in flight, the batch barriers order it after earlier draws and before later ones). `getFrameCopy(entry)` returns that copy, `drawMeshIndexed` binds it once `isFrameCopyReady` says the copy was filled (later refreshes don't make it unready, it holds the previous bytes until they complete). `removeDynamicEntry` removes the copies too (`GPU_LOCAL` ones through one deferred `REMOVE` each). Per-frame descriptors use it: `allocateDescriptorsMemory` creates the entry of an `isPerFrame()` descriptor with `FOLLOW_FRAMES_IN_FLIGHT`, `MemoryManager::getCopy(entry, f)` picks the copy a set copy points at.
- `MemoryManager::getBuffer(entry)` returns the `Buffer` an entry lives in: the
  page buffer for dynamic entries (whole buffer), or the static heap buffer for
  the entry's type and visibility (`StaticAllocator::getBuffer`; add
  `inBufferFirstByte`). `drawMeshIndexed` and `MemoryEntry::upload` use it.
- `MemoryManager::updateEntry` writes every `GPU_LOCAL` update (static or dynamic)
  into a fresh open staging range immediately and queues an `UPDATE` operation
  (`UpdateRequest` = entry id + `StagingData`); `resolvePendingOperations` queues the
  copy once the entry has no upload in flight. `CPU_SHARED` entries are written
  immediately. An entry with copies is not staged per write: the write goes to its shadow and `syncCopiesToFrame` stages the copies.
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
  `MemoryAccessParams::accessMode` `READ_ONLY` (see the create params above) is stored in `MemoryEntry::readOnly`. `MemoryManager::lockEntry` sets `MemoryEntry::locked` (throws if the entry is not `readOnly`). Writes are legal
  until then; once locked, `updateEntry`, `getStagingContainer` and `getSharedContainer` throw. Readbacks (`getReadbackContainer`
  builds its shared container without the check) and removal stay allowed. `Mesh` (readOnly meshes) and `DescriptorSet::lockReadOnlyDescriptor` lock entries.
- `GPU_LOCAL` entry updates are batched by `MemoryManager`. `MemoryEntry::upload`
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
- `MemoryVisabilityEnum`: `GPU_LOCAL` (device-local, uploaded through staging)
  and `CPU_SHARED` (host-visible|coherent, written with map/memcpy/unmap).

## Descriptors (static buffer descriptors)

Uniform and storage buffer descriptors, optionally dynamic. No bindless, images or samplers (`addBufferDescriptorLayout` throws for any type but uniform / storage).

- `DescriptorsManager` (`Graphics/`) owns `PoolMap<DescriptorSetLayout> layouts` and
  `Pool<DescriptorSet> descriptorSet`.
- `DescriptorSetLayout` has one set of shader stages for all its bindings; the descriptor type is per descriptor layout (`DescriptorLayout::type`, `UNIFORM_DESCRIPTOR` or `STORAGE_DESCRIPTOR`), so one set can mix uniform and storage bindings.
  `addBufferDescriptorLayout(name, type)` / `addBufferDynamicDescriptorLayout(name, type, dynamicCount)` add a binding (`addImageDescriptorLayout` is the not yet supported image twin, it throws). A `DescriptorLayout` is one binding, i.e. the block the shader sees (`buffer Objects { Object states[2]; mat4 model; }`),
  the descriptor layout slot index is the binding number (dead slots are skipped, numbers can be sparse). Its members are added in the order of the shader's block, each a `MemberBlueprint` (`Backend/Descriptor/`) of one of two kinds: a variable directly in the block
  (`DescriptorLayout::addMemberBlueprint`, a scalar or matrix overload, each with optional `repeatCountPerDimension`, `Api.h` `addMemberToDescriptorLayout`) or a struct (`addStructMemberBlueprint(name, repeatCountPerDimension)`, `Api.h` `addStructMemberToDescriptorLayout`) whose variables are added with `addVariable(structMemberId, ...)` (`Api.h` `addVariableToDescriptorLayout`).
  Every member owns a `ReflectionLayout` by value (a variable member holds exactly one variable, a struct member its variables; uniform layouts are `STD140`, storage layouts `STD430`). The id `addMemberBlueprint` / `addVariable` returns is a descriptor wide variable id (a slot of `DescriptorLayout::variables`, a `VariableRef` = member id + id in that member's layout), the id every setter takes.
  `create()` builds the `VkDescriptorSetLayout`.
- `deriveDescriptorSet(layoutId, allocatorType, memoryVisability)` creates a `DescriptorSet` with one `Descriptor` per
  descriptor layout slot (same slot index) and adds the set id to each descriptor layout's `derivedDescriptors`. Descriptor layouts can't be
  added, removed or get members after memory of a derived set was allocated, and can't be added/removed once sets are derived.
- A `DescriptorSetLayout` owns the `DescriptorPool`s of all its sets (`pools`, destroyed in `destroy()`). `addDescriptorSetLayout` has no set count: each pool is sized for `setsPerPool_` sets (optional parameter, default `DescriptorSetsPerPool` = 16 in `Utilities.h`, throws on 0), per (type, dynamic) kind descriptor layouts x `setsPerPool` x `framesAtFlightCount` (the worst case of a set whose descriptors are per frame). Pools are added lazily by `DescriptorSetLayout::allocateSets`, which tries every pool in order and adds one when all report `VK_ERROR_OUT_OF_POOL_MEMORY` / `VK_ERROR_FRAGMENTED_POOL`; the set remembers its pool in `DescriptorSet::poolId`. Pools are `FREE_DESCRIPTOR_SET_BIT`, `removeDescriptorSet` frees the set's `VkDescriptorSet`s (`vkFreeDescriptorSets`) into that pool. The device has no limit on the number of sets; `allocateDescriptorsMemory` throws when a set has more dynamic uniform / storage descriptors than the device allows, the other descriptor limits are checked per pipeline layout (`PipelineLayout::validateLimits`). The set holds `vkHandles`: one `VkDescriptorSet` per frame in flight if any descriptor `isPerFrame()` (`!readOnly && CPU_SHARED`), otherwise one; `get()` returns the copy of `currentFrame` (or the only one). Two phases: `allocateDescriptorsMemory()` calls `allocateMemoryEntryRaw` per descriptor (`Descriptor::memoryEntryId`: one entry, with one copy per frame in flight if `isPerFrame()`; size = `DescriptorShape::getSize()` (the block shape, still one descriptor, only the entry grows), `Descriptor::readOnly` passed as `readOnly_`; `setDescriptorReadOnly(descriptorLayoutId, readOnly_)` sets it and throws once the memory was allocated; static entries have to be added before `StaticAllocator::allocateAndUpload`), `create()` allocates the
  `VkDescriptorSet` copies and writes every binding of every copy with one `vkUpdateDescriptorSets` (copy f points at copy f of the descriptor's entry) (offset 0 for dynamic entries,
  `inBufferFirstByte` for static ones). `MemoryEntry::descriptorSet` points back at the set (on every copy of the entry).
- Block shape: a descriptor holds one block (or `dynamicCount` of them, see below) of its descriptor layout's members (one descriptor, one entry). `DescriptorLayout::makeShape` (called by `allocateDescriptorsMemory`) places the members in declaration order into a `DescriptorShape` (`Backend/Descriptor/DescriptorShape.h`): a variable at its next multiple of its `alignmentRule`, a struct member at a multiple of its layout's `structAlignment` (the largest variable rule, at least 16 for `STD140`) taking `repeatCountPerDimension` copies of the struct (a `RepeatShape`, `Reflection/RepeatShape.h`: per layer a count and a stride, innermost stride = the struct's `ReflectionLayout::size`, outer stride = inner stride x inner count, offset of an element = sum of id x stride over the layers, bounds checked per layer; `repeatCountPerDimension` is outermost layer first, e.g. `{2, 3}` = `states[2][3]`, `{1}` = one struct, throws on an empty list or a 0 count), then the block size padded to the largest alignment (at least 16 for uniform). This is the std140 / std430 layout of the shader's block, so the declaration order has to match the shader. The entry size, the per-frame shadow size and the dynamic `.range` all come from the shape. A plain uniform binding sees the whole entry, which has to fit `maxUniformBufferRange` (checked in `validateShape`, storage buffers have no such limit). `DescriptorSet::writeMemberRange` works out the byte offset of a variable (dynamic unit x stride + the member's `firstByteId` + the struct element picked by its ids + the variable's own instance, `getRepeatOffset`) and writes through containers with `repeatCount = 1`, `repeatStride` and `repeatBaseOffset` (= the struct element's byte offset): `getRepeatUnitOffset` returns `repeatBaseOffset + repeatUnitId x repeatStride`, so `Mesh` (base 0) is unchanged. A variable of a struct member takes one id per layer of its struct, a variable directly in the block none. Every plain setter has an overload taking `std::vector<uint32_t> idsPerLayer_`, `repeatUnitId_` is the one-layer case (it has to be 0 for a variable directly in the block, `makeSingleLayerIds`).
- Dynamic descriptors: `addBufferDynamicDescriptorLayout(name, type, dynamicCount)` makes a dynamic descriptor layout: the whole block is repeated `dynamicCount` (at least 1) times, one of them is picked per bind (`DescriptorLayout::dynamicCount`; buffers only, fixed when it is added; `DescriptorSetLayout::dynamic` is true while any live descriptor layout is, recomputed on remove). Dynamic is a property of the block, not of the set: a set can mix dynamic and plain bindings. The binding becomes `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (or `STORAGE_BUFFER_DYNAMIC`; `getVkDescriptorType(type, dynamic)`, there is no dynamic `DescriptorTypeEnum` value) and the pool counts every (type, dynamic) kind separately. The dynamic stride (`DescriptorShape::dynamicLayer`) is the block size rounded up with `alignUp` to `minUniformBufferOffsetAlignment` (uniform) or `minStorageBufferOffsetAlignment` (storage), the binding's `.range` is one unpadded block (`shape.blockSize`), and `allocateDescriptorsMemory` throws when a set has more dynamic uniform / storage descriptors than `maxDescriptorSetUniformBuffersDynamic` / `maxDescriptorSetStorageBuffersDynamic`. Entry starts are aligned (own buffer at offset 0 for dynamic entries, `StaticAllocator::addEntry` for static ones), `create` throws if a dynamic entry's `bufferInfo.offset` isn't, so offset + dynamic offset is always a multiple. Writing: the plain setters throw on a dynamic descriptor, `setDynamicDescriptorMember...` (same 7 variants) take `dynamicUnitId_` (the block) and optional `innerIdsPerLayer_` (the struct's layers). Binding: `bindDescriptorSet(..., dynamicUnitIds_)` takes one unit id per dynamic descriptor in binding order (empty = unit 0); `DescriptorSet::getDynamicOffsets` turns them into `id x dynamic stride` byte offsets for `vkCmdBindDescriptorSets` (Vulkan orders dynamic offsets by binding) and throws on a wrong count or an id >= `dynamicCount`.
- `DescriptorSet::setDescriptorMemberValue<T>(descriptorLayoutId, variableId, value)` (every setter takes a trailing `repeatUnitId_ = 0` that selects the repeat unit, offset `repeatUnitId_ * layout.size`, throws if >= `repeatCount`; plus `setDescriptorMemberMatrixElement`, `setDescriptorMemberMatrixRow` / `Column`, with `Array<T>` or `Matrix<T>` sources; every setter also takes a trailing `repeatIdPerDimension_` (one id per dimension of the variable, outermost first, empty for a variable that isn't repeated); a `GPU_LOCAL` write covers only the bytes it replaces, a column is written element by element) writes through
  `getStagingContainer` (`GPU_LOCAL`) or `getSharedContainer` (`CPU_SHARED`, readOnly descriptors) and `ReflectionLayout::setMemberScalar`, like `Mesh`.
  A per-frame descriptor (`isPerFrame()`) also writes through `getSharedContainer`: the entry has copies, so the write lands in `MemoryEntry::shadow` (zeroed when the copies are created) and bumps the entry's `version`.
- `Renderer::setup` calls `allocateAllDescriptorsMemory()` before the static upload and `createAll()` (layouts, then sets) after it and
  before `createAllPipelines()`. `Renderer::shutdown` calls `descriptorsManager.destroy()` (sets, then layouts) after the pipelines.
- Pipelines: `PipelinesManager::pipelineLayouts` (`Pool<PipelineLayout>`) owns the pipeline layouts. The user never creates one: `addGraphicsPipeline` calls `getOrAddPipelineLayout`, which builds the ordered
  set layout list from the descriptors the vertex and fragment shader declared (`ShaderDescriptorUse::setLayoutId` at `setIndex`; the shaders must agree on a shared set, otherwise it throws; an index nobody declared stays `UninitializedPoolId` and `PipelineLayout::create` fills it with `PipelinesManager::getEmptySetLayout()`, a zero-binding set layout, because a pipeline layout can't have a null set layout without the graphics pipeline library feature) and reuses an existing
  layout with the same list, so pipelines with the same set layouts share one `VkPipelineLayout`. `PipelineLayout::create` first runs `validateLimits`, which throws a readable error when the layout's sets break `maxBoundDescriptorSets`, the totals `maxDescriptorSetUniformBuffers` / `StorageBuffers` (and `...Dynamic`) over all its sets, or `maxPerStageDescriptorUniformBuffers` / `StorageBuffers` / `maxPerStageResources` for the vertex, fragment or compute stage (so `recreate` is checked too). Shaders have to declare their descriptors before the pipeline is added. A `GraphicsPipeline`
  holds a `pipelineLayoutId`. `PipelineLayout::usedByGraphicsPipelineIds` and `DescriptorSetLayout::usedByPipelineLayoutIds`
  are filled when a pipeline / a new layout is added.
  `createAllPipelines` creates the layouts first and skips pipelines and layouts that already exist. `bindDescriptorSet` always records `vkCmdBindDescriptorSets` and remembers the set per index in `Renderer::recordingSetIds` (cleared by `beginCMDsRecording`; `bindGraphicsPipeline` stores the bound pipeline in `Renderer::recordingPipelineId`; recording is single-threaded, one command buffer at a time). A `Shader` lists the descriptors it reads (`Shader::addDescriptor(setIndex, setLayoutId, descriptorLayoutId)`, `Api.h` `addDescriptorsToShader(shaderId, setIndex, setLayoutId, descriptorLayoutIds)`, which calls it once per descriptor layout; a shader may have none), see the sync paragraph below.
- Editor-time cascade: adding/removing a descriptor layout of a created `DescriptorSetLayout` calls `recreate()`: destroy + create the
  layout, then `PipelineLayout::recreate()` for each layout using it (destroy + create the layout and its graphics pipelines).
  Nothing waits for the GPU, the caller has to
  make sure nothing submitted uses them.
- CPU/GPU sync of `CPU_SHARED` writes: a per-frame descriptor's writes only touch the entry's CPU shadow, so they never wait and are legal at any time (also while recording).
  `MemoryManager::syncCopiesToFrame(currentFrame)` runs in `Renderer::draw` right after `waitForFrame(currentFrame)` and copies every shadow whose
  `version` differs from `copyVersions[copy]` into that frame's copy (whole entry) and records the version. The fence wait is the write-after-read
  dependency (the frame's previous submission read that copy), the memory is coherent so no flush, and the host write before `vkQueueSubmit` is visible to the submission. Other frames' copies are never touched.
  Non-per-frame `CPU_SHARED` descriptors (readOnly) are written straight into their single entry and don't wait: nothing tracks which frames bind a set anymore, so they are only safe to write before the GPU first reads them. `drawMeshIndexed(meshId, instanceCount, cmd, firstInstance = 0)` passes the optional `firstInstance` to `vkCmdDrawIndexed`, so `gl_InstanceIndex` starts there and a draw can pick its repeat unit(s) of an SSBO descriptor (`objects[gl_InstanceIndex]`). It looks up the descriptors the vertex and fragment shader of the bound pipeline declared in the sets bound at their indices (`getRecordingSet`; it throws when nothing is bound there) and skips the draw while one of them has a write pending (`DescriptorSet::isDescriptorSettled`: a `GPU_LOCAL` descriptor, readOnly or not, whose first write is still pending (`MemoryManager::hasPendingWrite`, i.e. upload queued or in flight, or an `UPDATE` waiting); once a check saw it settled, `Descriptor::firstDataSettled` stays true and later updates are not waited for), so a draw never reads a descriptor before its first data was copied (the batch barriers above order the later updates). Descriptors the shaders don't declare don't matter, even in the same set, and the set is bound anyway (the declaration is what lets the draw wait for the descriptors, so it is a contract: nothing checks the SPIR-V, so a shader reading an undeclared descriptor can read it before its data was copied): every binding was written by `create` and a shader only reads the bindings it uses. A skipped draw is recorded again on a later frame, since the command buffer is re-recorded every frame. For a recorded draw `drawMeshIndexed` queues the declared descriptors (`DescriptorsManager::queueLockOnSubmit`) and `Renderer::draw` calls `DescriptorsManager::lockBoundDescriptors` right after `resolveSync_SubmitToGPU`, which runs `DescriptorSet::lockReadOnlyDescriptor` (once per descriptor, `Descriptor::readOnlyLocked`): it `lockEntry`s every copy of the descriptor if it is readOnly, so a write after the submit that first recorded a draw reading it throws (also `GPU_LOCAL`); descriptors no draw read stay writable. `GPU_LOCAL` writes go through
  staging and don't wait.
- Removing (`removeDescriptorLayout`, `removeDescriptorSet`, `removeDescriptorSetLayout`, forwarded by `Api.h`): a descriptor layout can only be
  removed while no set is derived from its layout. `removeDescriptorSet` waits (`PassesGraph::waitForFrame`) for
  every frame in flight, removes all entries (every copy) of a `DYNAMIC_ALLOCATOR` set (`GPU_LOCAL` ones through the deferred `REMOVE` operation), unregisters the set from its descriptor layouts and destroys it (its pool).
  Entries of a `STATIC_ALLOCATOR` set can't be removed and stay in the static heap until shutdown. Like writes, it must not be called while recording.
  `removeDescriptorSetLayout` throws if sets are derived or a `PipelineLayout` still uses the layout (nothing detaches a layout from a pipeline layout yet), otherwise it
  destroys the layout and removes its descriptor layouts.
- Scene change: `Renderer::draw`'s re-define branch calls `allocateAllDescriptorsMemory`, `createAll` and `createAllPipelines` after `define*`. All three skip what already exists (a set counts as created once it has `vkHandles`, with all its per-frame copies). A new `STATIC_ALLOCATOR` set
  throws there (static entries have to be added before `StaticAllocator::allocateAndUpload`), so sets defined by a scene have to be `DYNAMIC_ALLOCATOR`. Passes defined by the scene
  have the limits described in `docs/OptionalPolishNotes.md`.
- `ReflectionLayout` pads members to its `alignment` (`NONE`/`STD140`/`STD430`): `VariableBlueprint` aligns `firstByteId` up to `alignmentRule`, `size` includes column/element/row padding, and `ReflectionLayout::size` ends at the last member rounded up to the layout's alignment (`structAlignment`: the largest member rule, at least 16 for `STD140`), so repeat unit N starts at `N * size`. `DescriptorLayout` creates the layouts of its members as `STD140` (uniform) or `STD430` (storage); vertex layouts stay `NONE` (the pipeline's stride and offsets are the packed sizes). A matrix aligns like a column in `STD430` (8 for 2-row matrices) and to 16 in `STD140` (`resolveSTDPaddingRule`).
  `DescriptorLayout::addMemberBlueprint` / `addVariable` (and `Api.h` `addMemberToDescriptorLayout` / `addVariableToDescriptorLayout` / `addVariableToVerticeLayout`) have a scalar and a matrix (`rowCount_`, `columnCount_`) overload, each with `repeatCountPerDimension_` (see below), forwarded to `ReflectionLayout::addVariableBlueprint`.

## Reflection

`ReflectionSystem::layoutsPerBufferType[bufferType - 1]` (no INDEX layout) holds
`ReflectionLayout`s. A layout is a pure blueprint: `VariableBlueprint`s (type, offset,
size) and the total `size`; it holds no data. A `CPUDataContainer` (byte vector ×
`repeatCount`, e.g. vertices) is owned by the instance that uses it.
`ReflectionLayout::setMemberScalar<T>` / `getMemberScalar<T>` take the container, a member id and
a repeat unit id, check `T` against the member's type and shape (`SCALAR`/`MATRIX`) through `GPUTypeMap<T>`, and
throw on mismatch or out-of-range repeat index. They write/read `sizeof(T)` bytes (matrix types such as `Mat3` column by column, each column at a multiple of the alignment rule; the element setters below do the same through `writeTyped`). `setElementInMemberMatrix`,
`setRowInMemberMatrix` and `setColumnInMemberMatrix` (plus the getters `getElementInMemberMatrix`, which read through `readTyped`; rows/columns come from a `Misc/Array.h` `Array<T>`, or from a row/column of a `Misc/Matrix.h` `Matrix<T>` through the overloads that take a source matrix and a source row/column id) write into `MATRIX` members at the offsets of
`VariableBlueprint::getMatrixElementOffset` (element stride = element size rounded up to `alignmentRule`, row stride = row size rounded up to it, same as `Array`/`Matrix`). They are templated on the container
type, constrained by `WritableDataContainer` / `ReadableDataContainer`
(`DataContainerConcepts.h`): `repeatCount` plus `write`/`read(offset, ptr, size)`.
`GPUDataContainer<Readable>` owns no bytes and touches no Vulkan memory: it holds ids (a `StagingData` range, or a
`MemoryEntryId` when `isShared`) and forwards each write/read to `MemoryManager`
(`StagingDataContainer`, write-only, is a per-update handle; `SharedDataContainer` is for `CPU_SHARED` entries). Under
`ENGINE_DEBUG` a write/read outside `validFirstOffset`/`validSize` throws. `Mesh`
keeps no copy of its vertex bytes; it writes through these containers.

### Mesh updates

- `Mesh::queueUpload(allocatorType, visibility, access)` (`access` is a `MemoryAccessParams`, used for both entries) reserves the vertex entry
  (`MemoryManager::reserveEntry`, no data yet) and uploads the index entry
  (`vbMemoryUploadId`, `ibMemoryUploadId`). With `READ_ONLY` access both entries are created `readOnly`: the index entry is locked
  at the end of `queueUpload`, the vertex entry at the end of `commitUpload` (`MemoryManager::lockEntry`), so later
  `setVerticeVariable` calls throw. Readbacks still work.
- `Mesh::setVerticeVariable<T>(variableId, verticeId, data)` computes
  `verticeId * layout.size + variable.firstByteId`, gets a container for that range
  and writes through `ReflectionLayout::setMemberScalar`. For `GPU_LOCAL` the container is a
  `StagingDataContainer` from `MemoryManager::getStagingContainer`; for `CPU_SHARED`
  it is the entry's `SharedDataContainer`. It throws under `ENGINE_DEBUG` if the mesh
  was never uploaded.
- Before `Mesh::commitUpload()` the `GPU_LOCAL` writes fill the entry's whole-entry
  upload staging range, so the initial data goes straight to staging.
  `commitUpload()` calls `MemoryManager::commitEntry`, which queues the copy. After it,
  every `setVerticeVariable` is an update: a fresh staging range sized to the variable, and
  an `UPDATE` operation.
- There is no dirty tracking (`DirtyInMesh`, `verticeToDirtyId`, `markVerticeDirty`,
  `queueUpdate` were removed): each write has its own range, so there is nothing to merge.
- `setVerticeVariable` has the same matrix variants as the descriptor set (`setVerticeVariableMatrixElement`, `setVerticeVariableMatrixRow` / `Column`). A vertex layout may have scalar variables, repeated or not (a `MAT*` variable or repeated element takes one attribute location per column); `GraphicsPipeline::create` throws for a `MATRIX` shaped variable.
- Reading: `Mesh::requestVerticeVariableReadback(variableId, verticeId)` calls
  `requestMemoryEntryReadback` for the variable's byte range and returns a `ReadbackId`. Once
  `isReadbackReady` is true, `Mesh::getVerticeVariable<T>(readbackId, variableId, verticeId)` (or `getVerticeVariableMatrixElement`; both take the optional `repeatIdPerDimension_`) maps it
  (`mapReadback`) and reads through `ReflectionLayout::getMemberScalar`. `GPU_LOCAL` meshes copy into a
  readback range, `CPU_SHARED` meshes are ready immediately and read the mapped memory. The caller
  calls `releaseReadback` afterwards. There is no synchronous `getVertice`.

### Repeated variables

A `VariableBlueprint` is a scalar (a scalar, vector or GLSL matrix type such as `MAT4`) or a matrix (a grid of elements), and either can be repeated per dimension: `repeatCountPerDimension` (outermost first, `{2, 3}` = `a[2][3]`, empty = not repeated, throws on a 0 count) is a parameter of every `addVariableBlueprint` / `addMemberBlueprint` / `addVariable` / `addVariableTo...Layout` overload. `int a[2]` is a scalar with `{2}`, `mat4 m[2]` a `MAT4` scalar with `{2}`. A repeated variable aligns like an array (16 for std140) and its `repeatLayers` (`RepeatLayer` = count + stride, outermost first) are built with `RepeatShape::fromCounts` from the instance size rounded up to the alignment rule: the innermost stride is that, every outer stride is the inner stride times the inner count. `elementSize` is one instance, `size` all of them. `VariableBlueprint::getRepeatOffset(repeatIdPerDimension)` is the byte offset of an instance (throws on a wrong number of ids or an id out of range), and every `ReflectionLayout` set / get (and the `Mesh` / `DescriptorSet` setters on top of them) takes the ids as `repeatIdPerDimension_`. A struct member's `repeatCountPerDimension` works the same way for the struct array. A vertex layout may repeat variables: the pipeline gives one attribute location per instance (all dimensions, last one fastest) and per column.

### Alignment

Every offset and stride of the reflection and descriptor code comes from one of these levels. The STD140 / STD430 rules are the layout's `STDAlignmentLayoutEnum` (`NONE` = 1, `STD140`, `STD430`, see `resolveSTDPaddingRule`), the third comes from the device.

| Level | Rule used | Job |
|---|---|---|
| Variable | STD140 / STD430 rule of the variable (`VariableBlueprint::alignmentRule`) | Lay out its own columns, rows and elements. |
| Struct | STD140 / STD430 rule of each variable (to place it), then the struct's own alignment (`ReflectionLayout::structAlignment`: the largest variable rule, at least 16 for `STD140`) | Place its variables inside itself, then pad its own size to `structAlignment`. |
| Block | STD140 / STD430 rule of a variable member, `structAlignment` of a struct member | Place the members, and pad the block size (to the largest alignment, at least 16 for uniform). |
| Device limit | Dynamic buffer alignment (`minUniformBufferOffsetAlignment` / `minStorageBufferOffsetAlignment`) | Place the blocks: the stride between the blocks of a dynamic descriptor layout (`DescriptorShape::dynamicLayer`, the binding's `.range` stays the unpadded block) and the start of a uniform / storage entry in the static heap (`StaticAllocator::addEntry`), so base offset + dynamic offset is always a multiple. Dynamic entries own their buffer, which starts at 0. |

Which layout gets which rule: a descriptor member's `ReflectionLayout` is `STD140` for uniform and `STD430` for storage descriptor layouts, vertex layouts are `NONE`. The shader has to declare the same block (same order, same types), nothing checks the SPIR-V.

A higher level only decides where the lower one starts and how much room it gets, never what is inside: the device limits place blocks, the block places its members, a struct places its variables and then pads its own size, a variable lays out its own columns, rows and elements.

A repeated variable's `size` includes the padding after its last element (N x stride, as GLSL sizes an array), so the next member starts after it even when its own alignment is smaller. A lone variable has no such tail (`vec3 v; float b;` puts `b` at 12).

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
  `dataTypeToName`); counts `XxxCount` (`BufferTypesCount`). `CopyCountModeEnum` and `MemoryAccessModeEnum` are `enum class` (the short names `NONE` and `UNSET` would clash with `STDAlignmentLayoutEnum::NONE` and the other enums), used as `CopyCountModeEnum::ONE`, `MemoryAccessModeEnum::READ_ONLY`.
- Vulkan create-info structs zero-initialized with `= {}` and filled field by
  field.
- `#pragma once`; everything framework-side in `namespace Graphics` (except
  `Globals`, `Disk`, `Misc/` containers).
- Tabs, Allman braces.
