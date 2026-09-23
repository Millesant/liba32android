# ELF32 linker metadata

Status: complete through feature 008; final exact-head gate PASSed

## Boundary

`elf32_linker_metadata` sits above structural `Elf32_Dyn` parsing and below string, symbol, dependency, and relocation semantics. Feature 006 extends the descriptor set with the fixed metadata needed by bounded symbol indexing without moving variable hash parsing into this layer. Feature 008 adds a separate validated AArch32 PLT `Elf32_Rel` descriptor without decoding or applying PLT relocations.

It consumes:

- ordered raw `Elf32DynamicEntry` values from `elf32_dynamic`;
- the loader-provided ELF load bias;
- `memory::GuestMemory` for read-only guest-range validation.

It returns validated guest-VA descriptors and string-table offsets only. It does not load libraries, materialize strings, resolve symbols, decode/apply relocations, modify mappings, or execute guest code.

```text
ELF32 image
   |
   v
elf32_loader
   |  load_bias + validated PT_DYNAMIC guest range
   v
elf32_dynamic
   |  ordered raw d_tag / d_val entries
   v
elf32_linker_metadata
   |  validated guest-VA descriptors + string offsets
   +------------------------+
   |                        |
   v                        v
elf32_linker_strings   elf32_symbol_lookup
   |                        | bounded SysV/GNU hash + dynsym index
   |                        v
   |                   symbol semantics
   v
dependency loading
   |
   v
relocation/runtime layers
```

Guest addresses remain logical 32-bit values. No host pointer crosses this layer.

## Supported metadata

The current semantic set recognizes:

- `DT_STRTAB` + `DT_STRSZ`;
- `DT_SYMTAB` + `DT_SYMENT`;
- `DT_HASH`;
- `DT_GNU_HASH`;
- `DT_REL` + `DT_RELSZ` + `DT_RELENT`;
- `DT_JMPREL` + `DT_PLTRELSZ` + `DT_PLTREL`, with `DT_PLTREL == DT_REL` required for the AArch32 PLT table;
- `DT_SONAME`;
- repeated `DT_NEEDED` offsets;
- a presence marker for GNU/SysV symbol-version metadata (`DT_VERSYM` / `DT_VERDEF*` / `DT_VERNEED*`) so name-only lookup can reject unsupported versioning explicitly.

Recognized singleton tags are unique. A duplicate is rejected even if the value matches. `DT_NEEDED` is intentionally repeatable and preserves dynamic-array order.

Unknown and deferred tags remain tolerated. Their presence does not imply semantic support. Version-table contents remain deferred; only their declaration is recorded for the symbol layer's safety boundary.

## Validation policy

Pointer-like values for STRTAB, SYMTAB, SysV/GNU hash tables, main REL, and `DT_JMPREL` receive exactly one checked addition of the loader-provided load bias.

The layer rejects:

- load-bias addition that exceeds the 32-bit guest address space;
- declared range ends that exceed the guest address space;
- unreadable referenced STRTAB/SYMTAB/main-REL/PLT-REL guest ranges or fixed SysV/GNU hash headers;
- incomplete STRTAB/STRSZ, SYMTAB/SYMENT, REL/RELSZ/RELENT, or JMPREL/PLTRELSZ/PLTREL groups;
- `DT_SYMENT != 16`;
- `DT_RELENT != 8`;
- `DT_RELSZ` not divisible by `DT_RELENT`;
- `DT_PLTREL != DT_REL`;
- `DT_PLTRELSZ` not divisible by the 8-byte ELF32 REL entry size;
- SONAME or NEEDED offsets outside `DT_STRSZ`.

Potentially large ranges are checked through bounded `GuestMemory::read` operations rather than allocating a host buffer equal to the guest-declared range.

For `DT_HASH` and `DT_GNU_HASH`, this layer deliberately validates only pointer rebasing plus fixed-header readability (8 and 16 bytes respectively). It does not trust bucket/chain counts here. Variable arrays and the complete inferred symbol-table extent are interpreted by `elf32_symbol_lookup` under explicit caller-selected ceilings.

The validator is read-only. Failure paths do not change guest bytes, mappings, or permissions.

## Deliberate limits

This layer does not yet:

- load `DT_NEEDED` dependencies or apply search-path/namespace policy;
- interpret variable SysV/GNU hash arrays or consume symbol entries;
- decode or apply ARM relocations;
- decode or apply PLT/JMPREL entries, implement `R_ARM_JUMP_SLOT`, or perform lazy binding;
- perform symbol lookup itself or implement version-aware/process-wide interposition policy;
- process RELRO, TLS, constructors/destructors, or Android packed relocations.

Bounded SONAME/NEEDED string consumption lives in `elf32_linker_strings`; bounded hash/dynsym indexing, exact per-object lookup, and graph-local BFS scope live in `elf32_symbol_lookup`; bounded main-`DT_REL` ARM relocation planning/application lives downstream in `elf32_relocation`. PLT REL metadata is now validated here, while PLT entry decoding/application, `R_ARM_JUMP_SLOT`, lazy binding, version-aware/global-group policy, RELRO, TLS, and broader runtime behavior remain separate downstream contracts.

## Validation evidence

Synthetic coverage exercises semantic collection, duplicate and incomplete-group failures, zero/non-zero load bias, address/range overflow, unreadable ranges, ELF32 entry-size rules, main/PLT REL-size divisibility, non-REL `DT_PLTREL` rejection, zero-length PLT tables, string-offset bounds, unknown-tag tolerance, hash descriptor rebasing/header readability, and no-mutation failure behavior.

The reproducible NDK-generated ARM32 `ET_DYN` fixture is also loaded through `elf32_loader`, parsed through `elf32_dynamic`, then validated through `elf32_linker_metadata`. The integration test checks that STRTAB/SYMTAB/REL guest addresses equal the raw dynamic pointer values plus the loader's actual load bias, preserves SONAME, confirms the freestanding fixture has no `DT_NEEDED`, and now explicitly requires no `DT_JMPREL` / `DT_PLTRELSZ` / `DT_PLTREL` tags and no published PLT REL descriptor.

The original linker-metadata T003 integration passed GitHub Actions run `35332054239` (#87). Feature 006 then extended this layer with fixed `DT_HASH` / `DT_GNU_HASH` descriptors and version-presence marking; exact-head CI #203 / run `35758444356` PASSed that extension at `45cd3e5a322263f53dc02277a9d0e801849515db`, and T004 real-fixture CI #206 / run `35837480789` remained green with 41/41 Linux CTest plus both Android jobs.

Feature 008 T001 required-job validation passed on GitHub Actions CI #222 / run `35933694619` at `9a81ed71a027beb166970bcf137bac9a71112f98`: Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all completed successfully. This head includes the explicit pinned-fixture no-PLT oracle required by AC8.

Feature 008 T002 documentation/spec/state convergence and final exact-head gate PASSed on CI #223 / run `35934340806` at `79e9d8c90824baf76d7ff382661422af17e3cb6e`. Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASSed; the bounded PLT REL metadata contract is complete with PLT relocation application/lazy binding still explicitly deferred.
