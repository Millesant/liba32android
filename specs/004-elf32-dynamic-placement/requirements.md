# Requirements — ELF32 Dynamic Guest Placement

## Goal

Add a deterministic, game-agnostic way to choose a valid guest virtual-address placement for ARM ELF32 `ET_DYN` images so dependency images can later be mapped without callers inventing bases.

The feature must preserve the existing loader contract that `load_elf32` receives an explicit `dynamic_base`; automatic placement is a separate layer that computes that value.

## Scope

- Discover the page-mapped extent and load-bias congruence requirements of a validated ARM ELF32 `ET_DYN` image.
- Search a caller-bounded 32-bit guest-VA window for a free placement in `MappedGuestMemory`.
- Return one deterministic `dynamic_base` without mutating guest memory.
- Preserve compatibility with both 4 KiB and 16 KiB host page sizes.
- Provide synthetic and real-fixture regression coverage.

## Non-goals

- Mapping the selected image as part of placement.
- Recursive `DT_NEEDED` graph traversal.
- Dependency deduplication, cycle detection, link-map or namespace semantics.
- Android filesystem/search-path policy.
- Symbol lookup/interposition/versioning.
- ARM relocations, PLT/JMPREL, RELRO, TLS, constructors/destructors.
- Stack/heap/TLS/bridge layout policy for a complete process.
- Guest pointer == host pointer identity.

## Requirements

### R1 — Reuse loader validation semantics

Placement MUST derive its image layout from the same ELF validation/planning rules used by `load_elf32`; it MUST NOT maintain a second divergent parser for `PT_LOAD` validity, page extents, or alignment constraints.

Malformed images MUST fail before any guest-memory mutation.

### R2 — Deterministic bounded search

The allocator MUST search only inside an explicit caller-selected guest-VA window.

For identical memory state, image layout, page size and options, it MUST return the same result.

The first policy is low-to-high first-fit. Search-window end is exclusive.

### R3 — Preserve loader `dynamic_base` semantics

The selected value is the guest address where the image's lowest page-aligned `PT_LOAD` mapping begins.

The resulting load bias MUST:

- fit in 32 bits;
- keep every mapped `PT_LOAD` byte inside the 32-bit guest address space;
- satisfy host-page alignment;
- preserve every ELF `PT_LOAD p_align` congruence requirement.

### R4 — Respect existing guest mappings

A candidate is valid only when every guest page required by the image is currently unmapped in `MappedGuestMemory`.

Existing mappings MUST never be replaced, moved, protected differently, or otherwise mutated by placement.

### R5 — Placement is non-mutating

Successful placement returns metadata only. It MUST NOT reserve/map pages as a side effect.

The subsequent loader remains authoritative and MUST still detect an `AddressConflict` if memory changes between placement and load.

This feature is synchronous; it does not claim concurrent allocation atomicity.

### R6 — Explicit failure classification

Placement MUST distinguish at least:

- malformed/unsupported ELF layout;
- non-`ET_DYN` input;
- invalid search window;
- placement arithmetic overflow;
- no fitting free range.

No successful base may be returned with an error.

### R7 — Page-size independence

Placement MUST use `MappedGuestMemory::page_size()` rather than assuming 4096.

Synthetic validation MUST exercise alignment logic above the host-page granularity. The existing real ARMv7 fixture with `PT_LOAD p_align=0x4000` remains the integration oracle.

### R8 — Layering

Guest placement belongs between dependency acquisition and ELF mapping.

The dependency provider remains responsible only for image acquisition. The loader remains responsible only for validating/mapping an explicit placement. No Android path/search policy enters the generic allocator.

## Acceptance Criteria

- AC1: a free synthetic `ET_DYN` image receives the expected first-fit `dynamic_base`.
- AC2: occupied candidate pages are skipped and the next valid congruent placement is selected.
- AC3: an image requiring 16 KiB `p_align` receives a base whose load bias satisfies that alignment.
- AC4: a bounded window too small or fully occupied returns `NoSpace` and leaves memory unchanged.
- AC5: malformed or `ET_EXEC` input is rejected without mapping guest pages.
- AC6: the selected base can be passed unchanged to `load_elf32` and the image loads successfully.
- AC7: the real ARM32 NDK fixture is automatically placed and loaded; its observed `p_align=0x4000` constraints are preserved.
- AC8: existing loader tests and Android cross-builds remain PASS.

## Invariants

- Guest VAs remain logical 32-bit values.
- Host pointers never cross the placement/ELF API boundary.
- `MappedGuestMemory` remains the mapping authority.
- Placement does not weaken ELF permission validation.
- D-0003 and D-0004 remain unchanged.

## Compatibility / Migration

The existing explicit `Elf32LoadOptions::dynamic_base` API remains valid and unchanged.

Automatic placement is additive. Existing callers that already choose a base do not need to migrate.

## Security / Performance Constraints

- All address/size arithmetic uses checked wide intermediates.
- Search work is bounded by the caller-selected window and page count.
- No destructive fixed host mapping is introduced.
- No filesystem access occurs.

## Open Questions

None block the first implementation slice. Full process layout policy and concurrent loaded-object allocation remain downstream concerns.
