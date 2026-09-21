# Design — ELF32 Dependency Graph Loading

## Context

The project now has all prerequisites needed to map acquired dependency images without collapsing layer boundaries:

- `elf32_dependency_resolver` acquires bounded host-owned images and opaque provider identities but deliberately does not map them;
- `elf32_dynamic_placement` deterministically selects a loader-ready base for one `ET_DYN` image without mutation;
- `load_elf32` maps one validated image and retains explicit-base semantics;
- the dynamic → linker-metadata → linker-string pipeline can discover one loaded object's ordered `DT_NEEDED` names.

The missing layer is graph ownership: recursively loading dependencies, reusing already known identities, representing cycles/repeated edges, bounding the aggregate, and rolling back prior successful mappings if a later object fails.

## Chosen Approach

Add a separate `elf32_dependency_loader` component that owns one graph-loading transaction.

The graph loader takes an owned root source, a caller-owned provider, `MappedGuestMemory`, and explicit options. It loads/inspects one object at a time, uses the existing acquisition resolver for each object's direct dependency set, deduplicates only after provider identity is known, recursively loads first-seen identities, and returns a self-contained graph on success.

The existing resolver is not modified into a graph loader.

## Architecture / Data Flow

```text
caller-owned root identity + ELF32 bytes
                 |
                 v
       dependency graph loader
                 |
        +--------+---------+
        |                  |
        v                  v
  root load-plan      graph/resource state
        |
   ET_EXEC fixed
   or ET_DYN placement
        |
        v
      load_elf32
        |
        v
   PT_DYNAMIC? ---------------- no --> empty linker/dependency state
        |
       yes
        v
 parse_elf32_dynamic
        |
        v
 build_elf32_linker_metadata
        |
        v
 build_elf32_linker_strings
        |
        v
 resolve_elf32_dependencies  (acquisition-only, unchanged)
        |
        v
 ordered resolved occurrences
        |
        +--> known identity --> append edge only
        |
        +--> new identity ----> register object --> recurse
```

On failure after successful loads:

```text
failure
  |
  v
reverse successful-load order
  |
  v
unmap only graph-owned segment mappings
  |
  v
return failure / no partial graph
```

## Interfaces / Contracts

Exact names may change during implementation, but the intended public shape is:

```cpp
struct Elf32DependencyLoadSource {
    std::string identity;
    std::vector<std::uint8_t> image;
};

struct Elf32DependencyLoadOptions {
    std::uint32_t max_objects{};
    std::uint32_t max_depth{};
    std::uint64_t max_dependency_occurrences{};
    std::uint64_t max_image_bytes{};
    std::uint64_t max_total_image_bytes{};
    std::uint32_t max_string_bytes{};
    Elf32DynamicPlacementOptions placement{};
};

struct Elf32DependencyEdge {
    std::string requested_name;
    std::size_t target_object{};
};

struct Elf32LoadedDependencyObject {
    std::string identity;
    std::vector<std::uint8_t> image;
    Elf32LoadResult load;
    std::vector<Elf32DynamicEntry> dynamic_entries;
    Elf32LinkerMetadata linker_metadata;
    Elf32LinkerStrings linker_strings;
    std::vector<Elf32DependencyEdge> dependencies;
};

struct Elf32DependencyGraph {
    std::vector<Elf32LoadedDependencyObject> objects; // root == 0
};

Elf32DependencyLoadResult load_elf32_dependency_graph(
    memory::MappedGuestMemory& memory,
    Elf32DependencyLoadSource root,
    Elf32DependencyProvider& provider,
    const Elf32DependencyLoadOptions& options);
```

The result/error type should carry a graph-layer error plus nested subsystem error fields where relevant instead of flattening existing error taxonomies.

No result exposes a host pointer as a guest address.

## Identity Model

Provider identity becomes authoritative object identity within one graph-loading call.

The root identity is supplied by the caller and participates in the same byte-identity namespace. This lets a transitive provider resolution refer back to the root and form a cycle without remapping it.

Maintain an internal identity → object-index table.

For a resolved occurrence:

1. if identity is new, create one object record and associate the returned owned image;
2. if identity is already known, compare the newly acquired image bytes against the first image for that identity;
3. mismatch fails as `IdentityImageMismatch`;
4. match appends an edge to the existing object and does not map again.

Request-name equality never defines object identity.

This is deliberately call-local. Process-wide namespace/link-map ownership is later work.

## Traversal / State Machine

Each object has an internal state:

- `Discovered`: identity/image registered, not yet loaded;
- `Loading`: mapped and currently traversing its metadata/dependencies;
- `Loaded`: full per-object pipeline and dependency traversal completed.

Root object 0 starts `Discovered`.

`load_object(index, depth)` behaves conceptually as follows:

1. If `Loaded`, return success.
2. If `Loading`, the caller has reached a cycle; return success without recursion.
3. Require `depth <= max_depth`.
4. Mark `Loading`.
5. Place/load the image according to root/dependency rules.
6. Record this object in successful-load order for rollback.
7. Run the dynamic/metadata/string pipeline.
8. Before provider calls, require the object's direct `DT_NEEDED` count to fit the remaining graph-wide occurrence budget.
9. Invoke `resolve_elf32_dependencies` once for this object's complete ordered direct dependency set, passing remaining graph-wide acquisition budgets.
10. For each resolved occurrence in order:
    - debit actual acquired bytes;
    - resolve/register identity and append the edge;
    - if the target is newly discovered, recursively `load_object(target, depth + 1)`;
    - if target is already `Loading` or `Loaded`, do not remap.
11. Mark `Loaded`.

Because the resolver acquires an object's complete direct set before recursion, provider request order is deterministic per object. Recursive graph processing remains depth-first by occurrence.

## Root and Dependency Loading

### Root

Use `plan_elf32_load` once to classify the root.

- `ET_EXEC`: call `load_elf32` with default options.
- `ET_DYN`: call `place_elf32_dynamic`, then call `load_elf32` with the exact returned base.

The root may have no `PT_DYNAMIC`.

### Dependencies

Dependencies must be shared-object style `ET_DYN` inputs.

Call `place_elf32_dynamic` first:

- `NotDynamic` maps to graph-level `DependencyNotDynamic`;
- other placement failures retain placement/load-plan detail.

On placement success, call `load_elf32` using that exact base.

This keeps `load_elf32` explicit-base behavior unchanged.

## Resource Accounting

Options are validated before guest mutation.

### Unique objects

`max_objects` includes the root. The root requires at least one object slot.

Known-identity edges do not consume additional slots.

### Depth

Root depth is 0.

A new identity reachable from an object at depth `d` is loaded at `d + 1`. Reaching an already-known `Loading`/`Loaded` object does not recurse and therefore does not consume deeper stack depth.

### Dependency occurrences

Maintain a graph-wide 64-bit count of materialized/traversed `DT_NEEDED` occurrences. Repetitions count.

Before calling the resolver for one object, compare its direct count with the remaining budget so failure occurs before provider access.

### Image bytes

`max_image_bytes` applies to the root and every provider image.

`max_total_image_bytes` includes:

- the root image;
- every provider image returned for every occurrence, including duplicate identities, because acquisition actually consumed resources.

The graph keeps the first image for each unique identity in successful output; duplicate acquisitions are compared then discarded.

When resolving one object's direct set, pass the remaining graph-wide byte budget into `Elf32DependencyResolveOptions`. The resolver's existing defensive byte-ceiling checks remain authoritative for that acquisition call.

### Strings

Pass `max_string_bytes` unchanged to the existing linker-string builder for every object.

### Placement

All `ET_DYN` objects use one caller-selected placement window. Successful earlier graph mappings naturally constrain later first-fit placement.

## Per-Object Metadata

On a successful object load:

- preserve the `Elf32LoadResult`;
- if `dynamic_segment` is absent, retain empty dynamic entries / metadata / strings and stop dependency processing for that object;
- otherwise parse dynamic entries;
- build validated linker metadata using the loader-provided `load_bias`;
- materialize strings with the explicit ceiling;
- store these value objects in the graph node.

This makes the graph a useful immutable input for later symbol/link-map work without re-reading host source buffers.

## Rollback

Track object indices in successful mapping order immediately after each `load_elf32` succeeds.

For each successfully loaded object, `Elf32LoadResult::segments` identifies graph-owned mapping ranges via `mapping_start` and `mapping_size`.

On later failure:

1. walk successful-load order in reverse;
2. unmap each loaded segment range in reverse segment order;
3. only use ranges from successful graph-owned load results;
4. never scan/unmap arbitrary guest ranges;
5. return no graph on ordinary rollback success.

The loader already rolls back its own partially failed object load, so the graph rollback list contains only fully successful object loads.

If any expected unmap fails, report `RollbackFailed` and retain the primary failure as diagnostic detail if the result shape allows it.

No concurrent guest mapping mutation is supported during the call; this makes placement→load and rollback ownership deterministic for the first implementation.

## Failure Semantics

Suggested graph-level error families:

- `InvalidOptions`
- `EmptyRootIdentity`
- `EmptyRootImage`
- `ImageTooLarge`
- `TotalImageBytesExceeded`
- `TooManyObjects`
- `MaxDepthExceeded`
- `TooManyDependencyOccurrences`
- `DependencyResolveFailed`
- `IdentityImageMismatch`
- `InvalidImage`
- `DependencyNotDynamic`
- `PlacementFailed`
- `LoadFailed`
- `DynamicParseFailed`
- `LinkerMetadataFailed`
- `LinkerStringFailed`
- `RollbackFailed`

The result should also retain existing nested error enum values for the relevant failing stage and identify the failing identity / requested edge when available.

## State / Lifetime / Concurrency

The graph result owns all object identities, unique source images, per-object metadata, and edges.

The provider pointer is not retained after the call.

No global loaded-object cache or process-wide link map is introduced.

The first implementation is synchronous and single-threaded with respect to graph loading and guest mapping. Atomic concurrent allocation/loading is deferred.

## Security / Compatibility

- Existing bounded acquisition remains in force.
- Aggregate graph counts/bytes/depth are bounded before or at the relevant boundary.
- Malformed dependency ELFs pass through the same validated planner/loader as roots.
- Dependencies cannot smuggle fixed-address `ET_EXEC` mappings into the guest.
- Identity collisions with differing bytes fail instead of silently aliasing.
- Rollback cannot target mappings not recorded from successful graph-owned loads.
- No Android path semantics are invented.
- No guest/host pointer identity assumption is introduced.
- Existing subsystem APIs remain source-compatible.

## Test Strategy

### T001 — Root transaction

Synthetic roots cover:

- dependency-free `ET_EXEC` success;
- dependency-free `ET_DYN` automatic placement + exact-base load;
- no-`PT_DYNAMIC` empty metadata/dependency state;
- malformed root / placement / load failures before partial graph publication;
- post-load dynamic/metadata/string failure rolls the root mapping back;
- preexisting unrelated mapping remains untouched.

### T002 — Direct dependency graph

Synthetic parent/child images and a deterministic recording provider cover:

- ordered direct dependencies;
- repeated names;
- different names aliasing one identity;
- one mapping per unique identity;
- dependency `ET_EXEC` rejection;
- identity/image mismatch;
- object/occurrence/image/string limits;
- provider errors surfaced through nested resolver detail.

### T003 — Recursive graph / cycles / rollback

Synthetic multi-object graphs cover:

- transitive load;
- depth-first deterministic traversal;
- root cycle `A -> B -> A`;
- sibling/shared dependency `A -> B,C; B -> C`;
- deeper depth-limit failure;
- later transitive parse/placement/load/acquisition failure;
- full reverse-order rollback of prior successful graph mappings;
- preexisting guest mappings preserved.

### T004 — Real fixture integration

Load the pinned ARMv7 fixture as a root through the graph API:

- if dynamic, require automatic placement alignment `0x4000`;
- require successful existing dynamic/metadata/string state;
- require zero dependency edges and zero provider calls;
- retain existing explicit-base fixture coverage unchanged.

### CI

GitHub Actions remains authoritative.

Feature completion requires:

- new focused graph tests PASS;
- real fixture graph integration PASS;
- all existing host tests PASS;
- reproducible ARM32 fixture generation PASS;
- exact shared library naming/alignment checks PASS;
- Android x86_64 probe cross-build PASS;
- Android arm64-v8a runtime/diagnostics cross-build PASS.

No device execution is required for this feature.

## Performance Strategy

Expected work is linear in:

- unique objects loaded;
- dependency occurrences acquired;
- source image bytes acquired/compared;
- existing per-object ELF/dynamic/string parsing.

Identity lookup should use a hash map or equivalent near-constant-time structure while preserving object/edge vectors for deterministic output.

Whole-image comparison occurs only when a provider reuses an identity. Resource ceilings bound worst-case comparison bytes.

No optimization may change ordered edge semantics or skip resolver-visible provider calls required by the acquisition boundary.

## Alternatives Considered

### Put recursion inside `elf32_dependency_resolver`

Rejected. It would collapse acquisition policy and loaded-object lifetime/mapping into one layer and break the deliberately completed `003` boundary.

### Deduplicate by `DT_NEEDED` name before provider resolution

Rejected. Names are not loaded-object identity; namespaces/aliases may map different names to one provider identity or the same name to context-specific objects in future provider designs.

### Use SONAME as graph identity

Rejected. SONAME may be missing, duplicated, aliased, or namespace-dependent. The provider already returns an opaque identity intended for later graph/link-map work.

### Ignore bytes from later equal identities

Rejected. Silently accepting different images under one identity makes cycle/dedup behavior ambiguous. Compare with the first image and fail on mismatch.

### Let dependencies be `ET_EXEC`

Rejected for this feature. `DT_NEEDED` objects are modeled as shared objects and must not claim arbitrary fixed guest addresses. The root remains the only place where fixed-address `ET_EXEC` is accepted.

### Reserve addresses during placement

Rejected for now. Immediate synchronous placement→load plus no concurrent mutation preserves the existing allocator/loader contracts without inventing reservation ownership.

### Implement symbols/relocations in the same feature

Rejected. Graph loading is already a feature-scale correctness boundary and produces the loaded object set needed by later symbol/relocation work.

## Assumptions

- The caller can provide a root identity from the same identity namespace used by its provider when cycles back to the root must deduplicate correctly.
- Equal provider identities are intended to denote one stable object during a graph-loading call.
- The existing acquisition resolver's direct-set all-or-nothing behavior is retained.
- The first implementation does not run concurrently with other guest mapping mutations.
- Future symbol/link-map semantics can consume the preserved graph without requiring graph loading to define symbol scope now.
