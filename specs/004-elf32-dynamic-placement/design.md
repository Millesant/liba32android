# Design — ELF32 Dynamic Guest Placement

## Context

The current loader correctly maps ARM ELF32 `ET_DYN` images only when the caller supplies `Elf32LoadOptions::dynamic_base`. The dependency resolver now returns host-owned dependency image bytes, but intentionally stops before mapping because no generic guest-VA placement layer exists.

The loader already computes the facts placement needs: host-page-aligned minimum/maximum `PT_LOAD` extent plus the load-bias congruence imposed by each `p_align`. Duplicating that parser in an allocator would create two definitions of a valid ELF image.

## Chosen Approach

Build the feature in three layers:

1. a generic non-mutating free guest-range search primitive over `MappedGuestMemory`;
2. a reusable immutable ELF load-layout plan extracted from the loader's existing validation/planning path;
3. an ELF32 dynamic-placement function that combines the layout constraints with the free-range search and returns an explicit `dynamic_base`.

`load_elf32` continues to require and consume an explicit base. Placement does not call the loader and does not reserve pages.

## Architecture / Data Flow

```text
host-owned ET_DYN bytes
        |
        v
validated Elf32 load-layout planning
        |
        | minimum_page
        | mapped_span
        | load-bias alignment/congruence
        v
ELF32 dynamic placement
        |
        v
generic free-range search over const MappedGuestMemory
        |
        v
explicit dynamic_base
        |
        v
load_elf32(..., options.dynamic_base)
```

Future dependency-graph code may consume a resolved dependency image, request a placement, and then invoke the existing loader.

## Interfaces / Contracts

### Generic guest-range search

Add a small memory-layer component, conceptually:

```cpp
struct GuestVaSearchOptions {
    std::uint32_t begin{};
    std::uint64_t end_exclusive{};
    std::uint64_t alignment{};
    std::uint64_t alignment_offset{};
};

enum class GuestVaSearchError : std::uint8_t {
    None = 0,
    InvalidRange,
    InvalidAlignment,
    SizeOverflow,
    NoSpace,
};

struct GuestVaSearchResult {
    GuestVaSearchError error{};
    std::uint32_t address{};
    explicit operator bool() const noexcept;
};

GuestVaSearchResult find_free_guest_range(
    const MappedGuestMemory& memory,
    std::uint64_t length,
    const GuestVaSearchOptions& options);
```

The candidate relation is:

```text
candidate % alignment == alignment_offset % alignment
```

Both candidate and length must be host-page compatible. The search is low-to-high first-fit and tests every page in the requested extent with `is_mapped`.

This function owns no ELF concepts and performs no mapping.

### Reusable ELF load layout

Refactor the existing loader planning path so validation produces immutable layout metadata before mutation. Exact type names may vary, but the plan needs at least:

- ELF type;
- entry;
- page-aligned lowest load page;
- page-aligned maximum load end;
- required load-bias alignment;
- validated raw/planned `PT_LOAD` data needed by `load_elf32`;
- validated optional `PT_DYNAMIC` metadata needed by the existing loader.

`load_elf32` consumes that plan rather than reparsing with separate rules.

For power-of-two ELF alignments, the required load-bias alignment is the maximum relevant `p_align` greater than one. Existing validation already rejects non-power-of-two values.

### Dynamic placement

Conceptually:

```cpp
struct Elf32DynamicPlacementOptions {
    std::uint32_t search_begin{0x10000};
    std::uint64_t search_end_exclusive{MappedGuestMemory::kAddressSpaceSize};
};

enum class Elf32DynamicPlacementError : std::uint8_t {
    None = 0,
    InvalidImage,
    NotDynamic,
    InvalidSearchWindow,
    AddressOverflow,
    NoSpace,
};

struct Elf32DynamicPlacementResult {
    Elf32DynamicPlacementError error{};
    std::uint32_t dynamic_base{};
    explicit operator bool() const noexcept;
};
```

The implementation derives:

- `span = maximum_page_end - minimum_page`;
- `alignment = max(page_size, required_load_bias_alignment)`;
- `alignment_offset = minimum_page % alignment`.

It asks the generic range finder for a free `span` beginning at a candidate with the required congruence. The returned candidate is directly usable as `Elf32LoadOptions::dynamic_base`.

The default search floor of `0x10000` leaves the conventional null/very-low guest range unused while remaining overrideable. This is guest-layout policy only; it does not imply host low-VA identity.

## State / Lifetime / Concurrency

All placement objects/results are value types. No global allocator state is retained.

The search observes one synchronous `MappedGuestMemory` state. Because placement does not reserve the result, a later load can fail with `AddressConflict` if another operation maps pages first. Concurrent atomic allocation is explicitly deferred.

## Failure Semantics

- Invalid range/alignment/overflow fails before scanning.
- Malformed ELF returns the corresponding placement-level invalid-image classification while retaining loader-specific detail internally or in a nested error field if useful.
- `ET_EXEC` returns `NotDynamic`.
- Exhaustion returns `NoSpace`.
- No failure mutates guest memory.
- `load_elf32` remains the final authority and may still reject a stale/conflicting placement.

## Security / Compatibility

All guest arithmetic uses 64-bit checked intermediates with explicit 32-bit bounds.

The allocator never invokes host `mmap` and never assumes guest VA == host pointer.

Existing explicit-base loader behavior remains source-compatible.

No search-path, namespace, symbol, relocation, ABI, JNI, or application behavior is introduced.

## Test Strategy

### T001 generic range search

Synthetic mapped-memory tests cover:

- first free range;
- occupied first candidate -> next candidate;
- multi-page occupancy inside candidate;
- non-zero alignment offset;
- 16 KiB-equivalent alignment constraint even on a 4 KiB host;
- invalid window/alignment/overflow;
- no-space;
- proof that search does not alter mappings or permissions.

### Shared loader plan

Move existing loader validation/planning into the reusable plan without changing behavior. Existing loader tests are the regression oracle.

Add direct tests for type, minimum page/span and required alignment.

### Dynamic placement

Synthetic ELF tests cover first-fit, conflict skipping, bounded exhaustion, `ET_EXEC` rejection, malformed input, and placement -> `load_elf32` success.

### Real fixture

Use the pinned NDK-generated ARMv7 fixture, automatically place it, require a valid base preserving `0x4000` load alignment, then load it through the existing loader and verify the same fixture metadata/bytes already covered by integration tests.

### CI

GitHub Actions remains authoritative. Completion requires Linux CTest PASS plus Android arm64-v8a and x86_64 diagnostic cross-builds remaining PASS.

No Android device run is required for the pure guest-VA planning feature.

## Performance Strategy

First-fit is linear in scanned guest pages. The 32-bit guest space is at most 1,048,576 pages at 4 KiB and 262,144 pages at 16 KiB, which is bounded and acceptable for the initial correctness-oriented allocator.

The generic search may skip ahead past an observed mapped page/candidate extent where doing so preserves deterministic first-fit semantics.

## Alternatives Considered

### Put automatic allocation directly inside `load_elf32`

Rejected. It would mix policy with mapping and weaken the existing explicit-base boundary used by tests and future link-map ownership.

### Let the dependency provider choose guest addresses

Rejected. Providers own image acquisition/search policy, not guest-memory layout.

### Duplicate the loader's ELF program-header parser in the allocator

Rejected. Two validation paths would eventually diverge on alignment, overflow, permissions, or segment overlap behavior.

### Reserve pages during placement

Deferred. A reservation would require ownership/commit semantics that the current loader does not understand and would turn its conflict checks against the allocator itself.

## Assumptions

- Placement and immediate loading are synchronous in the first graph implementation.
- ELF `p_align` values accepted by the loader remain powers of two, so the maximum required alignment represents the combined congruence constraint.
- Complete process address-space policy will be specified later.
