# Requirements — ELF32 Linker Strings

Status: proposed M4 slice

## Goal

Safely materialize the SONAME and ordered `DT_NEEDED` names referenced by already validated ELF32 linker metadata, without opening/loading dependencies or beginning symbol/relocation semantics.

## Scope

- Consume `Elf32LinkerMetadata` produced by `src/elf/elf32_linker_metadata.*`.
- Read only the validated STRTAB descriptor plus `soname_offset` / ordered `needed_offsets`.
- Materialize NUL-terminated byte strings from logical guest memory through `memory::GuestMemory`.
- Preserve SONAME optionality and `DT_NEEDED` order/duplicates.
- Bound every individual string scan/materialization with an explicit caller-provided payload-byte ceiling.
- Add focused synthetic malformed-string coverage and use the reproducible real ARM32 fixture to validate its known SONAME.

## Non-goals

- Opening, locating, or loading any `DT_NEEDED` dependency.
- Search-path policy, namespace/link-map management, or dependency deduplication.
- Path canonicalization or filesystem access.
- Symbol-table/hash consumption, symbol lookup/interposition, or versioning.
- Relocation decoding/application, PLT/JMPREL, RELRO, TLS, constructors/destructors, or Android packed relocations.
- Re-validating or replacing the semantic responsibilities of `elf32_linker_metadata`.
- Unicode/UTF-8 validation or normalization.
- Application-specific compatibility behavior.

## Requirements

### R1 — Layer boundary

String consumption must remain a separate layer above validated linker metadata and below future dependency loading.

The layer may depend on `Elf32LinkerMetadata` and `GuestMemory`, but it must not mutate guest mappings, guest bytes, loader state, CPU state, or dependency state.

### R2 — Validated string-table input

If SONAME or one or more NEEDED offsets are present, a string-table descriptor must also be present.

The consumer must defensively reject any supplied SONAME/NEEDED offset that is not strictly less than the declared string-table size, even though the preceding metadata builder already enforces that invariant.

If no SONAME/NEEDED offsets are present, an absent string table is valid and yields an empty result.

### R3 — NUL-terminated bounded strings

Each requested name starts at `string_table.guest_address + offset` and ends at the first NUL byte.

A name must terminate before the declared end of STRTAB and within the caller-provided per-string payload-byte ceiling.

The payload-byte ceiling counts bytes before the terminating NUL. A string with payload length exactly equal to the ceiling is valid when the following byte is NUL.

### R4 — Explicit resource bound

Every string-consumption call must receive an explicit finite maximum payload length. The API must not silently scan or allocate up to an attacker-controlled `DT_STRSZ` without such a caller-selected ceiling.

The first slice must distinguish a string that exceeds the configured ceiling from a string that reaches the end of STRTAB without a terminator.

### R5 — Guest-memory access

All bytes must be read through `memory::GuestMemory` using bounded reads. No host pointer may be derived from a guest VA, including when fastmem is available.

A guest read failure during string consumption must fail explicitly.

### R6 — Address arithmetic

Computing the start address or advancing through a requested string must use checked arithmetic. Any address/range overflow must fail explicitly.

### R7 — Byte semantics

Returned names are byte strings ending before the first NUL.

The layer must not require UTF-8, normalize bytes, rewrite path separators, or otherwise reinterpret the string contents.

An empty name (offset points directly at NUL) is structurally valid in this layer; later dependency/runtime policy may reject it if required.

### R8 — SONAME semantics

When `soname_offset` is absent, the result has no SONAME.

When it is present and valid, materialize exactly one SONAME string.

### R9 — NEEDED semantics

Materialize each `needed_offsets` entry in original dynamic-array order.

Repeated offsets and repeated resulting names must be preserved rather than deduplicated.

Failure of any NEEDED entry fails the whole operation; no partially successful string set is returned as a successful result.

### R10 — Failure behavior

The string layer must report explicit failure categories for at least:

- missing required string-table metadata;
- string offset outside STRTAB;
- address/range overflow;
- guest-memory read failure;
- unterminated string at the end of STRTAB;
- string exceeding the configured payload-byte ceiling.

Failure must not mutate guest memory or return a successful partial SONAME/NEEDED set.

### R11 — Evidence discipline

Completion requires focused synthetic valid/malformed coverage plus the reproducible real ARM32 fixture.

The real fixture must materialize the SONAME configured by `tools/build_arm32_loader_fixture.sh`: `liba32android_loader_fixture.so`. The current fixture has no `DT_NEEDED`, so ordered/repeated dependency-name behavior must be proven synthetically.

Existing linker-metadata tests and Android `arm64-v8a` cross-build coverage must remain green.

## Acceptance Criteria

- AC1: Valid synthetic metadata materializes an optional SONAME and ordered NEEDED strings from STRTAB.
- AC2: Repeated NEEDED offsets/names are preserved in order.
- AC3: An empty string is accepted when its offset points directly at NUL.
- AC4: Non-UTF-8 non-NUL bytes are preserved byte-for-byte; no text normalization occurs.
- AC5: Missing STRTAB when a SONAME/NEEDED offset is requested is rejected.
- AC6: An offset equal to or greater than STRSZ is rejected.
- AC7: A string with no NUL before the end of STRTAB is rejected as unterminated when it does not first exceed the configured length ceiling.
- AC8: A string whose payload would exceed the caller-provided maximum is rejected as too long; a payload exactly at the maximum followed by NUL succeeds.
- AC9: Guest read failure and address arithmetic overflow are rejected without mutation.
- AC10: Failure while reading any NEEDED name does not yield a successful partial result.
- AC11: The real ARM32 fixture materializes SONAME exactly as `liba32android_loader_fixture.so` and still yields zero NEEDED names.
- AC12: Existing host tests remain PASS and the Android `arm64-v8a` cross-build remains PASS in CI.

## Invariants

- Guest addresses remain logical 32-bit values independent from host pointer identity.
- `GuestMemory` remains the only generic guest-byte access seam.
- Loader mapping, structural dynamic parsing, linker metadata validation, string consumption, dependency loading, symbol semantics, and relocation application remain separate layers.
- Untrusted ELF metadata cannot force an unbounded per-string scan/allocation through this API.
- String consumption never broadens permissions or mutates guest memory.

## Compatibility / Migration

This feature adds a new internal ELF-layer API and consumes the existing `Elf32LinkerMetadata` contract. It must not change `Elf32LoadResult`, `Elf32DynamicResult`, or existing linker-metadata semantics.

## Security / Performance Constraints

- All address math must be checked before narrowing to 32-bit guest addresses.
- String scans must use fixed-size/bounded temporary reads rather than allocating a buffer equal to the remaining STRTAB range.
- Materialized host storage must be bounded by the explicit caller-provided maximum payload length per string.
- No filesystem or dependency side effects occur in this feature.

## Open Questions

No unresolved question blocks the first implementation slice. Dependency-name filesystem semantics, search paths, empty-name policy at dependency-load time, and any runtime-wide default maximum string length are explicitly deferred.
