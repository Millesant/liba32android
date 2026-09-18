# Design — ELF32 Linker Metadata

Status: ready for implementation

## Context

M3 already separates three concerns:

1. `elf32_loader` validates/maps the ELF image and returns guest-only load metadata including `load_bias`;
2. `elf32_dynamic` structurally parses the loader-validated `PT_DYNAMIC` bytes into ordered raw `d_tag` / raw-value entries;
3. dynamic-linker semantics are intentionally absent.

The first M4 slice adds a semantic metadata layer between raw dynamic parsing and future dependency/symbol/relocation logic.

## Chosen Approach

Add an engine-independent ELF component, tentatively `src/elf/elf32_linker_metadata.{h,cpp}`.

Its core operation is conceptually:

```cpp
Elf32LinkerMetadataResult build_elf32_linker_metadata(
    const memory::GuestMemory& memory,
    std::uint32_t load_bias,
    std::span<const Elf32DynamicEntry> entries);
```

The function is read-only with respect to guest memory. It recognizes only the first supported semantic tag set, validates relationships among those tags, rebases pointer-like values, validates referenced guest ranges, and returns compact guest-only descriptors.

## Architecture / Data Flow

```text
ELF image
   |
   v
elf32_loader
   |  load_bias + validated PT_DYNAMIC guest range
   v
elf32_dynamic
   |  ordered raw Elf32DynamicEntry values
   v
elf32_linker_metadata
   |  validated guest-VA descriptors / string offsets
   v
future layers
   +--> string consumption / SONAME + DT_NEEDED names
   +--> symbol-table semantics / hash lookup
   +--> relocation decoding/application
```

The new layer does not call the loader, does not remap pages, does not change permissions, and does not depend on Dynarmic.

## Data Model

The implementation should expose small guest-only descriptors equivalent to:

```cpp
struct Elf32StringTableMetadata {
    std::uint32_t guest_address{};
    std::uint32_t size{};
};

struct Elf32SymbolTableMetadata {
    std::uint32_t guest_address{};
    std::uint32_t entry_size{};
};

struct Elf32RelTableMetadata {
    std::uint32_t guest_address{};
    std::uint32_t size{};
    std::uint32_t entry_size{};
};

struct Elf32LinkerMetadata {
    std::optional<Elf32StringTableMetadata> string_table;
    std::optional<Elf32SymbolTableMetadata> symbol_table;
    std::optional<Elf32RelTableMetadata> rel_table;
    std::optional<std::uint32_t> soname_offset;
    std::vector<std::uint32_t> needed_offsets;
};
```

Exact public names may vary, but the contract must remain guest-only and must not store raw host addresses.

## Tag Classification

### Pointer-like supported singletons

These raw values receive exactly one checked addition of `load_bias`:

- `DT_STRTAB`;
- `DT_SYMTAB`;
- `DT_REL`.

### Scalar supported singletons

These remain raw scalar values:

- `DT_STRSZ`;
- `DT_SYMENT`;
- `DT_RELSZ`;
- `DT_RELENT`;
- `DT_SONAME`.

### Repeated supported tag

- `DT_NEEDED`: append each raw string-table offset in dynamic-array order.

### Deferred tags

All others remain outside this layer's semantic output for now. Their presence is tolerated because the structural parser already preserves them.

## Duplicate Policy

Recognized singleton tags are unique. A second occurrence is an explicit metadata error even when the raw values match. This makes ambiguity visible now rather than letting future linker behavior depend on accidental first/last-wins policy.

`DT_NEEDED` is intentionally repeatable.

## Pair / Group Validation

After a single pass collecting raw tag values:

- STRTAB/STRSZ must be both present or both absent.
- SYMTAB/SYMENT must be both present or both absent.
- REL/RELSZ/RELENT must be all present or all absent.
- SONAME or any NEEDED offset requires STRTAB/STRSZ.

This collection pass does not mutate guest state, so failures are side-effect free.

## Address Rebase and Range Validation

Use 64-bit intermediates for every `raw_pointer + load_bias` and `guest_address + size` calculation. Reject results above the 32-bit guest-address-space end before converting to `uint32_t`.

Referenced memory must be validated through `GuestMemory::read`, never by using `fastmem_base()` or constructing host pointers.

To avoid a hostile metadata size causing a giant host allocation, implementation should validate potentially large readable ranges in bounded chunks rather than allocating a buffer equal to the entire guest range.

Zero-length ranges require only arithmetic validity; non-empty ranges require successful reads.

## String Metadata

When STRTAB/STRSZ is present, return its rebased guest base and size.

SONAME and NEEDED values remain offsets rather than strings. Validate `offset < strsz`, but do not scan for a NUL byte in this feature. This keeps string consumption and malformed-string policy in a later focused layer.

Repeated NEEDED offsets are retained in original order.

## Symbol Metadata

ELF32 `Elf32_Sym` entries are 16 bytes, so `DT_SYMENT` must equal 16.

Because this slice intentionally does not interpret `DT_HASH`/`DT_GNU_HASH`, it has no trustworthy whole-table symbol count. Therefore it validates only that the rebased SYMTAB base can supply one complete 16-byte entry and returns the descriptor. Full symbol extent/count belongs to a later symbol/hash feature.

## REL Metadata

ELF32 `Elf32_Rel` entries are 8 bytes.

Require `DT_RELENT == 8` and `DT_RELSZ % 8 == 0`. Rebase `DT_REL`, validate the full declared REL byte range is readable, and return the descriptor. Do not decode `r_offset` / `r_info` and do not perform writes.

## Failure Semantics

Use a result/error model consistent with the existing ELF layers. The error set should distinguish at least:

- duplicate singleton;
- incomplete metadata group;
- address/range overflow;
- unreadable guest range;
- invalid symbol entry size;
- invalid REL entry size;
- invalid REL byte size;
- string offset out of range.

The result is successful when all recognized metadata is internally consistent, even if deferred or unknown tags are present.

## Compatibility / Security

- Never expose host pointers.
- Never change guest mappings or permissions from metadata parsing.
- Do not interpret a deferred tag merely because its value happens to point at readable memory.
- Reject malformed recognized metadata deterministically rather than guessing.
- Preserve M3 behavior and public contracts.

## Test Strategy

### Synthetic tests

Add a focused linker-metadata test covering:

- valid full metadata with non-zero load bias;
- zero load bias for fixed-address semantics;
- repeated ordered NEEDED offsets;
- duplicate singleton rejection;
- each incomplete pair/group;
- rebased pointer overflow;
- range-end overflow;
- unreadable STRTAB/SYMTAB/REL ranges;
- bad SYMENT;
- bad RELENT;
- non-divisible RELSZ;
- SONAME/NEEDED offset out of bounds;
- deferred/unknown tag tolerance;
- proof that validation does not mutate guest bytes.

### Real fixture

Extend or add a real-fixture integration test that:

1. loads the generated ARM32 fixture through `elf32_loader`;
2. parses `PT_DYNAMIC` through `elf32_dynamic`;
3. builds linker metadata with `load_result.load_bias`;
4. confirms valid STRTAB/SYMTAB/REL/SONAME descriptors;
5. confirms no NEEDED offsets for the current freestanding fixture.

### CI

CI is the authoritative build/test environment. Completion requires the new tests plus the existing 20 baseline tests to PASS and the Android `arm64-v8a` cross-build to remain PASS.

Termux remains the separate real-device/runtime validation environment; this metadata-only slice does not require a new device run.

## Alternatives Considered

### Interpret metadata inside `elf32_dynamic`

Rejected. The structural parser deliberately preserves raw entries and unknown tags. Semantic validation belongs in a distinct linker-facing layer.

### Fold metadata interpretation into `elf32_loader`

Rejected. Mapping an ELF must not implicitly begin dynamic-linker semantics.

### Fully parse strings, hashes, symbols, and relocations now

Deferred. That would turn one bounded metadata slice into several coupled linker subsystems and weaken failure isolation.

### Accept duplicate singletons with first-wins or last-wins behavior

Rejected for the supported set. Explicit rejection is deterministic and prevents ambiguous linker state.

## Assumptions

- The caller supplies the same load bias returned by the loader for the image whose dynamic entries are being interpreted.
- `GuestMemory::read` remains the authoritative generic readability check.
- Hash-based symbol count discovery and actual string consumption will be specified separately.
