# Requirements — ELF32 GNU RELRO protection

Status: ACTIVE — T001/T002 verified through CI #232; T003 real post-relocation sealing implementation prepared, exact-head validation NOT RUN

## Goal

Add bounded GNU RELRO support above the completed ELF32 loader and eager relocation layers.

The loader must validate and publish `PT_GNU_RELRO` ranges without changing permissions. A later explicit operation must seal those page-rounded ranges read-only only after the caller has completed relocations.

## Scope

- Recognize ELF32 `PT_GNU_RELRO` program headers.
- Validate their raw guest ranges and page-rounded mapped coverage in the shared immutable load plan.
- Publish load-biased logical guest RELRO metadata in `Elf32LoadResult`.
- Keep loader mapping permissions unchanged until an explicit post-relocation sealing API is invoked.
- Bound sealing work by a caller-selected page ceiling.
- Preflight all RELRO pages before the first permission mutation.
- Seal eligible pages to read-only without mapping new pages or broadening permissions.
- Preserve and restore prior page permissions on a later protection failure when possible.
- Reuse the existing pinned NDK ARMv7 fixture as the real GNU RELRO oracle.

## Non-goals

- Lazy PLT binding, resolver trampolines, or `DT_PLTGOT`.
- Android RELRO serialization/sharing APIs such as `ANDROID_DLEXT_WRITE_RELRO` / `ANDROID_DLEXT_USE_RELRO`.
- A process-wide linker orchestration API or one atomic transaction spanning relocations for every object plus RELRO.
- Symbol-version matching, requester-specific protected/`DT_SYMBOLIC` semantics, Android namespace/global-group policy.
- TLS, IFUNC execution, constructors/destructors, `dlopen`, `dlsym`, unload, or guest execution.
- RELA, RELR, Android packed relocations, COPY/REL32/instruction relocations, or text-relocation permission broadening.
- Treating guest addresses as host pointers.

## Requirements

### R1 — Program-header recognition

The shared ELF32 load plan recognizes `PT_GNU_RELRO == 0x6474e552`.

Zero RELRO program headers are valid. Multiple RELRO program headers are accepted and preserved in program-header order.

### R2 — Raw and page-rounded validation

For each GNU RELRO program header:

- `p_memsz` must be non-zero;
- `p_vaddr + p_memsz` must fit the 32-bit guest address space;
- protection start is `page_start(p_vaddr)`;
- protection end is `page_end(p_vaddr + p_memsz)`;
- every page in that rounded range must already belong to a non-empty readable `PT_LOAD` mapping.

The loader must not invent mappings to make RELRO valid.

### R3 — Loader result metadata

`Elf32LoadResult` exposes each validated RELRO descriptor after load bias as logical guest values:

- exact biased `guest_address`;
- original `memory_size`;
- page-aligned `mapping_start`;
- page-multiple `mapping_size`.

Bias/range overflow is an explicit load failure.

### R4 — Metadata stage remains non-mutating

Planning is read-only. Ordinary `load_elf32` still finishes with the original `PT_LOAD` permissions.

A writable RELRO page must remain writable immediately after load so existing relocation application can run before sealing.

### R5 — Explicit bounded sealing API

T002 adds a separate GNU RELRO sealing API over an already loaded object result.

If no RELRO metadata is present, sealing succeeds with empty work.

When RELRO is present, the caller must provide a non-zero page ceiling. Declared page occurrences above that ceiling fail before permission mutation.

### R6 — Defensive preflight

Before the first permission change, every declared RELRO descriptor is checked defensively for:

- non-zero, page-aligned mapped range metadata;
- page-multiple range size;
- exact guest range contained inside the declared mapped range;
- mapped pages;
- readable pages;
- no executable pages.

The executable-page restriction keeps this bounded feature from silently changing code permission semantics.

### R7 — Overlap and idempotence

Overlapping/repeated RELRO page declarations are permitted.

The implementation deduplicates pages for mutation while keeping the caller ceiling as a bound on declared page occurrences.

Pages already read-only are valid and make repeated sealing idempotent.

### R8 — Permission result

An eligible RELRO page is either already `R` or currently `RW`.

Successful sealing leaves it exactly `R`.

No new mapping is created, execute permission is never introduced, and a page that was not readable is never made readable.

### R9 — Transactional permission changes

All metadata/mapping/permission checks complete before the first mutation.

If a later page-protection call fails, restore previously changed pages in reverse order to their captured original permissions.

A rollback failure is reported distinctly while preserving the primary protection failure and identifying the affected page when available.

### R10 — Ordering contract

GNU RELRO sealing is a post-relocation operation.

This feature does not automatically invoke main REL or PLT REL application. Callers remain responsible for applying all relocations that target the object before sealing it.

### R11 — Real fixture metadata

The existing pinned NDK r27d/API 26 ARMv7 loader fixture must expose at least one `PT_GNU_RELRO` program header.

T001 validation compares raw fixture program-header values with the loader's biased RELRO metadata and records readelf evidence.

### R12 — Real post-relocation sealing

A fixture-backed test must:

- load the existing real ARMv7 fixture through the dependency-graph path;
- apply its supported main relocations (and the empty PLT path if applicable);
- verify relocation targets before sealing;
- seal the object's GNU RELRO;
- verify relocated target bytes are unchanged;
- verify RELRO pages are read-only and direct writes fail;
- verify non-RELRO mapped-page permissions are unchanged;
- not execute guest code.

### R13 — Compatibility

Existing loader, placement, dependency, symbol, main-REL, and eager JUMP_SLOT behavior remains source-compatible apart from additive RELRO metadata/API.

Loader mapping must not begin enforcing RELRO implicitly.

### R14 — Validation

Completion requires:

- synthetic valid/missing/multiple/empty/overflow/outside-load/non-readable RELRO metadata cases;
- load-bias/page-rounding coverage;
- sealing success, page-bound, malformed metadata, unmapped/unreadable/executable preflight, overlap, and idempotence coverage;
- real fixture GNU RELRO metadata evidence;
- real post-relocation sealing evidence;
- neighboring loader/dependency/symbol/relocation suites green;
- final exact-head Linux CTest, Android x86_64 probe, and Android arm64-v8a cross-build PASS.

## Acceptance Criteria

- AC1: Missing `PT_GNU_RELRO` is valid and publishes no RELRO metadata.
- AC2: Valid RELRO headers publish exact raw/page-rounded plan metadata without mutation.
- AC3: Empty, overflowing, outside-mapped-load, or non-readable rounded RELRO ranges fail before mapping.
- AC4: Loaded RELRO metadata is correctly load-biased and ordinary load keeps original writable permissions.
- AC5: Sealing performs no mutation until every declared page passes bounds/mapping/permission checks.
- AC6: Successful sealing changes eligible RW pages to R and is idempotent for already-R pages.
- AC7: Sealing never broadens permissions and rejects executable/unreadable/unmapped pages explicitly.
- AC8: A late protection failure rolls back earlier changed pages or reports rollback failure distinctly.
- AC9: Existing relocation behavior remains unchanged before sealing.
- AC10: The pinned real fixture exposes observed GNU RELRO metadata.
- AC11: Real relocations survive sealing byte-for-byte while RELRO writes become impossible and unrelated page permissions remain unchanged.
- AC12: Final exact-head Linux and both Android CI jobs PASS.

## Invariants

- Guest addresses remain logical 32-bit values.
- Loader metadata discovery and post-relocation permission hardening remain separate contracts.
- No RELRO permission change happens during load.
- RELRO never causes a missing page to be mapped or an unreadable page to become readable.
- Relocations must complete before sealing.
- Future lazy binding must define its own interaction with RELRO rather than weakening this eager contract implicitly.

## Evidence basis

- Android bionic `linker_phdr.cpp` applies GNU RELRO after relocations, rounds `p_vaddr` down and `p_vaddr + p_memsz` up to page boundaries, and calls `mprotect(..., PROT_READ)`; snapshot `b1a578a43032061379080689bae8b970ed3f142e`.
- The pinned project ARMv7 fixture already records a GNU RELRO program header in loader evidence.
- `MappedGuestMemory` already provides page-aligned `protect`, mapping-state, page-size, and permission queries without exposing host pointers.

## Open Questions

No unresolved question blocks T001 or T002. Lazy-binding/partial-RELRO policy and Android RELRO sharing remain explicit future contracts.
