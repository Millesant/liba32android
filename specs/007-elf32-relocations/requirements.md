# Requirements — ELF32 ARM REL Relocation Application

Status: readiness-checked; T001 ready for implementation

## Goal

Add the first bounded ARM ELF32 dynamic-relocation layer above the completed loader, linker metadata, dependency graph, and graph-local symbol lookup.

The feature must safely consume the already validated `DT_REL` table, decode ELF32 `Elf32_Rel` entries without host-structure assumptions, resolve the first supported relocation forms against logical 32-bit guest addresses, and apply supported writes transactionally through `memory::GuestMemory`.

This first feature is deliberately limited to the ordinary non-versioned `DT_REL` path needed before broader Android dynamic-linker compatibility. It does not absorb PLT/JMPREL, TLS, IFUNC, symbol-version matching, namespace/global-group policy, RELRO, constructors, or guest execution.

## Scope

- Add a separate `elf32_relocation` layer above validated linker metadata and feature-006 graph-local symbol lookup.
- Consume only the validated main `DT_REL` / `DT_RELSZ` / `DT_RELENT` descriptor already exposed by `Elf32LinkerMetadata`.
- Decode each 8-byte ELF32 REL entry explicitly:
  - `r_offset: uint32`;
  - `r_info: uint32`;
  - symbol index = `r_info >> 8`;
  - relocation type = `r_info & 0xff`.
- Compute each relocation place as checked `object.load.load_bias + r_offset`.
- Read the implicit REL addend from the 32-bit little-endian word at the place when the selected relocation type uses it.
- Bound relocation count with an explicit caller-selected ceiling.
- Support the first ordinary ARM relocation set:
  - `R_ARM_NONE` (0): no write;
  - `R_ARM_ABS32` (2): `S + A` modulo 2^32;
  - `R_ARM_GLOB_DAT` (21): Android-compatible `S` semantics, intentionally ignoring the in-place REL addend as current bionic does;
  - `R_ARM_RELATIVE` (23): `B + A` modulo 2^32, with `B` equal to the relocated object's load bias and symbol index required to be zero in this first slice.
- Resolve symbol-bearing relocations from the relocating object's dynamic-symbol entry and then through the existing graph-local BFS lookup starting at that object.
- Treat an unresolved weak reference as `S = 0` for the supported absolute relocation forms.
- Require non-weak unresolved symbols to fail before guest mutation.
- Reject protected-reference relocation semantics in this first feature rather than silently approximating requester-specific self-binding.
- Apply successful relocation writes through `GuestMemory::write`.
- Plan/validate all supported entries before the first write, snapshot original target words, and roll back earlier writes in reverse order if a later write fails.
- Reject duplicate writable relocation targets in one table so transactional planning never depends on a prior relocation's mutation as another entry's implicit addend.
- Integrate the pinned real ARM32 fixture: its two `R_ARM_GLOB_DAT` entries must resolve to the loaded `fixture_bss` and `fixture_data` guest values.

## Non-goals

- `DT_JMPREL`, `DT_PLTREL`, `DT_PLTRELSZ`, PLT/GOT lazy binding, or `R_ARM_JUMP_SLOT`.
- `R_ARM_REL32` or instruction-encoding relocations.
- `R_ARM_COPY`.
- `R_ARM_IRELATIVE` or GNU IFUNC execution.
- TLS relocations or TLS address calculation.
- RELA, RELR, Android packed relocations, APS2, or section-header relocation processing.
- Symbol-version matching.
- `DT_SYMBOLIC` / `DF_SYMBOLIC`, protected self-binding, Android global groups, namespaces, preloads, or process-wide interposition policy.
- Temporarily changing guest page permissions to support text relocations.
- RELRO enforcement.
- Constructors/destructors, `dlopen`, `dlsym`, unload, or guest execution.
- Treating guest addresses as host pointers.

## Requirements

### R1 — Layer boundary

Add `src/elf/elf32_relocation.{h,cpp}` as a separate dynamic-linker layer.

It may consume:

- `memory::GuestMemory`;
- one `Elf32DependencyGraph`;
- one relocating object index;
- that object's validated REL/linker metadata and load bias;
- feature-006 symbol-index/string/graph-lookup contracts.

It must not perform dependency acquisition/loading, choose guest bases, implement pathname policy, or execute guest code.

### R2 — REL table decoding

The validated metadata layer remains responsible for `DT_RELENT == 8`, `DT_RELSZ % 8 == 0`, checked rebasing of `DT_REL`, and readability of the declared REL byte range.

The relocation layer must still:

- reject zero relocation ceiling when a table is present;
- derive count as `rel_table.size / 8`;
- reject count above the caller ceiling;
- decode each entry byte-explicitly as little-endian;
- never cast guest bytes to a host `Elf32_Rel` structure.

An object with no `DT_REL` table succeeds with an empty plan/application result.

### R3 — Place calculation and target access

For a relocation that addresses a 32-bit place:

- compute `P = load_bias + r_offset` with checked 32-bit guest-address arithmetic;
- require 4-byte alignment for supported word relocations;
- require the original 4-byte target word to be readable before mutation;
- retain that original word as the REL addend where required and as rollback state.

No host pointer may be derived from `P`.

### R4 — Resource and duplicate bounds

Every call requires a finite `max_relocations`.

Planning must use checked count/address arithmetic and bounded storage.

Two supported write-producing entries may not target the same 32-bit guest place in one table. A duplicate target is rejected before mutation.

### R5 — R_ARM_NONE

`R_ARM_NONE` produces no write and requires no symbol lookup.

It remains represented in the decoded plan for deterministic evidence/debugging.

### R6 — R_ARM_RELATIVE

For `R_ARM_RELATIVE`:

- require symbol index zero in this first feature;
- use the in-place 32-bit REL addend `A`;
- use the relocating object's load bias as `B`;
- compute the stored word as `B + A` modulo 2^32;
- perform no symbol lookup.

A non-zero symbol index is explicitly unsupported rather than silently assigned Android/bionic edge semantics.

### R7 — Relocation symbol reference

For symbol-bearing supported relocations, the relocation's symbol index refers to the relocating object's dynamic symbol table.

The relocation layer must:

- build/reuse a bounded symbol index under explicit symbol/hash/name/scope ceilings;
- reject a relocation symbol index outside the indexed dynamic-symbol extent;
- decode the referenced `Elf32_Sym` exactly;
- require a valid string-table name offset for nonzero symbol indexes;
- read the exact symbol name through the bounded linker-string reader;
- preserve the reference symbol's binding/visibility/type/section metadata for policy decisions.

Symbol index zero is not a named external reference.

### R8 — Symbol resolution and weak behavior

For a named external reference:

- only ordinary `STB_GLOBAL` / `STB_WEAK`, `STV_DEFAULT` references are supported in this feature;
- `STV_PROTECTED` references are rejected because requester-specific self-binding is deliberately deferred;
- hidden/internal/local/TLS/IFUNC/common/XINDEX/versioned forms remain explicit failures;
- search begins at the relocating object and uses feature-006 deterministic graph-local BFS scope.

If graph lookup finds a definition, `S` is the returned logical guest value.

If graph lookup reports not found and the reference binding is `STB_WEAK`, use `S = 0` for `R_ARM_ABS32` and `R_ARM_GLOB_DAT`.

A non-weak unresolved reference fails before mutation.

### R9 — R_ARM_GLOB_DAT Android semantics

For `R_ARM_GLOB_DAT`, store `S`.

Although AAELF32 specifies `(S + A) | T`, current Android bionic intentionally does not add the REL in-place addend for ARM `GLOB_DAT`. This project's Android-compatibility target follows bionic for this relocation form.

The pinned fixture's two `R_ARM_GLOB_DAT` target words are initially zero, so the real-fixture oracle is consistent with both formulations; synthetic nonzero-addend coverage must lock the deliberate bionic behavior.

### R10 — R_ARM_ABS32

For `R_ARM_ABS32`, store `S + A` modulo 2^32.

The first implementation must preserve the resolved symbol value as returned by feature 006, including any architecture-significant low address bit already present in the ELF symbol value. It must not invent host-pointer or instruction-decoding semantics.

### R11 — Unsupported types

Any relocation type other than `NONE`, `ABS32`, `GLOB_DAT`, or `RELATIVE` fails explicitly before mutation.

This includes `JUMP_SLOT`, `REL32`, COPY, TLS, IRELATIVE, and instruction relocations.

### R12 — Transactional mutation

Application is all-or-nothing with respect to relocation-owned writes.

Before the first write:

- every entry must be decoded;
- all required symbol/string/hash reads and graph lookups must succeed;
- every supported writable target's original 4-byte word must be captured;
- all final 32-bit results must be computed.

Writes occur in table order.

If a write fails:

- roll back prior successful relocation writes in reverse order using the captured original words;
- report `RollbackFailed` if restoration itself fails while preserving the primary write failure separately;
- do not report a successful partial application.

The layer does not alter page permissions to make a failed write succeed.

### R13 — Isolation and compatibility

Existing loader, metadata, dependency, and symbol APIs remain source-compatible except for additive helper APIs if relocation implementation requires indexed-symbol decoding.

Guest VAs remain logical 32-bit values.

No relocation API/result contains a host pointer.

### R14 — Real fixture integration

The pinned NDK r27d / API 26 ARMv7 fixture at SHA-256 `6c2dbda2dec94eaa022ad09391ed3988c3a41828e5c1c065fc0101e5725b84c2` contains exactly two main `.rel.dyn` entries:

- `R_ARM_GLOB_DAT` at linked `r_offset = 0x82cc`, symbol index 2, `fixture_bss`;
- `R_ARM_GLOB_DAT` at linked `r_offset = 0x82d0`, symbol index 3, `fixture_data`.

Both in-place words are zero in the pinned artifact.

After graph loading and relocation application:

- each relocated place at `load_bias + r_offset` must equal the feature-006 graph-local resolved guest value of the named symbol;
- `fixture_data` still reads `0x12345678`;
- `fixture_bss` still reads zero;
- provider calls remain zero for this dependency-free fixture;
- mappings and permissions remain unchanged.

### R15 — Validation discipline

Completion requires:

- synthetic REL decode/count/overflow/read/alignment/unsupported-type coverage;
- `NONE`, `RELATIVE`, `GLOB_DAT`, and `ABS32` success coverage;
- nonzero-addend `GLOB_DAT` test proving bionic-compatible addend suppression;
- symbol-index/name/binding/visibility/unsupported-form failures;
- strong-not-found failure and unresolved-weak-to-zero behavior;
- duplicate-target rejection;
- pre-write failure leaves all guest words unchanged;
- injected later write failure with successful rollback coverage;
- rollback-failure reporting where a test backend can model it;
- pinned real ARM32 fixture `GLOB_DAT` application;
- neighboring ELF/linker/dependency/symbol tests remain green;
- final exact-head Linux CTest, Android x86_64 probe, and Android arm64-v8a cross-build PASS.

No Android device execution is required for this host-side relocation feature.

## Acceptance Criteria

- AC1: A valid bounded `DT_REL` table decodes deterministically into exact `r_offset`, symbol index, type, checked place, and original word/addend data.
- AC2: Missing REL metadata succeeds as empty work; malformed/out-of-bounds/over-limit/unaligned target inputs fail without mutation.
- AC3: `R_ARM_NONE` performs no write.
- AC4: `R_ARM_RELATIVE` with symbol index zero writes `load_bias + A` modulo 2^32 without symbol lookup.
- AC5: `R_ARM_GLOB_DAT` resolves the graph-local symbol and writes `S`, ignoring a nonzero in-place addend.
- AC6: `R_ARM_ABS32` writes `S + A` modulo 2^32.
- AC7: Symbol index bounds and exact symbol-name retrieval are enforced before graph lookup.
- AC8: Strong unresolved references fail pre-mutation; unresolved weak references use `S = 0` for supported absolute forms.
- AC9: Protected/versioned/TLS/IFUNC/common/XINDEX and unsupported relocation types fail explicitly rather than being approximated.
- AC10: Duplicate write targets are rejected before mutation.
- AC11: Any late write failure rolls back earlier relocation writes or reports rollback failure explicitly; no failed call publishes partial success.
- AC12: The pinned real ARM32 fixture's two `R_ARM_GLOB_DAT` places equal the resolved `fixture_bss` and `fixture_data` guest values after application.
- AC13: Guest mappings/permissions are unchanged by successful ordinary data relocation application.
- AC14: Existing loader/dynamic/metadata/string/dependency/symbol behavior remains PASS.
- AC15: Final exact-head Linux CTest, Android x86_64, and Android arm64-v8a CI jobs PASS.

## Invariants

- Relocation places and results are logical ELF32 guest values, never host pointers.
- Main REL decoding consumes only validated linker metadata and `GuestMemory`.
- Symbol scope remains feature-006 graph-local BFS; object-vector order is not interposition order.
- All required validation/resolution completes before the first relocation write.
- Failure is not allowed to leave relocation-owned partial writes without an explicit rollback-failure result.
- The layer never broadens page permissions to make a relocation pass.
- PLT/JMPREL, versioning, TLS, IFUNC, RELRO, and Android namespace/global-group policy remain separate contracts.

## Security / Performance Constraints

- Relocation count is explicitly bounded.
- All guest-address calculations are checked before access.
- All guest reads/writes use 4-byte bounded operations for this feature.
- No malformed relocation may trigger unbounded symbol/hash/name traversal beyond existing caller-selected feature-006 limits.
- Planning is O(relocation count plus bounded symbol lookup work).
- Mutation stores only the precomputed plan and original words required for rollback.

## Evidence / Reference Basis

Normative/reference inputs:

- Arm, *ELF for the Arm Architecture* (AAELF32), release 2025Q4 / 23 January 2026: ELF32 ARM relocation encodings and operations, including `R_ARM_ABS32`, `R_ARM_GLOB_DAT`, `R_ARM_JUMP_SLOT`, and `R_ARM_RELATIVE`.
- Android bionic `linker/linker_relocate.cpp`, snapshot `25a5023ac2049f1e25a3a0f7ee75da5899ba5049`: current generic ARM mapping and explicit note that bionic does not add the AAELF32 addend for ARM `R_ARM_GLOB_DAT`.
- Project CI #208 artifact `arm32-loader-fixture-ecdae1cea991ecd079487e1cda0bc2fe3c7fff99`, artifact ID `10745105004`, digest `sha256:425dff403ae29d3704a62daa254dcdc147d0a068276cf52afe394d66b2db277f`: direct `readelf -rW` observation of two `R_ARM_GLOB_DAT` entries and zero in-place target words.

The project intentionally chooses Android bionic behavior over the generic AAELF32 addend formula for `R_ARM_GLOB_DAT` because Android compatibility is the target environment.

## Open Questions

No unresolved question blocks T001 read-only REL decoding/planning.

Later tasks must validate whether the first application slice should include any additional ordinary ARM relocation type before feature convergence. New types are FOLLOW_UP unless required by the accepted feature criteria above.
