# ELF32 dependency resolution

Status: feature 021 ordered provider chain complete; exact-head implementation CI PASSed

## Boundary

`elf32_dependency_resolver` sits above bounded linker-string materialization and below dependency graph/loading, symbol, and relocation layers. Automatic single-image `ET_DYN` placement now exists separately; this resolver intentionally still stops at host-owned image acquisition.

It consumes:

- host-owned ordered `Elf32LinkerStrings::needed` names;
- a caller-owned `Elf32DependencyProvider`;
- an optional borrowed opaque requester identity used only during synchronous provider calls;
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
    | optional requester identity + exact request bytes
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

Feature 021 adds an optional generic `Elf32DependencyProviderChain` between the
resolver and concrete providers. The caller supplies a finite ordered borrowed
provider span; this is sufficient to place an application-local provider before
a platform provider without teaching the resolver either provider's policy.

## Provider contract

The original `Elf32DependencyProvider::resolve` receives the exact requested
dependency bytes plus a finite image-byte ceiling. Feature 020 adds
`resolve_for(requester_identity, requested_name, max_image_bytes)`. Its
default implementation delegates to `resolve`, so existing context-free
providers remain source-compatible and behaviorally unchanged.

`Elf32DependencyResolveOptions::requester_identity` is an optional borrowed
byte string. The resolver forwards it unchanged to every ordered provider
occurrence and never stores it in successful output. Callers must keep its
storage valid only for the synchronous resolve call.

The resolver does not normalize, UTF-8 validate, rewrite separators, prepend directories, or otherwise reinterpret the requester identity or a non-empty dependency name.

An empty dependency name is rejected before provider invocation.

On success the provider supplies:

- a non-empty opaque identity byte string;
- a non-empty owned dependency image.

Provider identity is not interpreted as a path, SONAME, or link-map identity in this layer.

### Ordered provider chains

`Elf32DependencyProviderChain` borrows an ordered
`std::span<Elf32DependencyProvider* const>`. Provider objects and the span
backing storage must outlive the chain.

Lookup rules are strict:

- `NotFound` continues to the next provider;
- `Failed` returns immediately;
- the first success returns immediately;
- an empty chain returns `NotFound`;
- a null provider entry returns `Failed` instead of being skipped.

Both context-free and requester-aware calls are supported. Direct
context-free chain calls invoke each child's `resolve`; requester-aware calls
invoke each child's `resolve_for`. Requester identity, requested-name bytes,
and the resolver-computed image ceiling are forwarded unchanged to every
attempted child. Legacy child providers continue through feature 020's default
requester-aware fallback. The chain does not validate or
rewrite successful child results; the existing resolver performs identity,
image, and byte-ceiling validation.

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

The higher `elf32_dependency_loader` layer consumes this resolver's host-owned results. For every object it passes that graph object's exact owned identity as requester context before acquiring its direct dependencies. Within one graph-loading call it then uses provider result identity as the object key, preserves ordered/repeated dependency edges, terminates cycles by reusing already known identities, automatically places/loads first-seen `ET_DYN` dependencies, runs each new object's dynamic → metadata → string pipeline, and rolls back graph-owned mappings on aggregate failure. The same requester propagation applies to persistent link-map appends.

The resolver itself still does none of that work. This preserves the acquisition boundary and the project invariant that guest virtual addresses are logical 32-bit values independent of host-pointer identity. Persistent link-map lifetime is implemented downstream, while Android namespace/search policy remains external even though feature 020 now supplies the requester-context seam it requires. Symbols, relocations, and execution remain separate concerns.

## Feature 021 validation

Result revision `4324883faef008810bcf77c390eecd92c16718cd`
PASSed Linux A32 smoke check `108371320947`, Android x86_64
address-space probe check `108371320869`, and Android arm64-v8a cross-build
check `108371320992`.

Focused resolver tests cover NotFound fallback, success/hard-failure
short-circuit, exact requester/request/limit forwarding, context-free child
dispatch, empty/null chains, legacy child-provider compatibility, and
preservation of resolver validation.

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
- successful result ownership independent of provider temporary storage;
- legacy providers exercised through the default requester-aware fallback;
- requester identities containing arbitrary bytes forwarded unchanged;
- repeated requester-aware occurrences retain order and the existing ceilings;
- recursive loader coverage proving root and nested object identities reach the provider.

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


## Feature 020 validation

Result revision `2509dce17e8d1b993325ed810d37a29e1b46df45`
PASSed Linux A32 smoke check `108312595857`, Android x86_64
address-space probe check `108312595898`, and Android arm64-v8a cross-build
check `108312595982`.

Focused coverage proves exact opaque requester-byte forwarding, the default
legacy-provider fallback, ordered/repeated request preservation, nested
requester propagation, and persistent cross-root requester identity handling.
