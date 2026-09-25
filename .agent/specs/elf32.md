# ELF32 and dynamic-linking contract

Status: Accepted current project contract
Last reconciled: 2026-09-25

## L32-E001 — ELF32 mapping

The loader accepts validated little-endian ARM ELF32 `ET_EXEC` and explicit-base `ET_DYN`, maps validated `PT_LOAD` segments through `MappedGuestMemory`, copies file bytes, zero-fills BSS, applies final segment permissions, detects conflicts, and rolls back loader-owned mappings on failure.

## L32-E002 — Shared load planning and placement

Pre-mutation ELF validation/layout is shared through `elf32_load_plan`. Automatic `ET_DYN` placement is deterministic, caller-bounded, non-mutating, preserves host-page and `p_align` constraints, and returns a loader-ready guest base.

## L32-E003 — Structural dynamic metadata

`PT_DYNAMIC` discovery and `Elf32_Dyn` parsing remain structural. Dynamic entries preserve raw signed tags/raw values, require bounded valid guest ranges and `DT_NULL` termination, and do not themselves perform dynamic linking.

## L32-E004 — Linker metadata and strings

Validated linker metadata covers STRTAB/STRSZ, SYMTAB/SYMENT, main REL/RELSZ/RELENT, separate AArch32 PLT REL metadata, SONAME, and ordered `DT_NEEDED` offsets. String materialization is explicitly bounded and preserves ordered/repeated dependency names.

## L32-E005 — Dependency acquisition and graph loading

Dependency acquisition is provider-backed and bounded; filesystem/search-path/namespace policy stays outside the generic core. Recursive graph loading is transactional, uses provider identity as the per-call object key, preserves ordered/repeated edges, reuses cycles/aliases, automatically places dependency `ET_DYN` images, and rolls back graph-owned mappings on aggregate failure.

## L32-E006 — Symbol resolution

Dynamic-symbol indexing supports bounded SysV/GNU hash processing and exact byte-name lookup. Graph-local resolution uses deterministic breadth-first dependency scope. Version-aware/process-wide interposition, TLS, IFUNC, and unsupported special-section semantics fail explicitly rather than being approximated.

## L32-E007 — Main REL relocations

Main `DT_REL` supports bounded planning/resolution/application for `R_ARM_NONE`, `R_ARM_RELATIVE`, `R_ARM_GLOB_DAT`, `R_ARM_ABS32`, and `R_ARM_REL32`. All semantic checks complete before writes; later write failures trigger reverse rollback. `GLOB_DAT` writes `S` and does not use the in-place word as an addend. `REL32` applies `((S + A) | T) - P` modulo 2^32, deriving `T` from the defining Thumb `STT_FUNC` rather than the requester.

## L32-E008 — Eager PLT relocation

The separate PLT REL path accepts eager `R_ARM_JUMP_SLOT`, resolves through the same bounded graph-local symbol policy, writes `S` directly, and uses the original slot word only for rollback. Lazy binding and `DT_PLTGOT` resolver state remain outside the accepted contract.

## L32-E009 — GNU RELRO

Validated `PT_GNU_RELRO` ranges are exposed as guest-only loader metadata without early sealing. The explicit post-relocation sealing API is caller-bounded, preflights the full page set, deduplicates overlaps, accepts already-read-only pages, changes only RW pages to R, never broadens permissions, and rolls back earlier changes on a later protection failure when possible.

Real post-relocation fixture integration is verified by CI #234 / run `35946857448` at `ff1792457f05bd9dd58740e1b768576b9ad4f1c3`: the real GLOB_DAT targets survive sealing, RELRO becomes read-only, direct writes fail, and non-RELRO permissions remain unchanged.

## L32-E010 — Combined main + eager PLT relocation transaction

The additive combined relocation API fully prepares the supported main `DT_REL` and eager PLT REL tables before mutation, rejects cross-table duplicate write targets, applies main writes before PLT writes, and rolls back failures in reverse across the combined sequence. Direct and rollback failures identify the source table as well as relocation index. The existing main-only and PLT-only APIs retain their independent transaction semantics.

## L32-E011 — Real ARM32 fixture execution

The pinned freestanding NDK ARM32 fixture is loadable through the dependency graph, resolvable through GNU-hash graph lookup, relocatable through the combined main+PLT transaction, sealable through GNU RELRO, and executable through the generic A32 CPU adapter on the Linux validation host. The bounded call harness supplies AAPCS r0/r1 arguments, an 8-byte-aligned mapped guest stack, ARM/Thumb state derived from the defining function symbol, and a same-state return sentinel. This proves integrated host execution only and is not Android-device execution evidence.

## L32-E012 — Deferred linker scope

Still outside the accepted implementation: Android namespace/search-path/link-map lifetime policy across independent graph loads; version-aware/process-wide/global-group interposition; lazy binding; broader ARM relocation families; RELA/RELR/Android packed relocations; TLS/IFUNC; constructors/destructors; `dlopen`/`dlsym`/unload; and guest execution of the real ARM32 fixture.
