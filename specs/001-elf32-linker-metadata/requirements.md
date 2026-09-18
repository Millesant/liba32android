# Requirements — ELF32 Linker Metadata

Status: proposed M4 slice

## Goal

Turn the already parsed raw `Elf32_Dyn` entries into validated, linker-facing ELF32 metadata expressed entirely in logical guest-address terms. This slice must make string-table, symbol-table, REL relocation-table, SONAME, and dependency-name offsets safe to consume later without loading dependencies, looking up symbols, or applying relocations.

## Scope

- Consume the ordered `Elf32DynamicEntry` sequence produced by `src/elf/elf32_dynamic.*`.
- Consume the ELF load bias produced by `src/elf/elf32_loader.*`.
- Recognize and validate the first linker-facing tag set:
  - `DT_STRTAB` / `DT_STRSZ`;
  - `DT_SYMTAB` / `DT_SYMENT`;
  - `DT_REL` / `DT_RELSZ` / `DT_RELENT`;
  - `DT_SONAME`;
  - repeated `DT_NEEDED` string-table offsets.
- Rebase pointer-like values into logical guest VAs with checked 32-bit arithmetic.
- Validate referenced guest ranges through `memory::GuestMemory`; do not expose or derive host pointers.
- Preserve the existing structural parser as the owner of raw ordering, unknown tags, and `DT_NULL` termination.

## Non-goals

- Opening or loading any `DT_NEEDED` dependency.
- Reading dependency or SONAME strings into a dependency resolver.
- Symbol lookup, interposition, versioning, or hash-table lookup.
- Parsing the symbol table into a resolved symbol graph.
- Applying ARM relocations or writing relocation results.
- PLT/JMPREL processing.
- RELRO, TLS, constructors/destructors, init/fini arrays, or Android packed relocations.
- GNU hash or SysV hash interpretation.
- Automatic `ET_DYN` guest-VA allocation.
- Any application-specific compatibility behavior.

## Requirements

### R1 — Layer boundary

The linker-metadata layer must remain separate from ELF mapping and raw dynamic-array parsing. It may depend on the raw `Elf32DynamicEntry` representation, `GuestMemory`, and a caller-supplied load bias, but it must not mutate loader mappings or CPU state.

### R2 — Guest-address semantics

Pointer-like dynamic values in the supported set must be converted to logical guest VAs by adding the loader-provided load bias with checked 32-bit arithmetic. A rebased value that exceeds the 32-bit guest address space must be rejected.

No host pointer may cross this API boundary.

### R3 — Supported singleton tags

For the first slice, `DT_STRTAB`, `DT_STRSZ`, `DT_SYMTAB`, `DT_SYMENT`, `DT_REL`, `DT_RELSZ`, `DT_RELENT`, and `DT_SONAME` are singleton metadata. Any repeated occurrence of one of these recognized singleton tags is malformed and must be rejected, even when the repeated raw value is identical.

Unknown/deferred tags remain structurally valid and are not rejected merely for being repeated.

### R4 — Repeated dependency-name offsets

`DT_NEEDED` may occur multiple times. The first slice must preserve its offsets in dynamic-array order as linker metadata, but must not load a dependency or resolve a filename.

Each `DT_NEEDED` offset requires a valid string-table descriptor and must be strictly less than `DT_STRSZ`.

### R5 — String-table contract

`DT_STRTAB` and `DT_STRSZ` form one optional pair.

- Both must be present or both absent.
- If present with a non-zero size, the rebased range `[strtab, strtab + strsz)` must fit the 32-bit guest address space and be readable through `GuestMemory`.
- `DT_SONAME` and every `DT_NEEDED` require the string-table pair.
- `DT_SONAME` and `DT_NEEDED` offsets must be strictly less than `DT_STRSZ`.
- This slice does not require NUL-termination checks or materialize strings; those belong to the string-consumption layer.

### R6 — Symbol-table descriptor contract

`DT_SYMTAB` and `DT_SYMENT` form one optional pair.

- Both must be present or both absent.
- `DT_SYMENT` must equal the ELF32 symbol-entry size of 16 bytes.
- The rebased symbol-table address must fit the guest address space.
- When present, at least one complete 16-byte symbol entry at the rebased base address must be readable through `GuestMemory`.
- Full symbol-table extent/count validation is deferred until a later layer has a trustworthy symbol-count bound from hash/relocation metadata.

### R7 — REL descriptor contract

`DT_REL`, `DT_RELSZ`, and `DT_RELENT` form one optional group.

- All three must be present or all absent.
- `DT_RELENT` must equal the ELF32 REL-entry size of 8 bytes.
- `DT_RELSZ` must be an exact multiple of `DT_RELENT`.
- The rebased REL base plus `DT_RELSZ` must fit the 32-bit guest address space.
- A non-empty REL range must be readable through `GuestMemory`.
- No relocation entry is interpreted or applied in this slice.

### R8 — Deferred tags

Tags outside the first supported semantic set, including `DT_GNU_HASH`, `DT_HASH`, PLT/JMPREL tags, init/fini tags, Android relocation tags, RELRO/TLS-related metadata, and unknown processor/vendor tags, must not make an otherwise valid dynamic array fail solely because they are not yet interpreted.

### R9 — Failure behavior

The metadata builder must report explicit failure categories for at least:

- duplicate recognized singleton tag;
- incomplete required tag pair/group;
- rebased address overflow or range overflow;
- unreadable referenced guest range;
- invalid `DT_SYMENT`;
- invalid `DT_RELENT`;
- `DT_RELSZ` not divisible by `DT_RELENT`;
- string offset outside `DT_STRSZ`.

A failed metadata build must not mutate guest memory.

### R10 — Evidence discipline

The feature is complete only after focused synthetic valid/malformed cases and the reproducible real ARM32 fixture execute successfully in CI. Existing baseline tests and Android `arm64-v8a` cross-build coverage must remain green.

## Acceptance Criteria

- AC1: A valid synthetic dynamic array can produce linker metadata containing rebased string, symbol, and REL descriptors plus SONAME and ordered `DT_NEEDED` offsets.
- AC2: `ET_EXEC` semantics are naturally represented by `load_bias == 0`; non-zero load bias rebases pointer-like values exactly once.
- AC3: every recognized singleton duplicate is rejected.
- AC4: partial string/symbol/REL groups are rejected.
- AC5: rebasing/range overflow and unreadable referenced memory are rejected without guest mutation.
- AC6: `DT_SYMENT != 16`, `DT_RELENT != 8`, and non-integral `DT_RELSZ / DT_RELENT` are rejected.
- AC7: SONAME/NEEDED offsets outside the declared string table are rejected; repeated valid `DT_NEEDED` offsets preserve input order.
- AC8: unknown/deferred tags such as `DT_GNU_HASH` remain tolerated.
- AC9: the existing real ARM32 fixture produces valid linker metadata for its observed STRTAB/SYMTAB/REL/SONAME metadata and still reports no `DT_NEEDED`.
- AC10: the previous 20-test baseline remains green and Android `arm64-v8a` cross-build remains PASS in CI.

## Invariants

- Guest VAs remain logical 32-bit values independent from host pointer identity.
- `GuestMemory` remains the validation/read seam.
- Loader mapping, raw dynamic parsing, linker metadata, dependency loading, symbol lookup, and relocation application remain separate layers.
- Validation never broadens guest memory permissions.
- Deferred/unknown tags are not silently treated as implemented semantics.

## Compatibility / Migration

This feature adds a new internal ELF-layer API. It must not change the existing `Elf32LoadResult` or `Elf32DynamicResult` semantics required by the M3 baseline.

## Open Questions

No unresolved question blocks the first implementation slice. Full symbol-table bounds, hash semantics, NUL-terminated string consumption, PLT relocations, and dependency loading are explicitly deferred to later feature packages.
