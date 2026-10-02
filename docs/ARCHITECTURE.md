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
    Reflection/               ReflectionLayout, MemberBlueprint, DataContainer
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
  update staging heaps and a queue of pending `UPDATE`/`REMOVE` operations that
  `resolvePendingOperations()` applies each frame once the entry's upload fence
  is signaled.
- `StaticAllocator`: entries are added before allocation (throws if already
  allocated). `allocateAndUpload()` creates one `GPULocalHeap` (one memory block,
  one `VkBuffer` per `BufferTypeEnum`, bound at aligned offsets), one
  `CPUSharedHeap`, and one `StagingHeap`, then enables a `ONESHOT` `TRANSFER`
  pass that copies staging → GPU. `checkUploadsStatus()` polls its fence.
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
- Each dynamic page has its own `maxDormantStagingHeaps` (given in `addPage`,
  carried by `PageInfo`). A page's staging heaps are
  all `memoryBlockSize` big and live in `MemoryManager::uploadStagingHeaps`.
  `BasePage::setStagingDataForUpload` links the entry's `MemoryBlock` to a heap
  (`MemoryBlock::stagingHeapId`, taken from `freeStagingHeaps` or newly added),
  stages the entry at the same offset its buffer has in the block
  (`stagingData.stagingOffset`; `MemoryEntry::upload` stages byte N of the entry
  at `stagingOffset + N` and copies from the same position; update heaps from
  `setStagingDataForUpdate` cover only the updated range, so they have
  `stagingOffset` 0 and the range starts at heap position 0), and counts it in the
  heap's `pendingUploadsCount`.
  `MemoryManager::resolveInProgressOperations()` (called before
  `resolvePendingOperations()`) decrements that count once the entry's upload
  batch is complete. When `pendingUploadsCount == 0`,
  `BasePage::releaseStagingHeap` unlinks the heap from the block and either
  keeps it in `freeStagingHeaps` or destroys it if `maxDormantStagingHeaps` free
  heaps already exist.
- `Api.h` routes `addAndUploadMemoryEntry`, `updateMemoryEntry`, `removeUpload`,
  `getMemoryEntry` and `isUploadInGPU` to `MemoryManager::addEntry`,
  `updateEntry`, `removeDynamicEntry`, `getEntry` and `isUploadInGPU`.
- `MemoryEntry` holds both static and dynamic fields.
- `MemoryManager::getBuffer(entry)` returns the `Buffer` an entry lives in: the
  page buffer for dynamic entries (whole buffer), or the static heap buffer for
  the entry's type and visibility (`StaticAllocator::getBuffer`; add
  `inBufferFirstByte`). `drawMeshIndexed` and `MemoryEntry::upload` use it.
- `MemoryManager::updateEntry` defers every `GPU_LOCAL` update (static or
  dynamic) as an `UPDATE` operation, applied once the entry has no upload in
  flight; `CPU_SHARED` entries are written immediately.
- `GPU_ONLY` entry updates are batched by `MemoryManager`. `MemoryEntry::upload`
  writes the staging heap and calls `MemoryManager::queueCopy`, which stores a
  `PendingCopy` and returns a batch id kept in `MemoryEntry::uploadBatchId`.
  `updateUploadBatches()` (called at the start and end of
  `resolvePendingOperations()`) starts a batch when the previous one's fence is
  signaled: it moves `queuedCopies` to `recordedCopies`, resets the fence and
  enables one `ONESHOT` `TRANSFER` pass (`uploadQueuedCopiesCMDs`) that records
  one `vkCmdCopyBuffer` per copy. An entry is pending while
  `uploadBatchId > completedUploadBatchId`. No semaphore is used.
- `MemoryVisabilityEnum`: `GPU_ONLY` (device-local, uploaded through staging)
  and `CPU_SHARED` (host-visible|coherent, written with map/memcpy/unmap).

## Reflection

`ReflectionSystem::layoutsPerBufferType[bufferType - 1]` (no INDEX layout) holds
`ReflectionLayout`s. A layout has `MemberBlueprint`s (type, offset, size) and
`DataContainer`s (byte vector × `repeatCount`, e.g. vertices).
`setMemberInDataContainer<T>` checks `T` against the member's type through
`GPUTypeMap<T>` and throws on mismatch or out-of-range repeat index. `Mesh` stores
its vertex data in a data container of its vertex layout.

### Mesh updates

- `Mesh::queueUpload(allocatorType, visibility)` creates the vertex and index
  memory entries (`vbMemoryUploadId`, `ibMemoryUploadId`).
- `updateMesh<T>(meshId, memberId, verticeId, data)` (`Api.h`) writes the member
  into the mesh's data container (`Mesh::setVertice`) and calls
  `Mesh::markVerticeDirty`. Nothing reaches the GPU yet.
- `Mesh::markVerticeDirty` records the changed bytes in `Mesh::dirtyInMeshes`
  (`DirtyInMesh`: first/last vertice id, `inSrcOffset`, `inEntryOffset`, `size`).
  The container and the vertex entry share one layout, so both offsets are equal.
  `Mesh::verticeToDirtyId` maps each dirty vertice to its `DirtyInMesh`, so the
  lookup is a hash lookup of the vertice and its two neighbours, not a scan.
  A change joins an existing `DirtyInMesh` only if its vertice is inside that
  vertice range or directly next to it; otherwise it starts a new one, so bytes
  between distant vertices are not re-uploaded.
- `Mesh::queueUpdate()` calls `updateMemoryEntry` on the vertex entry for each
  `DirtyInMesh`, then clears the list. It is called explicitly (not every frame)
  and throws under `ENGINE_DEBUG` if the mesh was never uploaded.

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
