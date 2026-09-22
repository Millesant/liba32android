# ELF32 dependency resolution

Status: complete; merged through PR #17

## Boundary

`elf32_dependency_resolver` sits above bounded linker-string materialization and below dependency graph/loading, symbol, and relocation layers. Automatic single-image `ET_DYN` placement now exists separately; this resolver intentionally still stops at host-owned image acquisition.

It consumes:

- host-owned ordered `Elf32LinkerStrings::needed` names;
- a caller-owned `Elf32DependencyProvider`;
- explicit caller-selected limits for dependency count, per-image bytes, and total acquired image bytes.

It returns host-owned dependency image inputs only. It does not map those images into guest memory, choose `ET_DYN` bases, recurse dependencies, construct a link map, resolve symbols, apply relocations, or execute guest code.

```text
elf32_loader
    |
    v
elf32_dynamic
    |
    v
elf32_linker_metadata
    |
    v
elf32_linker_strings
    | ordered/repeated DT_NEEDED byte strings
    v
elf32_dependency_resolver
    | exact request bytes
    | explicit acquisition ceilings
    v
caller/platform Elf32DependencyProvider
    | filesystem/archive/package/namespace policy
    v
host-owned dependency ELF image inputs
    |
    v
elf32_dependency_loader
    | provider-identity reuse / cycles / ordered edges
    | automatic ET_DYN placement -> explicit-base elf32_loader
    v
owned loaded-object dependency graph
```

The generic ELF core therefore remains independent from Android filesystem layout, APK packaging, namespace/search-path policy, and application profiles.

## Provider contract

`Elf32DependencyProvider::resolve` receives:

- the exact requested dependency bytes as a `std::string_view`;
- a finite maximum image-byte ceiling for that occurrence.

The resolver does not normalize, UTF-8 validate, rewrite separators, prepend directories, or otherwise reinterpret a non-empty dependency name.

An empty dependency name is rejected before provider invocation.

On success the provider supplies:

- a non-empty opaque identity byte string;
- a non-empty owned dependency image.

Provider identity is not interpreted as a path, SONAME, or link-map identity in this layer.

## Ordering and duplicate semantics

`DT_NEEDED` occurrences remain ordered and occurrence-preserving.

For every input occurrence:

- the provider is invoked once;
- the original request bytes are retained in the successful result;
- repeated names remain repeated.

Neither request names nor provider identities are deduplicated here. Deduplication requires later graph/link-map, namespace, cycle, SONAME, and loaded-object lifetime policy.

## Resource limits

Every aggregate call has explicit finite limits for:

- maximum dependency occurrences;
- maximum bytes for one dependency image;
- maximum total image bytes.

Dependency count is checked before provider access.

For each request the provider receives:

`min(max_image_bytes, remaining_total_image_bytes)`

The resolver then defensively validates the returned image against that exact ceiling even if the provider violates its contract.

A zero per-image budget fails before provider access for a non-empty dependency set. A zero remaining total budget likewise fails before the next provider call.

All byte accounting uses `std::uint64_t`.

## Failure semantics

The resolver distinguishes:

- too many dependencies;
- empty dependency name;
- dependency not found;
- provider failure;
- empty provider identity;
- empty dependency image;
- per-image limit exceeded;
- total-image budget exceeded.

Aggregate output is all-or-nothing. A later failure may occur after earlier provider calls, but a failed resolver result does not publish previously acquired dependencies as a successful partial aggregate.

Provider-internal I/O/cache side effects are outside the transaction boundary.

## Relationship to ELF mapping

The ELF loader still consumes an explicit `Elf32LoadOptions::dynamic_base` for `ET_DYN`, and `elf32_dynamic_placement` can now compute such a base from the shared validated load plan and current guest-memory state.

This dependency-resolution layer deliberately does not call either placement or `load_elf32`; it remains an acquisition-only boundary.

The higher `elf32_dependency_loader` layer now consumes this resolver's host-owned results. Within one graph-loading call it uses provider identity as the object key, preserves ordered/repeated dependency edges, terminates cycles by reusing already known identities, automatically places/loads first-seen `ET_DYN` dependencies, runs each new object's dynamic → metadata → string pipeline, and rolls back graph-owned mappings on aggregate failure.

The resolver itself still does none of that work. This preserves the acquisition boundary and the project invariant that guest virtual addresses are logical 32-bit values independent of host-pointer identity. Process-wide link-map lifetime, Android requester-sensitive lookup policy, symbols, relocations, and execution remain downstream concerns.

## Validation evidence

Synthetic coverage includes:

- ordered successful acquisition;
- repeated dependency names with one provider call per occurrence;
- empty dependency set with zero provider calls;
- dependency-count precheck;
- empty-name rejection before the relevant provider call;
- non-UTF-8 and slash-containing request bytes forwarded unchanged;
- provider NotFound vs generic failure;
- empty provider identity and empty image rejection;
- zero/limited per-image budget;
- zero/limited total-image budget;
- remaining-total budget propagated as the provider request ceiling;
- oversized/buggy provider results rejected;
- later-occurrence failure with no successful partial aggregate;
- successful result ownership independent of provider temporary storage.

The pinned NDK-generated ARM32 fixture is also exercised through:

`loader → dynamic → linker metadata → linker strings → dependency resolver`

The fixture intentionally has zero `DT_NEEDED`. The integration test uses a provider that fails if invoked and requires:

- zero materialized dependency names;
- zero resolved dependency entries;
- zero provider calls.

GitHub Actions run `35406297624` (#118) passed the T003 implementation head. The Linux job reported 26/26 CTest cases passing, including `elf32_dependency_resolution` and `elf32_real_dependency_resolution`; the Android `arm64-v8a` cross-build also passed.

Final exact-head GitHub Actions run `35406975309` (#122) passed on `e2502067ef57c77a6c6c6589f9dc60e4ffc8702f`, with Linux A32 smoke and Android `arm64-v8a` cross-build both PASS. PR #17 was then squash-merged to `bleeding` as `c1f0f30d6fde7c73c93dec83f5808353a838c856`.

## Deliberate limits

This layer does **not**:

- implement Android/bionic search paths or namespaces in generic code;
- open files directly from the generic ELF core;
- recursively resolve transitive dependencies;
- deduplicate names or provider identities;
- detect cycles;
- build a dependency graph or link map;
- choose guest addresses or map dependency ELF images itself (the separate automatic placement primitive is available to a future graph/loader layer);
- parse dependency dynamic metadata as part of resolution;
- consume SysV/GNU hash tables for lookup;
- perform symbol lookup/interposition/versioning;
- decode or apply ARM relocations or PLT/JMPREL;
- process RELRO, TLS, constructors/destructors, or Android packed relocations.
