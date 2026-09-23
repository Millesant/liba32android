# Design — ELF32 ARM REL Relocation Application

## Context

Feature 001 already validates and rebases the main `DT_REL` table but intentionally stops before decoding entries or writing guest memory.

Feature 005 provides one owned loaded-object dependency graph with stable object indexes, logical load bias, linker metadata, and deterministic dependency edges.

Feature 006 provides bounded dynamic-symbol indexing and exact graph-local BFS lookup, but deliberately stops before relocation writes and before requester-specific protected/versioned/global-group semantics.

Feature 007 closes only the first ordinary main-`DT_REL` application boundary.

## Component

Add:

```text
src/elf/elf32_relocation.h
src/elf/elf32_relocation.cpp
```

Dependency direction:

```text
loaded dependency graph
  | object.load.load_bias
  | object.linker_metadata.rel_table
  | object.linker_metadata.{symtab,strtab,hash,version marker}
  v
elf32_relocation
  |-- read-only REL decode / plan
  |-- bounded reference-symbol decode
  |-- graph-local lookup via elf32_symbol_lookup
  |-- precomputed 32-bit writes + original-word snapshots
  v
GuestMemory::write
```

The component must not call the dependency provider, remap/protect memory, or execute guest code.

## Runtime posture

- Architecture: Arm AArch32.
- Object format: ELF32 little-endian `EM_ARM`.
- Relocation form: dynamic `Elf32_Rel` from main `DT_REL`.
- ABI/reference: AAELF32 2025Q4, with explicit Android bionic compatibility override for ARM `R_ARM_GLOB_DAT` addend handling.
- Environment: Android-oriented userspace runtime hosted in AArch64 Android.
- Guest address model: logical 32-bit values through `GuestMemory`; host pointers are forbidden above the memory backend.
- Validation path: Linux synthetic/real-fixture CTest plus existing Android cross-build/probe CI.

## Public model

Initial API shape:

```cpp
struct Elf32RelocationOptions {
    std::uint32_t max_relocations{};
    Elf32SymbolLookupOptions symbols{};
};

struct Elf32RelocationEntry {
    std::uint32_t index{};
    std::uint32_t offset{};
    std::uint32_t info{};
    std::uint32_t symbol_index{};
    std::uint8_t type{};
    std::uint32_t place_guest_address{};
    std::optional<std::uint32_t> original_word;
};

struct Elf32RelocationPlan {
    std::size_t object_index{};
    std::vector<Elf32RelocationEntry> entries;
};

enum class Elf32RelocationPlanError : std::uint8_t {
    None = 0,
    InvalidOptions,
    InvalidGraphObject,
    TooManyRelocations,
    RelocationReadFailed,
    PlaceOverflow,
    UnalignedPlace,
    TargetReadFailed,
    DuplicateTarget,
    UnsupportedRelocationType,
};

Elf32RelocationPlanResult build_elf32_rel_relocation_plan(
    const memory::GuestMemory& memory,
    const Elf32DependencyGraph& graph,
    std::size_t object_index,
    const Elf32RelocationOptions& options);
```

T001 may start with this read-only plan surface. Application/result types are added in later tasks once resolution semantics are implemented.

The public header must document:

- caller-selected limits;
- exact supported relocation types;
- logical guest-address meaning;
- read-only plan vs mutating apply behavior;
- all-or-nothing/rollback semantics for apply;
- bionic-specific `GLOB_DAT` addend policy;
- unsupported protected/version/TLS/IFUNC/PLT boundaries.

## REL decoding

Each table entry is exactly 8 bytes:

```text
+0  r_offset  uint32 little-endian
+4  r_info    uint32 little-endian
```

Decode:

```text
symbol_index = r_info >> 8
type         = r_info & 0xff
```

The metadata layer has already validated table size/divisibility/readability, but relocation code does not trust host packing or pointers and decodes bytes explicitly.

## Place and addend

For a supported write-producing entry:

```text
P = checked_u32(load_bias + r_offset)
```

Require `P % 4 == 0`.

Read four bytes at `P` as little-endian `original_word`.

For REL relocations, `original_word` is the implicit addend `A` for types whose semantics use an addend. It is always retained for rollback.

`R_ARM_NONE` is represented but does not require target access.

## First supported type policy

### R_ARM_NONE

No lookup, no write.

### R_ARM_RELATIVE

Strict first form:

- symbol index must be zero;
- `result = load_bias + A` modulo 2^32.

The project rejects non-zero-symbol RELATIVE entries for now even though modern bionic can still resolve the named symbol before ignoring its value. This keeps the initial contract deterministic and matches the overwhelmingly normal form.

### R_ARM_GLOB_DAT

Resolve `S` and write exactly `S`.

This intentionally follows current bionic rather than the AAELF32 `S + A` wording. Synthetic coverage with nonzero `A` prevents accidental drift back to the generic formula.

### R_ARM_ABS32

Resolve `S` and write `S + A` modulo 2^32.

The resolved guest symbol value is used as-is. No host pointer conversion is permitted.

### Unsupported

All other types fail during planning/resolution before mutation.

## Reference-symbol decoding

A symbol-bearing relocation's symbol index belongs to the relocating object's dynamic symbol table.

To avoid unsafe duplicate parsing logic, T002 may add an additive feature-006 helper that decodes one already-indexed `Elf32_Sym` by index. If added, it must preserve feature-006 byte-explicit decoding and error semantics rather than exposing raw guest pointers.

A reference with symbol index zero is invalid for `ABS32` / `GLOB_DAT` in this first feature.

For nonzero references:

1. validate index < bounded dynamic-symbol count;
2. decode the reference `Elf32_Sym`;
3. read its exact name from the object's validated string table;
4. reject unsupported binding/visibility/type/section/version forms needed for relocation policy;
5. resolve the name with `lookup_elf32_graph_symbol(memory, graph, object_index, name, options.symbols)`.

Protected references are rejected even though feature 006 can return protected definitions, because correct relocation binding can require requester-specific self-binding not modeled by graph-global name lookup.

## Weak references

When the reference is `STB_WEAK` and graph lookup returns only not-found:

- `S = 0` for `ABS32`;
- `S = 0` for `GLOB_DAT`.

Other lookup/index/string failures remain errors.

This behavior is relocation policy, not symbol lookup policy, because unresolved-weak-to-zero depends on the relocation/reference context.

## Plan then mutate

Application has two phases.

### Phase 1 — complete resolution plan

For every entry:

- decode;
- validate type/place/target;
- capture original word;
- resolve any symbol;
- compute final word;
- reject duplicates.

No write occurs until every entry has a final disposition.

### Phase 2 — commit writes

Write final words in REL table order.

Track which writes succeeded.

On later write failure:

1. retain the write failure as `primary_error`;
2. restore previous targets in reverse order using original words;
3. if any restoration fails, return `RollbackFailed`;
4. never return a successful partial application.

No page permission changes are attempted. A target that is readable but not writable therefore fails at commit and exercises rollback.

## Duplicate target rule

Reject duplicate write-producing `P` values before mutation.

Without this rule, the second REL entry's implicit addend could depend on whether planning snapshots before or after the first write, defeating the plan-before-mutate invariant. Real well-formed dynamic relocation tables do not need duplicate-place semantics for this first slice.

## Error model

Expected categories across plan/apply:

- InvalidOptions
- InvalidGraphObject
- TooManyRelocations
- RelocationReadFailed
- PlaceOverflow
- UnalignedPlace
- TargetReadFailed
- DuplicateTarget
- UnsupportedRelocationType
- InvalidRelativeSymbol
- MissingSymbolMetadata
- SymbolIndexOutOfRange
- ReferenceSymbolReadFailed
- ReferenceNameFailed
- UnsupportedReferenceBinding
- UnsupportedReferenceVisibility
- UnsupportedReferenceType
- UnsupportedReferenceSection
- UnsupportedVersioning
- SymbolLookupFailed
- UnresolvedStrongSymbol
- TargetWriteFailed
- RollbackFailed

Nested feature-006 index/lookup/string errors should be retained where useful.

## Ownership / lifetime

The relocation layer borrows:

- `GuestMemory`;
- `Elf32DependencyGraph`.

Plans own only small decoded metadata and original/final 32-bit words.

No provider pointer, host mapping pointer, or guest-memory alias is retained.

The graph must remain immutable while a plan/apply call executes. Concurrent guest mutation is outside this feature.

## Real fixture oracle

CI #208 artifact ID `10745105004` was inspected directly:

```text
Relocation section '.rel.dyn' contains 2 entries:
0x000082cc  R_ARM_GLOB_DAT  sym 2  fixture_bss
0x000082d0  R_ARM_GLOB_DAT  sym 3  fixture_data
```

Both target words in the file are zero.

The real-fixture test should:

1. load through the dependency graph;
2. snapshot mappings/permissions;
3. resolve expected symbol values with feature 006;
4. apply feature-007 relocations to object 0;
5. read `load_bias + 0x82cc` and `load_bias + 0x82d0`;
6. require them to equal resolved `fixture_bss` / `fixture_data`;
7. verify data/BSS content remains correct;
8. verify mappings/permissions remain unchanged.

## Documentation obligations

Because this layer introduces writes and rollback semantics, the public header and durable architecture doc must state:

- what is validated before mutation;
- target/addend interpretation;
- supported relocation formulas;
- Android-specific `GLOB_DAT` choice;
- weak-reference behavior;
- duplicate-target rule;
- rollback guarantees and failure-output validity;
- no-permission-broadening policy.

## Test strategy

### T001 — read-only REL plan

Synthetic cases:

- no REL table -> empty success;
- exact entry decode;
- count limit;
- table read failure despite crafted metadata;
- place overflow;
- unaligned supported place;
- target read failure;
- `NONE` with no target read;
- supported/unsupported type classification;
- duplicate writable target;
- plan is read-only.

Pinned fixture plan:

- count = 2;
- both entries are `R_ARM_GLOB_DAT`;
- symbol indexes 2 and 3;
- linked offsets `0x82cc` / `0x82d0`;
- checked places include actual load bias;
- original words are zero.

### T002 — reference symbol semantics

Synthetic cases for index bounds, exact names, strong/weak, default/protected/hidden, versioned object, unsupported symbol forms, and graph-local scope.

### T003 — application + rollback

Synthetic `NONE`, `RELATIVE`, `GLOB_DAT`, `ABS32`, nonzero-addend bionic `GLOB_DAT`, strong-not-found, unresolved weak, pre-write failure, later write failure rollback, and duplicate target coverage.

### T004 — real fixture apply

Apply the two real `GLOB_DAT` relocations and compare GOT target words to feature-006 resolved symbol guest values.

### T005 — convergence

Add/update `docs/architecture/elf32-relocation.md`, README, state/spec, compare implementation to R1-R15/AC1-AC15, and run final exact-head CI.

## Performance

- One bounded linear pass decodes/plans the REL table.
- Symbol work is hash-directed through feature 006.
- Storage is O(relocation count) for decoded entries plus rollback words.
- No global cache is required initially.

## Alternatives considered

### Apply while decoding

Rejected. A later malformed symbol/type/target could leave partial guest mutation.

### Use host `Elf32_Rel*` pointers

Rejected. Guest addresses are logical and all guest bytes must flow through `GuestMemory`.

### Follow AAELF32 addend semantics for GLOB_DAT

Rejected for this Android-targeted runtime. Current bionic explicitly omits the addend for ARM GLOB_DAT.

### Support JUMP_SLOT immediately

Rejected. The project does not yet validate `DT_JMPREL` / PLT relocation metadata, and lazy binding is a separate contract.

### Temporarily unprotect RX/RO pages

Rejected. Text relocation compatibility and permission transitions require a separate explicit security/compatibility design.

### Let duplicate targets apply sequentially

Rejected for the first feature because it conflicts with complete pre-mutation planning of implicit REL addends.

## Assumptions

- The relocating graph is immutable during one call.
- Supported writes target ordinary 32-bit little-endian data words.
- Feature-006 graph lookup remains the accepted local symbol-scope contract.
- The pinned fixture remains byte-identical unless its source/build recipe changes.
