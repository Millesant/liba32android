# Design — ELF32 PLT REL Metadata

Status: DONE — T002 final exact-head CI #223 / run `35934340806` PASSed at `79e9d8c90824baf76d7ff382661422af17e3cb6e`

## Context

`elf32_linker_metadata` already validates main `DT_REL`, string/symbol tables, hash headers, SONAME/NEEDED offsets, and version-presence markers. Feature 007 consumes only the validated main REL descriptor and deliberately rejected PLT/JMPREL.

The next smallest dependency is therefore PLT metadata, not relocation application.

## Chosen Approach

Extend the existing linker-metadata types and parser.

Collected metadata gains an optional `plt_rel_table`. Validated metadata gains an optional guest-only `plt_rel_table` using the existing REL descriptor shape.

## Tag model

Recognize:

- `DT_PLTRELSZ = 2`;
- `DT_PLTREL = 20`;
- `DT_JMPREL = 23`.

`DT_JMPREL` is pointer-like and receives one checked load-bias addition. `DT_PLTRELSZ` is a scalar byte size. `DT_PLTREL` is a scalar selector and must equal `DT_REL = 17`. All three are singleton tags.

## Collection

Collect `jmprel`, `pltrelsz`, and `pltrel`. After the first `DT_NULL`:

1. require either zero or all three fields;
2. reject non-`DT_REL` `DT_PLTREL`;
3. publish a collected PLT REL descriptor with raw address, byte size, and fixed 8-byte entry size.

The collection pass remains side-effect free.

## Validation

Reuse the main-REL range-validation mechanics:

1. require `size % 8 == 0`;
2. rebase raw `DT_JMPREL` with checked 32-bit arithmetic;
3. require the range to fit the guest address space;
4. read-validate the non-empty range in bounded chunks;
5. publish a logical guest-only descriptor.

A zero-byte table requires no guest read.

## Error surface

Add `IncompletePltRelTable`, `InvalidPltRelType`, and `InvalidPltRelSize`. Existing duplicate, address/range overflow, and read errors remain unchanged.

## API / Documentation

The header and `docs/architecture/elf32-linker-metadata.md` must state that `plt_rel_table` is metadata only, its entry size is ELF32 REL size 8, and it implies no JUMP_SLOT/lazy-binding support.

## Test Strategy

Extend `tests/elf32_linker_metadata.cpp` with valid PLT collection/validation, main+PLT coexistence, duplicate/partial/wrong-selector/bad-size/overflow/unreadable/zero-size cases, and no-mutation checks. Existing real-fixture integration remains the absent-group regression oracle.

## Alternatives

- Parse PLT tags in `elf32_relocation`: rejected because tag grouping/rebasing/range validation belong to linker metadata.
- Implement JUMP_SLOT now: rejected as a separate behavioral and mutation contract.
- Infer REL when `DT_PLTREL` is missing: rejected; malformed metadata must remain visible.

## Readiness

Acceptance has executable validation; invariants are preserved; docs destinations are known; T001 is bounded to metadata + tests; T002 is convergence/final CI.
