# Requirements — ELF32 ARM eager JUMP_SLOT relocations

Status: READY — readiness checked at base `bbad7db39e430ba87a373841e3ede954696c85c5`

## Goal

Add bounded eager application of ARM ELF32 PLT `R_ARM_JUMP_SLOT` relocations above the completed feature-008 PLT REL metadata boundary.

The feature consumes only the validated `plt_rel_table` descriptor, decodes `Elf32_Rel` entries byte-explicitly, resolves each symbol through the existing graph-local symbol layer, and writes resolved logical guest values transactionally through `GuestMemory`.

## Scope

- Add `R_ARM_JUMP_SLOT` (22) as a named ARM relocation constant.
- Add separate PLT planning/resolution/application APIs; do not silently fold PLT entries into the existing main-`DT_REL` APIs.
- Consume only `Elf32LinkerMetadata::plt_rel_table`.
- Decode 8-byte REL entries as little-endian `r_offset` + `r_info`.
- Require every PLT entry in this feature to be `R_ARM_JUMP_SLOT`.
- Compute `P = load_bias + r_offset` with checked 32-bit guest arithmetic.
- Require word alignment/readability, snapshot the original word for rollback, and reject duplicate PLT write targets.
- Bound each call with the existing caller-selected `max_relocations`.
- Resolve JUMP_SLOT symbols using the same bounded feature-006 index/name/graph-local BFS contracts used by feature 007.
- Apply JUMP_SLOT eagerly as `S`, ignoring the in-place word as an addend.
- Preserve unresolved weak behavior as `S = 0`; unresolved strong references fail before mutation.
- Reuse the feature-007 transaction rule: all semantic checks/final values before the first write; reverse rollback on a later failure.
- Add a reproducible ARMv7 consumer/provider fixture pair proving a real `DT_NEEDED` + `R_ARM_JUMP_SLOT` path through dependency loading and graph-local resolution.

## Non-goals

- Lazy binding or deferred first-call resolution.
- `DT_PLTGOT` resolver state/trampolines.
- Applying main REL and PLT REL in one combined transaction.
- Accepting main-REL relocation types from the PLT table.
- RELA, RELR, Android packed relocations, APS2, COPY, REL32, instruction relocations, or IRELATIVE.
- Symbol-version matching, requester-specific protected/`DT_SYMBOLIC` semantics, Android namespace/global-group/process-wide interposition policy.
- TLS, IFUNC execution, RELRO, constructors/destructors, `dlopen`, `dlsym`, unload, or guest execution.
- Temporarily broadening guest page permissions.
- Treating guest addresses as host pointers.

## Requirements

### R1 — Separate PLT API boundary

Expose separate functions for PLT REL planning, reference resolution, and eager application.

Existing `build_elf32_rel_relocation_plan`, `resolve_elf32_rel_relocation_references`, and `apply_elf32_rel_relocations` retain main-`DT_REL` semantics.

Internal helpers may be shared, but public behavior must not merge the two table contracts.

### R2 — PLT descriptor source

The PLT APIs consume only `object.linker_metadata.plt_rel_table`, already validated by feature 008.

An object with no PLT descriptor succeeds with empty work.

A present descriptor must defensively retain `entry_size == 8` and `size % 8 == 0` checks because public metadata types can be constructed directly.

### R3 — Count bound

When a PLT descriptor is present, `max_relocations` must be non-zero.

Reject a PLT entry count above the caller ceiling before reading entries or targets.

### R4 — Entry decoding and type policy

Decode each 8-byte entry explicitly:

- `r_offset: uint32 little-endian`;
- `r_info: uint32 little-endian`;
- symbol index = `r_info >> 8`;
- type = `r_info & 0xff`.

Only `R_ARM_JUMP_SLOT` (22) is accepted in the PLT table for feature 009. Every other type, including NONE, GLOB_DAT, ABS32, RELATIVE, TLS, IFUNC, COPY, REL32, and instruction relocations, fails before mutation.

### R5 — Target validation and rollback snapshot

For every JUMP_SLOT entry:

- compute `P = load_bias + r_offset` with checked 32-bit arithmetic;
- require `P % 4 == 0`;
- read the original 32-bit little-endian word through `GuestMemory`;
- retain it only as rollback state, not as a semantic addend;
- reject duplicate PLT target words within one table.

No host pointer is derived from `P`.

### R6 — Reference-symbol policy

JUMP_SLOT requires a non-zero dynamic-symbol index in the relocating object.

Reuse feature-007 reference validation:

- bounded symbol index/name reads;
- GLOBAL or WEAK binding;
- DEFAULT visibility;
- NOTYPE/OBJECT/FUNC type;
- no protected/hidden/internal/local/TLS/IFUNC/common/XINDEX/versioned forms;
- graph-local BFS lookup beginning at the relocating object.

This feature does not create a new interposition policy.

### R7 — Weak/strong lookup behavior

A resolved definition yields logical guest value `S`.

An unresolved WEAK JUMP_SLOT reference uses `S = 0`.

An unresolved non-weak reference fails before mutation.

### R8 — Android/AAELF32 JUMP_SLOT value

Eager JUMP_SLOT stores `S`.

AAELF32 specifies that the REL-form JUMP_SLOT addend is always zero and describes the relocation as resolving to the symbol address.

Current Android bionic uses the non-REL-addend path for generic JUMP_SLOT and writes the resolved symbol address.

The implementation must therefore ignore the in-place target word as an addend. Synthetic coverage must use a non-zero original word so this rule cannot drift.

### R9 — Transactional apply

Before the first PLT write:

- planning must succeed for every entry;
- every reference must resolve or satisfy weak-zero policy;
- every final JUMP_SLOT word must be computed;
- every original target word must be available for rollback.

Writes occur in PLT table order.

On a later failure, restore every prior successful PLT write in reverse order and verify restoration. If rollback itself fails, report it distinctly while preserving the primary write failure.

A failed call publishes no partial successful application.

### R10 — Memory permissions

The feature never changes guest mappings or permissions.

If a target is not writable through the existing `GuestMemory` contract, application fails and follows rollback semantics.

### R11 — Real fixture pair

Add reproducible pinned-NDK ARMv7 DSOs:

- provider DSO exports one default-visible function, `fixture_import`;
- consumer DSO calls `fixture_import` and links against the provider, producing a `DT_NEEDED` edge and a PLT REL `R_ARM_JUMP_SLOT` for that symbol.

The build must remain freestanding/`-nostdlib`, use API 26, no build ID, and 16 KiB maximum page size.

CI must build the pair twice and byte-compare each output.

### R12 — Real graph-backed application

A fixture-backed test must:

- load the consumer root through `load_elf32_dependency_graph`;
- have the provider callback return the provider fixture for the exact requested `DT_NEEDED` name;
- require a two-object graph with one consumer dependency edge;
- require a validated consumer PLT REL descriptor;
- plan/resolve/apply the real JUMP_SLOT through the feature-009 APIs;
- independently resolve `fixture_import` through feature 006 and require the relocated slot equals that guest value;
- preserve segment permissions and bytes outside the PLT target word;
- not execute guest code.

### R13 — Compatibility

Existing main REL APIs/results and feature-007 formulas remain unchanged.

Existing dependency, symbol, loader, and metadata contracts remain source-compatible apart from additive relocation APIs/constants and new fixture/test plumbing.

### R14 — Validation

Completion requires:

- synthetic empty/present/limit/read/place/alignment/type/duplicate cases;
- JUMP_SLOT reference bounds/form failures;
- strong-not-found and weak-zero behavior;
- non-zero original word proving eager `S` semantics;
- late write rollback and rollback-failure coverage for PLT application;
- reproducible real consumer/provider artifact inspection showing the expected `DT_NEEDED` and `R_ARM_JUMP_SLOT`;
- graph-backed real JUMP_SLOT application;
- neighboring main relocation/dependency/symbol tests green;
- final exact-head Linux CTest, Android x86_64 probe, and Android arm64-v8a cross-build PASS.

## Acceptance Criteria

- AC1: Missing PLT metadata succeeds with empty plan/resolution/application.
- AC2: Valid bounded PLT REL entries decode exact offset/info/symbol/type/place/original word without mutation.
- AC3: Only JUMP_SLOT is accepted in the PLT table; malformed type/count/read/place/alignment/duplicate inputs fail before mutation.
- AC4: JUMP_SLOT references use the existing bounded graph-local symbol contract.
- AC5: Strong misses fail pre-write; weak misses write zero.
- AC6: JUMP_SLOT writes `S` and ignores a non-zero original target word.
- AC7: Late write failure restores earlier PLT writes or reports rollback failure explicitly.
- AC8: Main-`DT_REL` behavior remains unchanged.
- AC9: The reproducible consumer/provider pair contains the expected dependency + JUMP_SLOT shape.
- AC10: The real consumer dependency graph resolves and applies the JUMP_SLOT to the provider symbol guest value without guest execution.
- AC11: Mappings/permissions and non-target bytes remain unchanged by real application.
- AC12: Final exact-head Linux and both Android CI jobs PASS.

## Invariants

- All addresses/results are logical 32-bit guest values.
- PLT metadata, main REL metadata, and relocation application remain separate contracts.
- PLT planning/resolution is read-only.
- No relocation write occurs until the complete PLT table is semantically valid.
- Eager JUMP_SLOT does not introduce lazy resolver state.
- No compatibility policy is broadened merely to satisfy the real fixture.

## Evidence basis

- Arm AAELF32: `R_ARM_JUMP_SLOT` code 22 is a dynamic data relocation that resolves to the symbol address; for REL form its addend is always zero.
- Android bionic `linker_relocate.cpp` snapshot `25a5023ac2049f1e25a3a0f7ee75da5899ba5049`: generic JUMP_SLOT computes the resolved symbol address with the non-REL-addend path and writes it directly.
- Feature 008 provides the validated PLT REL descriptor and intentionally stops before entry decoding/application.

## Open Questions

No unresolved question blocks T001. Lazy binding and combined main+PLT transaction ordering remain separate future contracts.
