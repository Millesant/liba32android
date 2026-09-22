# ELF32 linker metadata

Status: M4 validated metadata layer implemented through T003

## Boundary

`elf32_linker_metadata` sits above structural `Elf32_Dyn` parsing and below string, symbol, dependency, and relocation semantics. Feature 006 extends the descriptor set with the fixed metadata needed by bounded symbol indexing without moving variable hash parsing into this layer.

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
- `DT_SONAME`;
- repeated `DT_NEEDED` offsets.

Recognized singleton tags are unique. A duplicate is rejected even if the value matches. `DT_NEEDED` is intentionally repeatable and preserves dynamic-array order.

Unknown and deferred tags remain tolerated. Their presence does not imply semantic support.

## Validation policy

Pointer-like values for STRTAB, SYMTAB, SysV/GNU hash tables, and REL receive exactly one checked addition of the loader-provided load bias.

The layer rejects:

- load-bias addition that exceeds the 32-bit guest address space;
- declared range ends that exceed the guest address space;
- unreadable referenced STRTAB/SYMTAB/REL guest ranges or fixed SysV/GNU hash headers;
- incomplete STRTAB/STRSZ, SYMTAB/SYMENT, or REL/RELSZ/RELENT groups;
- `DT_SYMENT != 16`;
- `DT_RELENT != 8`;
- `DT_RELSZ` not divisible by `DT_RELENT`;
- SONAME or NEEDED offsets outside `DT_STRSZ`.

Potentially large ranges are checked through bounded `GuestMemory::read` operations rather than allocating a host buffer equal to the guest-declared range.

For `DT_HASH` and `DT_GNU_HASH`, this layer deliberately validates only pointer rebasing plus fixed-header readability (8 and 16 bytes respectively). It does not trust bucket/chain counts here. Variable arrays and the complete inferred symbol-table extent are interpreted by `elf32_symbol_lookup` under explicit caller-selected ceilings.

The validator is read-only. Failure paths do not change guest bytes, mappings, or permissions.

## Deliberate limits

This layer does not yet:

- load `DT_NEEDED` dependencies or apply search-path/namespace policy;
- interpret variable SysV/GNU hash arrays or consume symbol entries;
- decode or apply ARM relocations;
- handle PLT/JMPREL;
- implement symbol lookup/interposition/versioning;
- process RELRO, TLS, constructors/destructors, or Android packed relocations.

Bounded SONAME/NEEDED string consumption lives in `elf32_linker_strings`; bounded hash/dynsym indexing begins in `elf32_symbol_lookup`; exact lookup, graph scope, and relocation/runtime behavior remain separate slices.

## Validation evidence

Synthetic coverage exercises semantic collection, duplicate and incomplete-group failures, zero/non-zero load bias, address/range overflow, unreadable ranges, ELF32 entry-size rules, REL-size divisibility, string-offset bounds, unknown-tag tolerance, hash descriptor rebasing/header readability, and no-mutation failure behavior.

The reproducible NDK-generated ARM32 `ET_DYN` fixture is also loaded through `elf32_loader`, parsed through `elf32_dynamic`, then validated through `elf32_linker_metadata`. The integration test checks that STRTAB/SYMTAB/REL guest addresses equal the raw dynamic pointer values plus the loader's actual load bias, preserves SONAME, and confirms the freestanding fixture has no `DT_NEEDED`.

GitHub Actions run `35332054239` (#87) passed the T003 implementation head on both the Linux test job and Android `arm64-v8a` cross-build. A later exact-head CI run is still required after documentation/state convergence before the feature is considered complete.
