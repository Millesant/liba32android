# ELF32 GNU RELRO architecture

Status: feature 010 complete; implementation verified by CI #234 and convergence verified by CI #235 / run `35947449163`

## Boundary

GNU RELRO is a post-relocation permission-hardening layer. It is deliberately split between loader metadata discovery and a later explicit sealing operation:

```text
ELF program headers
      |
      v
elf32_load_plan
 validated PT_GNU_RELRO exact + page-rounded ranges
      |
      v
elf32_loader
 load-biased guest-only RELRO metadata
      |
      |  relocations still need writable data
      v
main REL -> eager PLT REL
      |
      v
elf32_relro
 RW -> R sealing
```

The loader never seals RELRO implicitly. `elf32_relro` does not parse ELF bytes, resolve symbols, apply relocations, create mappings, or expose host pointers.

## Metadata contract

For each non-empty `PT_GNU_RELRO`, the shared load plan records:

- exact pre-bias virtual address and memory size;
- host-page-rounded mapping start/end;
- proof that every rounded page is already covered by a non-empty readable `PT_LOAD`.

The loader then publishes load-biased logical guest metadata:

- exact `guest_address`;
- original `memory_size`;
- page-aligned `mapping_start`;
- page-multiple `mapping_size`.

Address/bias overflow and invalid rounded coverage fail before guest mapping mutation. Zero RELRO headers are valid; multiple headers remain ordered metadata.

## Explicit sealing contract

`seal_elf32_gnu_relro` operates on an already loaded object and requires a caller-selected `max_pages` ceiling whenever RELRO metadata is present.

Before the first permission mutation it validates the complete declared set:

- non-zero page-aligned mapped ranges;
- page-multiple sizes;
- exact guest range contained in the declared rounded range;
- declared page-occurrence count within the caller ceiling;
- mapped/readable pages;
- no executable page;
- current permission shape is exactly R or RW.

Overlapping declarations are permitted. Mutation pages are sorted/deduplicated, while the resource ceiling still counts declared page occurrences.

## Permission and rollback semantics

A successful seal leaves every unique RELRO page exactly read-only:

- R pages are already valid and make sealing idempotent;
- RW pages transition to R;
- no page is mapped;
- execute permission is never introduced;
- unreadable memory is never made readable;
- permission broadening is forbidden.

If a later protection operation fails, earlier changed pages are restored in reverse order to their captured permissions. A rollback failure is distinct from the primary protection failure and may leave partial hardening; the result identifies the affected page when available.

## Ordering

The accepted eager-linker sequence is:

1. load/place the object graph;
2. parse validated linker metadata;
3. resolve/apply supported main REL relocations;
4. resolve/apply eager PLT REL relocations;
5. seal GNU RELRO;
6. continue with later runtime work.

A future lazy-binding contract must define GOT/RELRO interaction explicitly. The current implementation does not weaken RELRO in anticipation of an unimplemented resolver.

## Real fixture evidence

The pinned NDK r27d / API 26 ARMv7 loader fixture exposes one observed GNU RELRO range. T001 established its metadata shape; T002 established synthetic bounded sealing/rollback behavior.

T003 is verified by CI #234 / run `35946857448` at `ff1792457f05bd9dd58740e1b768576b9ad4f1c3`:

- Linux A32 smoke passed 49/49 CTest including `elf32_real_relro_seal`;
- Android x86_64 address-space probe passed;
- Android arm64-v8a runtime/diagnostics cross-build passed;
- the real fixture is loaded through the dependency graph;
- supported main relocations are applied before sealing;
- the empty eager-PLT path completes;
- both real GLOB_DAT targets are verified inside GNU RELRO before sealing;
- relocated values remain byte-for-byte intact after sealing;
- writes into RELRO fail after sealing;
- non-RELRO mapped-page permissions remain unchanged;
- no guest ARM code is executed.

## Deliberate limits

This bounded feature does not implement:

- lazy PLT binding or `DT_PLTGOT` resolver state;
- Android RELRO serialization/sharing APIs;
- one atomic transaction spanning graph-wide relocation plus RELRO;
- symbol-version/process-wide/global-group interposition policy;
- TLS or IFUNC;
- broader relocation forms;
- constructors/destructors, `dlopen`, `dlsym`, unload, or guest execution.
