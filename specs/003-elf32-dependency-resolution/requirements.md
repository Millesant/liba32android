# Requirements — ELF32 Dependency Resolution

Status: implemented through T004; final exact-head CI pending

## Goal

Introduce a bounded, engine-independent dependency-resolution boundary that consumes the ordered `DT_NEEDED` names already materialized by `elf32_linker_strings` and acquires host-owned dependency ELF images through an injected provider, without mapping those images into guest memory or beginning symbol/relocation semantics.

## Scope

- Consume `Elf32LinkerStrings::needed` in original order.
- Introduce an injected dependency provider interface owned by the caller/platform layer.
- Forward each requested dependency name to the provider byte-for-byte.
- Acquire a provider-defined non-empty dependency identity plus host-owned ELF image bytes.
- Preserve one resolved result per `DT_NEEDED` occurrence, including repeated names.
- Require explicit caller-provided bounds for dependency count, per-image bytes, and total acquired image bytes.
- Keep the operation synchronous, deterministic, read-only with respect to guest state, and all-or-nothing on failure.
- Add focused synthetic provider coverage and a real-fixture zero-dependency integration check.

## Non-goals

- Mapping dependency images into `MappedGuestMemory`.
- Choosing `ET_DYN` guest virtual addresses or implementing an automatic guest-VA allocator.
- Recursively walking transitive dependencies.
- Building a link map or dependency graph.
- Deduplicating dependencies by name, SONAME, path, provider identity, or loaded-object identity.
- Detecting dependency cycles.
- Implementing Android/bionic filesystem search paths, namespaces, APK lookup, `LD_LIBRARY_PATH`, RPATH/RUNPATH, or application-specific lookup policy inside the generic ELF core.
- Canonicalizing path separators or interpreting slash-containing names.
- Symbol-table/hash consumption, symbol lookup/interposition/versioning.
- Relocation decoding/application, PLT/JMPREL, RELRO, TLS, constructors/destructors, or Android packed relocations.
- `dlopen`/`dlsym` or runtime unload semantics.
- Executing any acquired dependency.

## Requirements

### R1 — Layer boundary

Dependency resolution must be a separate layer above `Elf32LinkerStrings` and below future dependency mapping/link-map/symbol/relocation layers.

It may consume host-owned materialized dependency names and return host-owned dependency sources, but it must not mutate guest mappings, guest bytes, CPU state, or existing loader/linker results.

### R2 — Provider-owned lookup policy

The generic ELF core must not open filesystem paths directly in this feature.

A caller-supplied provider owns all policy required to turn a requested dependency name into an image, including any filesystem, archive, package, namespace, or search-path behavior.

The core passes the requested name bytes exactly as received from `Elf32LinkerStrings`. It must not normalize, UTF-8 validate, rewrite separators, prepend directories, or otherwise reinterpret non-empty names.

### R3 — Empty-name policy

An empty `DT_NEEDED` name is structurally valid in the string layer but is not a valid dependency request in this layer.

The dependency resolver must reject an empty requested name before invoking the provider.

### R4 — Ordered occurrence semantics

Process `needed` entries in stored order.

Each occurrence is a distinct request. Repeated names must remain repeated in the successful result.

The resolver must not silently deduplicate or cache repeated requests as semantic behavior. A provider may internally cache its own I/O, but from the core's observable contract each occurrence is submitted in order.

### R5 — Provider result contract

For each successful request the provider returns:

- a non-empty provider-defined identity byte string; and
- a non-empty host-owned dependency image.

The core treats provider identity as opaque. It does not assume it is a filesystem path, SONAME, UTF-8 string, canonical name, or globally unique link-map key.

The returned aggregate owns its dependency image bytes; no borrowed provider buffer may escape the call.

### R6 — Explicit resource bounds

Every aggregate dependency-resolution call must receive explicit finite limits for:

- maximum dependency occurrences;
- maximum bytes for any one returned image;
- maximum total bytes across all returned images.

The occurrence limit counts repeated names.

Before calling the provider for a dependency, the resolver supplies an image-byte ceiling no larger than both the configured per-image maximum and the remaining total-byte budget.

The resolver must defensively reject a provider result that exceeds the supplied ceiling even if the provider violated its contract.

All byte-count arithmetic must use checked wide arithmetic.

### R7 — Empty dependency set

An empty `needed` vector succeeds with an empty resolved result.

The provider must not be invoked in that case.

### R8 — Failure categories

The layer must report explicit failure categories for at least:

- too many dependency occurrences;
- empty dependency name;
- dependency not found;
- provider failure;
- empty provider identity;
- empty dependency image;
- dependency image exceeding its supplied byte ceiling;
- total image bytes exceeding the configured total limit.

Provider failure must be distinguishable from dependency-not-found.

### R9 — All-or-nothing aggregate result

Failure of any occurrence fails the complete operation.

A failed result must not expose previously resolved dependencies as a successful partial aggregate.

The provider may already have observed earlier requests, but the core must not publish partial successful output.

### R10 — No loading side effects

This feature must not call `load_elf32`, map memory, choose guest bases, or parse the acquired dependency images.

The existing loader's explicit `ET_DYN` `dynamic_base` requirement remains unchanged. Dependency guest placement is a later feature.

### R11 — Evidence discipline

Completion requires focused synthetic provider tests plus a real-fixture integration check.

Synthetic tests must prove ordered occurrence behavior, duplicate preservation, raw-byte forwarding, provider/error/resource semantics, and all-or-nothing output.

The current pinned ARM32 fixture has zero `DT_NEEDED`; its integration test must prove that the existing loader → dynamic → metadata → strings pipeline yields an empty dependency set and that the dependency provider is not called.

Existing host tests and Android `arm64-v8a` cross-build coverage must remain green.

## Acceptance Criteria

- AC1: Valid ordered dependency names produce the same number of resolved dependency entries in the same order.
- AC2: Repeated dependency names remain repeated and cause one observable provider request per occurrence.
- AC3: An empty dependency set succeeds empty and does not call the provider.
- AC4: An empty dependency name is rejected before provider invocation.
- AC5: Non-UTF-8 bytes and slash-containing names are forwarded unchanged to the provider.
- AC6: Provider not-found and provider-failure results are reported distinctly.
- AC7: A successful provider result with empty identity or empty image is rejected.
- AC8: Dependency occurrence count, per-image byte limit, and total-byte limit are all enforced with checked arithmetic.
- AC9: A provider result larger than the supplied request ceiling is rejected even if the provider violated the contract.
- AC10: Failure on a later dependency occurrence yields no successful partial dependency aggregate.
- AC11: The resolver does not map guest memory, choose `ET_DYN` bases, call `load_elf32`, or parse dependency ELF contents.
- AC12: The real ARM32 fixture yields zero resolved dependencies and zero provider calls after the existing linker-string pipeline.
- AC13: Existing host tests remain PASS and the Android `arm64-v8a` cross-build remains PASS in CI.

## Invariants

- Guest virtual addresses remain logical 32-bit values independent of host pointer identity.
- No dependency provider contract may expose a host pointer as a guest pointer.
- Search-path/filesystem/namespace policy stays outside the generic ELF core in this slice.
- Dependency occurrence order is stable.
- Duplicate handling at this layer is preservation, not deduplication.
- Returned dependency images are host-owned inputs only; they are not loaded objects.
- Symbol lookup and relocation semantics remain downstream.

## Compatibility / Migration

This feature adds a new dependency-resolution API and consumes the existing `Elf32LinkerStrings` contract.

It must not change `Elf32LoadResult`, `Elf32DynamicResult`, `Elf32LinkerMetadata`, or existing linker-string semantics.

The existing explicit `Elf32LoadOptions::dynamic_base` requirement for `ET_DYN` remains unchanged.

## Security / Performance Constraints

- No unbounded dependency-count or image-size acquisition.
- Every provider call receives a finite byte ceiling.
- Total acquired bytes are bounded and overflow-checked.
- No direct filesystem access occurs in the generic resolver.
- No guest-memory writes occur.
- No whole dependency graph or transitive closure is built in this slice.

## Open Questions

No unresolved question blocks the first implementation slice.

Future packages must still decide:

- guest-VA placement/allocation for dependency `ET_DYN` images;
- recursive dependency-graph traversal;
- provider-identity/SONAME deduplication and cycle semantics;
- link-map/namespace ownership;
- Android-specific search-path behavior;
- symbol lookup/interposition and relocation ordering.
