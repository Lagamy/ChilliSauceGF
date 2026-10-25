# TODO

Three plans, in the order they should be done. Plan 1 is independent. Plan 2 is a
prerequisite for plan 3. Each step is meant to be a separate small-to-medium task.
Anything marked **needs approval** is a new abstraction or an ownership/API change.

---

## Plan 1: Staging refactor (`StagingAllocator`)

### Current state (verified in code)

- Initial dynamic uploads: one `StagingHeap` per memory block, sized to the block, mirroring
  the block layout (`BasePage::setStagingDataForUpload`). Pages keep `freeStagingHeaps`
  capped by `maxDormantStagingHeaps`. No churn here.
- Updates: `MemoryManager::setStagingDataForUpdate` reuses a free heap only if
  `heap.size >= size`, otherwise allocates an exact-size heap. When the free list exceeds
  `Globals::maxDormantUpdateStagingHeaps` it destroys all free heaps. This is the churn.
- Static init: `StaticAllocator` has its own single staging heap.
- Staging state is spread over `MemoryManager::uploadStagingHeaps`, `BasePage` free lists,
  `MemoryBlock::stagingHeapId` and `StaticAllocator`.
- Known bugs in `setStagingDataForUpdate` (replaced by this plan):
  - `attemptsToFindSuitable` is never incremented, so `maxAttemptsToUseExistingStagingHeaps`
    has no effect.
  - The dormant-cap branch destroys every free heap, not only the excess.

### Goal

One `StagingAllocator` owning all staging memory, with two directions:

- **UPLOAD** (CPU writes, GPU copies from it): write-combined host-visible memory.
- **READBACK** (GPU copies into it, CPU reads): `HOST_VISIBLE | HOST_CACHED`, buffer usage
  `TRANSFER_DST`. Write-combined memory must not be used for reading (very slow).

Design points (paging mirrors `DynamicAllocator` / `BasePage`):

- Naming: a **staging heap** is one buffer plus its memory (the role of a memory block), a
  **staging range** is one slice of a heap holding one write (the role of a buffer bound into a
  block), a **staging page** groups heaps for one range-size class.
- `StagingAllocator` is configured before `init()` with `addPage(direction, upperBoundForRangeSize,
  unit, heapSize, unit, maxDormantHeaps)`, same shape as `DynamicAllocator::addPage`. Page infos are
  sorted by upper bound and pages are created in `init()`. Adding a page after `init()` throws under
  `ENGINE_DEBUG`, as does an upper bound bigger than the heap size. Pages exist per direction.
- A request for a range goes to the first page whose upper bound fits it (throw if none does),
  so small writes never share heaps with big ones.
- Inside a page: `Pool<StagingHeap>`, one `FreeSpace` interval list per heap (reuse the existing
  `FreeSpace` and its merge-on-free logic), and an alive list. A range is placed in the first heap
  with a fitting interval; if none, a new heap is added. Alignment of the range start is the
  copy-offset alignment the buffer needs.
- A range is freed when the batch that reads/writes it completes (`completedUploadBatchId`). Same
  fence model for both directions, no semaphore.
- A heap with no live ranges goes to the page's dormant list. When dormant heaps exceed the page's
  `maxDormantHeaps`, the oldest are destroyed, only the excess. This is what bounds memory churn.
- `Globals::maxDormantUpdateStagingHeaps` and `maxAttemptsToUseExistingStagingHeaps` become
  unnecessary, since the cap is per page.
- Readback heaps are created lazily on the first read request.

### Steps

1. **(needs approval)** Add `StagingAllocator` and `StagingPage` with UPLOAD only, paged as above
   (`addPage`, `init`, `deallocate`; route the config through `Api.h` next to
   `addPageForDynamicMemoryEntries`). Move `setStagingDataForUpdate` and the page
   `freeStagingHeaps` logic into it. Pages and
   `MemoryManager` stop owning staging heaps. Remove `MemoryBlock::stagingHeapId` and
   `BasePage::releaseStagingHeap` if they become unused.
2. Switch initial dynamic uploads (`setStagingDataForUpload`) and static init to range
   allocations from `StagingAllocator`. Delete the old paths, `maxDormantStagingHeaps` in
   `PageInfo`/`BasePage`/`addPageForDynamicMemoryEntries`, and the two `Globals` tunables above.
3. **DONE (compiled, not run)** **(needs approval)** Add the READBACK direction and the readback API:
   request a read of an entry range, get a handle, query "ready" (batch fence), map and read,
   release the handle. Copy direction is GPU_ONLY buffer to readback range through the same
   batched `ONESHOT` `TRANSFER` pass. `CPU_SHARED` entries are mapped and read directly.
4. Update docs/ARCHITECTURE.md (Memory management section).

### Noticed (leave alone until step 1)

- `DynamicAllocator::addAndUploadEntry` throws only if `size_ > upperBound`, but picks a page with
  `size_ < upperBound`. An entry exactly as big as the last page's bound passes the check, matches
  no page, and hits "How did you get there?". `StagingAllocator` should use `<=` in both places.

### Hazards to respect

- A host write must never land in a staging range that an in-flight copy is reading
  (write-after-read on host memory). Ranges are reused only after their batch completes.
- Readback: the CPU reads only after the batch fence is signaled. Mapped memory that is not
  `HOST_COHERENT` also needs `vkInvalidateMappedMemoryRanges` before reading.

---

## Plan 2: Decouple data containers from the reflection layout

### Problem

`ReflectionLayoutAndDataContainers` (`Reflection/Layout.h`) mixes a shared blueprint with
per-instance data (`PoolMap<DataContainer> dataContainers`). A layout, and especially a
descriptor set layout, should not hold data or staging.

### Goal

The layout is a pure blueprint: `alignment`, `alignmentRule`, `memberBlueprints`, `size`.
Per-instance data is owned by the instance (Mesh, DescriptorSet) or not stored at all.

### Steps

1. **(needs approval, public API / ownership change)** Remove `dataContainers`,
   `createDataContainer`, `setDataContainer` and `DataContainer` use from the layout.
   Rename back to a layout-only struct and fix the `Layout.h` / `Layout.cpp` class-name
   mismatch (`ReflectionLayoutAndDataContainers` vs `ReflectionLayoutAndContainers`).
2. Typed helpers take the destination as a parameter, or return offset and size:
   - `setMember<T>(rContainer_, memberId_, repeatUnitId_, data_)` /
     `getMember<T>(...)`, templated on the container type (see plan 3), or
   - a layout function returning `{offset, size}` of a member for a given repeat unit
     (`repeatUnitId_ * layout.size + member.firstByteId`).
   Keep the `GPUTypeMap<T>` check against the member's `dataType` and the
   `repeatUnitId_ < repeatCount` bounds check (throw `std::runtime_error`).
3. Update users: `Mesh` (`vertexDataContainerId`, `setVerticeDataContainer`,
   `setVertice`, `getVertice`, the `Mesh.cpp:12/23/28` call sites), `DescriptorSet`,
   `DescriptorSetLayout` (member `layoutAndDataContainers`), `ReflectionSystem.h`.
4. Update docs/ARCHITECTURE.md (Reflection section) and any stale comments.

---

## Plan 3: No persistent CPU-side data mirror

### Reasoning

The layout gives every member's offset and size, so any member can be updated partially
without remembering the other bytes. A byte-vector mirror is not needed, for static or
dynamic data. The only extra cost is reading back from `GPU_ONLY` memory, which is covered
by the READBACK direction in plan 1. Host-visible entries are mapped and read directly.

### Two data containers, resolved through templates

The typed helpers from plan 2 (`setMember<T>`, `getMember<T>`) are templated on the
container type. The layout computes the member offset once; the container only provides
"write N bytes at offset" and "read N bytes at offset". A C++20 `requires` clause enforces
that interface at compile time, no virtuals.

- `CPUDataContainer`: `std::vector<char> data` (the existing `DataContainer`).
  Purely CPU-side reflection (gameplay, runtime data unrelated to the GPU). Optional, for
  whoever wants it. `Mesh` and `DescriptorSet` do not use it for GPU data.
- `GPUDataContainer`: owns no bytes. It holds a `PoolId` to a staging block or a
  `CPU_SHARED` memory block, plus a start offset to edit from the correct byte.
  **Put a comment at the top of the file about the lifetime difference:**
  - `GPU_ONLY` entries: a **per-update handle**. It points at a staging range that exists only
    from the write until its batch completes, after which the range is reused. It must not be
    stored (not a `Mesh` member). `MemoryManager` returns a fresh one for every update.
  - `CPU_SHARED` entries: a **persistent handle** (mapped block + offset), valid as long as
    the entry exists.
- Reads are disabled at compile time for the staging variant (upload staging is
  write-combined, reading it is very slow). `GPU_ONLY` reads go through READBACK (plan 1),
  `CPU_SHARED` reads go through the mapped memory.
- Staging range size for updates covers only the dirty bytes (as `setStagingDataForUpdate`
  does today), not the whole entry. For `CPU_SHARED`, the offset is the offset inside the
  entry.

### Goal

After `queueUpload`, `Mesh` (and `DescriptorSet`) hold no copy of the uploaded bytes.

### Design

- `updateMesh<T>` computes `entryOffset = verticeId * layout.size + member.firstByteId`,
  asks `MemoryManager` for a `GPUDataContainer` covering that range, and writes through
  `setMember<T>`.
- Initial vertex data before `queueUpload` is written straight into the upload staging range (confirmed).
- `DirtyInMesh` / `verticeToDirtyId` coalescing existed because copies read from a
  mirror. With a per-update range for each write, there is nothing to coalesce into, so drop
  it for the `GPU_ONLY` path at first. Add range/copy merging back only if the copy count
  becomes a problem. `CPU_SHARED` writes go straight to mapped memory and need no dirty list.
- `Mesh::queueUpdate` goes away or shrinks, depending on how the above lands.
- `getVertice` on a `GPU_ONLY` mesh becomes an async readback (plan 1, step 3).

### Steps

Depends on plan 1 (steps 1 and 2) and plan 2.

1. **DONE (3a)** **(needs approval, new abstraction)** Add `CPUDataContainer` (rename of `DataContainer`)
   and `GPUDataContainer` with the lifetime comment, and the `requires` constraint used by
   the typed helpers.
2. **DONE** **(needs approval, public API / ownership change)** Change `setVertice` / `updateMesh` to
   write through a `GPUDataContainer`. Rework the initial-data path of `queueUpload`.
   Also update the callers: the `updateMesh<T>` call sites in `Api.h` and the uses in
   `src/Demo/Triangle.cpp`.
3. **DONE** Decide the fate of `markVerticeDirty` / `queueUpdate` / `DirtyInMesh` /
   `verticeToDirtyId` (see Design). Check that a staging range is not reused until its batch
   completes.
4. Replace `getVertice` with a readback request for `GPU_ONLY` entries and a direct map
   for `CPU_SHARED`. Update `Api.h` accordingly (`updateMesh<T>`, any getter).
5. Remove the now-unused container code from `Mesh` / `DescriptorSet`.
6. Update docs/ARCHITECTURE.md (Reflection and Mesh updates sections).

---

## Notes

- Compile errors are expected mid-refactor.
- Build check after each step: `cmake --build build`, first 20 error lines only.
