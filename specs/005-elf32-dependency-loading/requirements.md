# Requirements — ELF32 Dependency Graph Loading

Status: implemented on draft PR #32; final T005 exact-head convergence gate pending

## Goal

Add the first recursive ELF32 dependency-loading layer above the existing acquisition-only dependency resolver. Starting from one caller-owned root ELF32 image plus an opaque root identity, load the root and its transitive `DT_NEEDED` objects into `MappedGuestMemory`, preserve a deterministic dependency graph, handle repeated/cyclic identities without remapping the same object, and roll back graph-owned mappings on aggregate failure.

This feature stops before symbol lookup, relocations, constructors, TLS/RELRO, execution, or Android/bionic pathname policy.

## Scope

- Accept one owned root identity + root image.
- Load an `ET_EXEC` root at fixed guest addresses or automatically place an `ET_DYN` root.
- Parse each newly loaded object's existing dynamic → linker-metadata → linker-string pipeline.
- Resolve each object's ordered `DT_NEEDED` names through the existing `Elf32DependencyProvider` / `resolve_elf32_dependencies` boundary.
- Automatically place and load dependency images.
- Require dependency images to be `ET_DYN`.
- Use provider identity as the opaque graph-object key for dependency objects; the caller-provided root identity participates in the same identity namespace.
- Preserve ordered/repeated dependency edges even when several edges resolve to one object.
- Detect identity reuse/cycles by identity and reuse the already known object rather than remapping it.
- Retain owned source bytes and validated loader/linker metadata for successfully loaded objects.
- Enforce explicit graph/resource ceilings.
- Roll back every mapping created by this graph-loading call when the aggregate fails.

## Non-goals

- Changing `elf32_dependency_resolver` into a loader or graph owner.
- Android/bionic namespaces, search paths, APK lookup, RPATH/RUNPATH, `LD_LIBRARY_PATH`, requester-sensitive pathname policy, or application-specific lookup policy.
- Symbol-table/hash lookup, symbol interposition/versioning, or link-editor scope rules.
- ARM relocation decoding/application, PLT/JMPREL, Android packed relocations.
- RELRO, TLS, constructors/destructors, `dlopen`, `dlsym`, unload, or execution.
- Defining final process-wide link-map lifetime across independent graph-loading calls.
- Concurrent graph mutation or atomic multi-threaded placement.
- Treating guest addresses as host pointers.

## Requirements

### R1 — Layer boundary

Add a new layer above `elf32_dependency_resolver`, `elf32_dynamic_placement`, and `elf32_loader`.

The acquisition resolver remains acquisition-only and retains its existing ordered-occurrence contract. The new layer owns graph identity, placement/loading, recursion, rollback, and graph result lifetime.

### R2 — Root ownership and identity

The call receives one non-empty opaque root identity and one non-empty owned root image.

The root identity is object index 0 and must use the same identity namespace that the provider returns for references back to the root. The graph loader does not infer root identity from SONAME, pathname, or ELF contents.

The successful graph owns the root image bytes so its result does not borrow caller storage.

### R3 — Object identity, deduplication, and cycles

Within one graph-loading call, equal identity bytes identify the same logical loaded object.

- The first occurrence of an identity creates the object.
- Later occurrences create additional edges to that object and must not map it again.
- An edge to an object currently being loaded is a cycle and succeeds as a graph edge without recursive remapping.
- Repeated names may resolve to one identity or different identities; name equality alone never deduplicates.
- Different requested names may alias the same identity.
- If a later acquisition returns an already known identity with image bytes different from the first image associated with that identity, fail explicitly as an identity/image mismatch rather than silently choosing one version.

### R4 — Deterministic graph and edge ordering

Object index 0 is the root. New objects receive monotonically increasing indices on first identity discovery.

Each object's dependency edges preserve its materialized `DT_NEEDED` order and repetitions. Each edge retains the exact requested-name bytes and the target object index.

Traversal is deterministic depth-first by dependency occurrence after the current object's complete direct dependency set has been acquired by the existing resolver. Provider request order therefore remains the resolver's ordered direct-set behavior; graph recursion order follows dependency occurrence order.

### R5 — Placement and loading

For the root:

- `ET_EXEC` loads at its fixed guest addresses using the existing loader.
- `ET_DYN` uses `place_elf32_dynamic`, then passes the returned `dynamic_base` unchanged to `load_elf32`.

For dependencies:

- every new dependency object must be `ET_DYN`;
- automatic placement must succeed before mapping;
- the exact returned `dynamic_base` is passed to the unchanged explicit-base loader.

Placement remains non-reserving. In this first implementation graph loading is synchronous and performs placement immediately followed by load without concurrent guest-memory mutation.

### R6 — Per-object linker pipeline

After a new object loads successfully:

1. if no `PT_DYNAMIC` exists, treat the object as having no dynamic entries and no dependencies;
2. otherwise parse the loader-validated dynamic segment;
3. build validated linker metadata using the object's load bias;
4. materialize linker strings using an explicit per-string byte ceiling;
5. resolve that object's ordered `DT_NEEDED` set through the existing dependency resolver;
6. traverse the resulting dependency occurrences.

A successful graph node retains the load result, parsed dynamic entries, validated linker metadata, linker strings, identity, owned source image, and ordered dependency edges.

### R7 — Existing provider contract remains unchanged

This feature reuses `Elf32DependencyProvider` and `resolve_elf32_dependencies` without adding requester path/namespace context.

The provider continues to receive exact non-empty dependency name bytes and an explicit image ceiling. Requester-sensitive Android/bionic lookup semantics remain deferred and may require a later provider/API extension.

### R8 — Explicit resource bounds

Every graph-loading call must receive finite caller-selected limits for at least:

- maximum unique objects, including the root;
- maximum recursion depth, with root depth 0;
- maximum total `DT_NEEDED` occurrences traversed across the graph;
- maximum bytes for any one root/provider image;
- maximum total source-image bytes acquired/owned during the call, including the root and repeated provider acquisitions;
- maximum linker-string payload bytes per materialized string;
- the guest search window used for automatic `ET_DYN` placement.

The root is validated against the per-image and total-image byte limits before guest mutation.

A dependency occurrence that resolves to an already known identity still counts toward dependency-occurrence and acquired-byte budgets because the provider call and image acquisition actually occurred.

An edge to an already known object does not consume another unique-object slot and does not trigger deeper recursion.

### R9 — Aggregate rollback

Graph loading is all-or-nothing with respect to mappings created by this call.

On any failure after one or more objects have loaded:

- unmap graph-owned successful object mappings in reverse successful-load order;
- do not unmap or modify guest pages that predated the call;
- publish no successful partial graph;
- clear/discard partial graph-owned metadata/images from the failure result.

The existing loader remains responsible for rolling back its own partially failed object load. If graph rollback unexpectedly cannot unmap a previously successful graph-owned mapping, report an explicit rollback failure.

### R10 — Failure categories and diagnostics

The graph layer must distinguish at least:

- invalid/empty root identity or image;
- invalid resource options / root image limit exceeded;
- unique-object limit exceeded;
- recursion-depth limit exceeded;
- dependency-occurrence limit exceeded;
- dependency acquisition failure (with nested `Elf32DependencyResolveError`);
- dependency identity/image mismatch;
- invalid dependency image / dependency not `ET_DYN`;
- placement failure (with nested placement/load-plan detail);
- ELF load failure;
- dynamic-array parse failure;
- linker-metadata failure;
- linker-string failure;
- rollback failure.

Failure diagnostics must identify the failing object identity when one exists and the requested dependency name when failure arose from a specific edge. No host pointer may appear in graph APIs/results.

### R11 — Graph result contract

Successful output exposes:

- root object index 0;
- a vector of unique loaded objects keyed by opaque identity;
- owned source image bytes per object;
- loader result / guest metadata per object;
- parsed dynamic entries, validated linker metadata, and linker strings per object;
- ordered/repeated dependency edges from each object to target object indices.

The graph does not assign symbol-scope meaning to object-vector order or edge order beyond preserving deterministic discovery and `DT_NEEDED` occurrence structure.

### R12 — Compatibility and isolation

Existing public contracts remain source-compatible:

- `load_elf32` still requires an explicit base for `ET_DYN`;
- `place_elf32_dynamic` remains non-mutating;
- `resolve_elf32_dependencies` remains acquisition-only and occurrence-preserving;
- provider search/path policy remains external;
- guest addresses remain logical 32-bit values independent of host pointer identity.

### R13 — Validation discipline

Completion requires focused synthetic graph coverage, rollback coverage, existing ELF/linker regression coverage, and real-fixture integration.

No Android device run is required for this host-side graph/loading feature. GitHub Actions remains authoritative for Linux tests plus Android arm64-v8a and x86_64 cross-build/probe jobs.

## Acceptance Criteria

- AC1: A dependency-free `ET_EXEC` root loads as object 0 with no edges.
- AC2: A dependency-free `ET_DYN` root is automatically placed and loaded using the exact selected `dynamic_base`.
- AC3: A root with ordered direct dependencies produces edges in the same order and loads each first-seen dependency once.
- AC4: A transitive dependency is loaded recursively and receives its own parsed dynamic/metadata/string state.
- AC5: Repeated dependency occurrences remain repeated edges even when they target one identity.
- AC6: Different requested names resolving to one provider identity produce distinct edges to one mapped object.
- AC7: A cycle back to a currently loading root/ancestor terminates without remapping and preserves the cycle edge.
- AC8: Equal identities with different acquired image bytes fail explicitly and roll back graph-owned mappings.
- AC9: A provider-returned `ET_EXEC` dependency is rejected without publishing a partial graph.
- AC10: Unique-object, depth, dependency-occurrence, per-image, total-image, and per-string bounds are enforced deterministically.
- AC11: A later parse/acquisition/placement/load failure removes mappings created by earlier successfully loaded graph objects while preserving unrelated preexisting mappings.
- AC12: Root/provider image ownership in successful output is independent of provider/caller temporary storage.
- AC13: Missing `PT_DYNAMIC` succeeds with empty dynamic/linker/dependency state.
- AC14: Existing resolver occurrence semantics are unchanged; duplicate/cycle deduplication happens only in the new graph layer after acquisition.
- AC15: The pinned real ARM32 fixture loads through the graph API, retains its required 0x4000 placement alignment when dynamic, yields zero dependency edges, and existing fixture tests remain green.
- AC16: Linux CTest, Android x86_64 address-space probe cross-build, and Android arm64-v8a runtime/diagnostics cross-build all PASS at the final feature head.

## Invariants

- Guest virtual addresses remain logical 32-bit values.
- No host pointer crosses ELF/linker graph APIs as a guest address.
- The dependency provider owns lookup policy; the graph layer owns loaded-object identity/lifetime only for one call/result.
- Provider identity, not request-name equality or SONAME, is the dedup/cycle key in this feature.
- Ordered/repeated dependency-edge semantics are preserved.
- Successful object mappings are unique per identity within one graph.
- Failure never publishes a successful partial graph.
- Symbol/relocation semantics remain downstream.

## Compatibility / Migration

This is an additive higher-level API. Existing loader, placement, dynamic, metadata, strings, and acquisition-resolver APIs remain unchanged.

Future Android-specific provider/search work may extend or wrap the provider contract because this feature intentionally does not pass requester namespace/path context.

Future symbol/link-map work may consume this graph but must not retroactively change its recorded dependency occurrence structure.

## Security / Performance Constraints

- All counts and byte totals use checked wide arithmetic.
- All graph/resource limits are explicit; no unbounded recursion or image aggregation.
- No direct filesystem/archive access occurs in the generic graph loader.
- Every mapped object is validated through the existing ELF32 planner/loader path.
- Depth-first traversal is bounded by caller-selected limits.
- Identity/image consistency prevents one identity from silently referring to multiple ELF images during a call.
- Rollback is restricted to mappings proven to have been created successfully by this graph-loading operation.

## Open Questions

No unresolved question blocks the first implementation slice.

Later features must still define Android/bionic requester-sensitive lookup, process-wide link-map ownership across calls, symbol scope/interposition, relocations, constructors/TLS/RELRO, and runtime unload semantics.
