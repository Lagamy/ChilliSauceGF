# TODO

## Shader reflection guardrails

Goal: shaders and C++ descriptor declarations stay decoupled, and mismatches become readable errors at pipeline creation. Reflection only reads compiled SPIR-V, it generates nothing. GLSL stays the source.

Start after push constants, images and samplers are done (the checks should cover them too).

Needs approval before starting: the reflection dependency (SPIRV-Reflect, one C file + header, is the lightest candidate) and whether `Shader::addDescriptor` stays a declaration that is checked or becomes derived.

1. Dependency + load: add the reflection library, reflect the `.spv` in `Shader` when it is created, store the result on the `Shader` (sets, bindings, types, stages, block members, vertex inputs, stage outputs, push constants). No checks yet.
2. Descriptor checks: every `set`/`binding` the shader uses exists in the set layout with the same descriptor type (uniform/storage, dynamic or not), and the binding's stage flags include the using stage. Runs in `getOrAddPipelineLayout`.
3. Declared vs used: the descriptors listed by `Shader::addDescriptor` equal what the SPIR-V reads (this is what makes the `drawMeshIndexed` draw-skip trustworthy). Decide check vs derive here.
4. Block layout checks: member order, types, offsets, repeat counts and block size match `DescriptorShape` / `ReflectionLayout` (replaces the "declaration order has to match the shader" contract in ARCHITECTURE.md).
5. Vertex input checks: attribute `location` and format match the vertex layout and the pipeline's stride/offsets (`GraphicsPipeline::create`).
6. Stage interface check: vertex outputs match fragment inputs (location and type).
7. Image/sampler and push constant checks: same pattern as 2 and 4 for the new descriptor types and push constant ranges.
8. Docs: update ARCHITECTURE.md (the "nothing checks the SPIR-V" notes, `Shader` section) once the checks exist.

Limit: reflection checks layout and bindings, not runtime data. Validation layers still cover the rest.

## Later (not scheduled)

- Data-driven command recording (about 0.5-1 descriptor-system-sized units), then derivable sync in stages: buffers on a single queue, hazard tracking, images and attachments, multi-queue. Images and dynamic rendering come first.
- Enable the synchronization validation layer before the sync work starts.
- Multi-draw indirect.
- Bindless descriptors.
