# Design — ELF32 Dependency Resolution

Status: implemented through T004; final exact-head CI pending

## Context

`002-elf32-linker-strings` now produces host-owned `Elf32LinkerStrings` containing an optional SONAME and ordered/repeated `DT_NEEDED` byte strings.

The next linker boundary needs a way to turn those dependency names into dependency ELF image inputs without embedding Android filesystem/search behavior into the generic ELF layer.

A full dependency loader is not yet a safe first step because the current `elf32_loader` deliberately requires an explicit `Elf32LoadOptions::dynamic_base` for every `ET_DYN` image. The project does not yet have automatic guest-VA allocation, recursive dependency-graph ownership, cycle/dedup semantics, or a link-map abstraction.

This feature therefore resolves/acquires dependency images only. It creates a narrow platform-injection seam that a later graph/mapping layer can consume.

## Chosen Approach

Add a separate component:

`src/elf/elf32_dependency_resolver.{h,cpp}`

Conceptual API:

```cpp
enum class Elf32DependencyProviderError : std::uint8_t {
    None = 0,
    NotFound,
    Failed,
};

struct Elf32DependencySource {
    std::string identity;
    std::vector<std::uint8_t> image;
};

struct Elf32DependencyProviderResult {
    Elf32DependencyProviderError error{Elf32DependencyProviderError::None};
    Elf32DependencySource source;

    explicit operator bool() const noexcept;
};

class Elf32DependencyProvider {
public:
    virtual ~Elf32DependencyProvider() = default;

    virtual Elf32DependencyProviderResult resolve(
        std::string_view requested_name,
        std::uint64_t max_image_bytes) = 0;
};

struct Elf32DependencyResolveOptions {
    std::uint32_t max_dependencies{};
    std::uint64_t max_image_bytes{};
    std::uint64_t max_total_image_bytes{};
};

struct Elf32ResolvedDependency {
    std::string requested_name;
    std::string identity;
    std::vector<std::uint8_t> image;
};

struct Elf32ResolvedDependencies {
    std::vector<Elf32ResolvedDependency> ordered;
};

enum class Elf32DependencyResolveError : std::uint8_t {
    None = 0,
    TooManyDependencies,
    EmptyDependencyName,
    DependencyNotFound,
    ProviderFailed,
    EmptyProviderIdentity,
    EmptyDependencyImage,
    ImageTooLarge,
    TotalImageBytesExceeded,
};

struct Elf32DependencyResolveResult {
    Elf32DependencyResolveError error{Elf32DependencyResolveError::None};
    Elf32ResolvedDependencies dependencies;

    explicit operator bool() const noexcept;
};

Elf32DependencyResolveResult resolve_elf32_dependencies(
    const Elf32LinkerStrings& strings,
    Elf32DependencyProvider& provider,
    const Elf32DependencyResolveOptions& options);
```

Exact type names may vary during implementation, but the contracts below must remain equivalent.

## Architecture / Data Flow

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
    | ordered DT_NEEDED byte strings
    v
elf32_dependency_resolver
    | exact request bytes
    | explicit resource ceilings
    v
caller/platform Elf32DependencyProvider
    | provider-defined lookup/search/archive policy
    v
host-owned dependency image inputs
    |
    v
future dependency graph + guest placement + ELF mapping
```

The generic resolver does not know Android filesystem layout, APK packaging, namespace policy, guest placement, symbol tables, relocations, Dynarmic, or application profiles.

## Interfaces / Contracts

### Input

The resolver consumes only `Elf32LinkerStrings::needed`.

SONAME is intentionally not used to resolve dependencies in this slice.

Each requested name is already host-owned and NUL-free because the preceding string layer copied the payload bytes before the NUL terminator.

### Provider boundary

`Elf32DependencyProvider` is the only object that translates dependency request bytes into image bytes.

The provider receives:

- the exact requested name bytes as `std::string_view`;
- a finite maximum image byte count for that request.

The provider owns all lookup policy. Examples a future provider may implement include:

- an in-memory test map;
- an APK/archive lookup layer;
- Android/bionic-style namespace/search-path behavior;
- an application package resolver.

Those policies are not part of the generic resolver contract.

The provider result owns:

- an opaque non-empty identity byte string;
- an owned `std::vector<std::uint8_t>` containing the dependency image.

The aggregate copies/moves those owned values into its successful result. No borrowed provider memory survives the call.

### Provider identity

Identity is opaque provider metadata for future dependency-graph/link-map work.

This layer does not interpret or deduplicate it.

It may later represent a canonical path, APK member key, namespace-qualified name, content identifier, or another provider-specific stable key.

Identity has byte semantics; UTF-8 is not required.

### Name policy

The string layer deliberately allowed empty names and arbitrary bytes. At this dependency boundary:

- empty names are rejected before provider invocation;
- non-empty arbitrary byte names remain valid requests;
- slash-containing names are forwarded unchanged;
- no UTF-8 validation, path normalization, basename extraction, or separator rewriting occurs.

This makes filename/search semantics an explicit provider concern rather than an accidental generic-core policy.

## Resource-Bound Algorithm

Let:

- `count_limit = options.max_dependencies`;
- `per_image_limit = options.max_image_bytes`;
- `total_limit = options.max_total_image_bytes`.

Algorithm:

1. If `needed.size() > count_limit`, return `TooManyDependencies` without provider calls.
2. If `needed` is empty, return successful empty output without provider calls.
3. Initialize `total_bytes = 0` using 64-bit arithmetic.
4. For each requested name in order:
   1. reject empty name as `EmptyDependencyName`;
   2. compute `remaining_total = total_limit - total_bytes` using checked arithmetic;
   3. if `remaining_total == 0`, return `TotalImageBytesExceeded`;
   4. if `per_image_limit == 0`, return `ImageTooLarge`;
   5. compute `request_limit = min(per_image_limit, remaining_total)`;
   6. call the provider exactly once for this occurrence with the exact name bytes and `request_limit`;
   7. translate provider `NotFound` to `DependencyNotFound`;
   8. translate provider `Failed` to `ProviderFailed`;
   9. require non-empty identity;
   10. require non-empty image;
   11. if image size exceeds `request_limit`, classify:
       - `TotalImageBytesExceeded` when remaining total budget is strictly smaller than the configured per-image limit;
       - otherwise `ImageTooLarge`;
   12. checked-add image size to `total_bytes` and require `total_bytes <= total_limit`;
   13. append one resolved occurrence containing the original requested name, provider identity, and owned image.
5. Publish the completed aggregate only after every occurrence succeeds.

The defensive post-provider size check remains mandatory even though the provider was given a byte ceiling.

## Ordering and Duplicate Semantics

The first dependency-resolution slice preserves occurrence semantics exactly.

For each `needed[index]`:

- invoke the provider once;
- append one successful resolved entry at the same ordinal.

Repeated names are not deduplicated.

The generic resolver also does not deduplicate identical provider identities.

This is intentional. Deduplication requires dependency-graph/link-map semantics, SONAME/identity policy, cycle handling, and loaded-object lifetime rules that do not belong in this acquisition boundary.

A provider may use an internal cache to avoid repeated filesystem/archive I/O, but that caching must not change the observable request order or number of logical provider resolutions seen through a test provider.

## Failure Semantics

The aggregate is all-or-nothing.

On any failure:

- return the explicit error;
- return no successful resolved dependency entries;
- do not call `load_elf32`;
- do not mutate guest memory;
- do not publish a partial link map or dependency graph.

Provider calls that already occurred cannot be undone. Providers should therefore avoid externally visible side effects beyond dependency acquisition, and the generic resolver does not promise transactional rollback of provider-internal I/O/cache state.

## State / Lifetime / Concurrency

The first implementation is synchronous and single-call oriented.

The resolver retains no global state and no provider pointer after return.

Successful results own all requested names, provider identities, and image vectors required by the next layer.

There is no concurrent provider invocation in this slice.

## Relationship to Guest Placement

The current ELF loader requires an explicit `dynamic_base` for `ET_DYN`.

This feature deliberately stops before `load_elf32`.

A later feature must decide guest-VA placement/allocation before a general dependency graph can be mapped safely.

That later layer may consume each `Elf32ResolvedDependency::image` and supply an explicit base to the existing loader, or introduce a separate allocator while preserving the current logical guest-VA contract.

## Security / Compatibility

- Dependency count is bounded before provider access.
- Every provider request carries an explicit finite image-byte ceiling.
- Aggregate total image bytes are bounded.
- All resource arithmetic uses wide checked counters.
- Generic code performs no filesystem or archive access.
- No guest pointer/host pointer identity assumption is introduced.
- No guest mapping or code execution occurs.
- Arbitrary non-empty byte names remain compatible; encoding policy is not invented here.
- Existing loader/dynamic/metadata/string APIs remain unchanged.

## Test Strategy

### Synthetic provider tests

Use a deterministic fake provider that:

- records request names in order;
- records each supplied byte ceiling;
- returns configured identities/images/errors;
- counts calls.

Cover:

- ordered successful dependencies;
- repeated names and one provider call per occurrence;
- empty dependency set with zero calls;
- empty name rejected before provider call;
- non-UTF-8 name bytes forwarded exactly;
- slash-containing name forwarded exactly;
- provider NotFound;
- provider Failed;
- empty identity;
- empty image;
- count limit;
- per-image limit;
- total image limit;
- malicious/buggy provider returning an image larger than its supplied ceiling;
- later occurrence failure returns no successful partial aggregate;
- successful result owns image bytes independent of provider temporaries.

### Real fixture integration

Extend/add a fixture-backed integration path:

1. load the pinned ARM32 fixture;
2. parse its dynamic array;
3. build linker metadata;
4. build linker strings;
5. call dependency resolution with a fake provider that would fail if invoked;
6. require successful empty resolved dependencies;
7. require zero provider calls.

The current fixture intentionally has no `DT_NEEDED`, so positive acquisition behavior remains synthetic in this feature.

### CI

GitHub Actions remains authoritative.

Completion requires:

- synthetic dependency-resolution tests PASS;
- real fixture zero-dependency integration PASS;
- existing loader/dynamic/metadata/string tests PASS;
- reproducible ARM32 fixture generation PASS;
- exact `liba32android.so` output-name check PASS;
- Android `arm64-v8a` runtime/diagnostics cross-build PASS.

No Termux/device run is required for this host-side acquisition boundary.

## Performance Strategy

Runtime work is linear in the number of dependency occurrences plus the image bytes returned by the provider.

No recursive graph traversal occurs.

The resolver does not duplicate an image beyond the ownership transfer/copy required by the provider/result API.

Providers are free to cache acquisition internally, but semantic duplicate preservation remains unchanged.

## Alternatives Considered

### Open dependency files directly inside the ELF core

Rejected.

It would prematurely bake Android/filesystem/search behavior into the generic linker and make tests/device-specific behavior harder to isolate.

### Implement Android/bionic search paths now

Rejected.

Namespace/search semantics are platform policy and broader than the next bounded generic linker slice. An injected provider leaves room for a later Android-specific implementation.

### Load each returned image immediately with `load_elf32`

Rejected for this slice.

The current loader requires an explicit `ET_DYN` guest base, and the project has not yet designed general dependency guest-VA allocation, graph lifetime, deduplication, or cycles.

### Deduplicate repeated `DT_NEEDED` names during acquisition

Rejected.

Name equality is not enough to define loaded-object identity across namespaces, provider identities, aliases, or SONAMEs. Preserve occurrences until a link-map/graph policy exists.

### Return borrowed spans into provider storage

Rejected.

Provider storage lifetime would leak into the linker contract. Host-owned result images are simpler and restart-safe for later parsing/loading.

### Reject slash-containing or non-UTF-8 names in generic code

Rejected.

Those are provider/platform pathname policies. The generic boundary only rejects the structurally useless empty dependency name.

## Assumptions

- `Elf32LinkerStrings` has already materialized dependency names safely and bounded each name.
- Dependency images can be represented as host-owned byte vectors at this boundary.
- The caller can choose finite dependency-count/per-image/total-image limits appropriate to its environment.
- A later feature will define guest placement and recursive dependency-graph/link-map semantics before general dependency loading begins.
