# ELF32 dynamic-array parsing

Status: M3 structural metadata layer

## Boundary

`elf32_dynamic` sits above validated ELF mapping and below any future dynamic linker. It consumes only:

- `memory::GuestMemory` reads;
- a loader-produced `Elf32DynamicSegment` containing biased guest VA, `p_filesz`, and `p_memsz`.

It returns raw ELF32 `d_tag` / `d_val` pairs. It does not depend on Dynarmic, host pointers, Android compatibility shims, JNI, dependency loading, symbol resolution, relocation application, or application profiles.

```text
ELF32 image
   |
   v
elf32_loader ----> validated PT_DYNAMIC guest range
   |                         |
   v                         v
GuestMemory <--------- elf32_dynamic structural parser
                             |
                             v
                    raw d_tag / d_val entries
                             |
                             v
                    future linker metadata layer
```

The parser therefore preserves the existing address-space rule: guest addresses remain logical 32-bit guest VAs and no host pointer crosses the ELF API boundary.

## Structural policy

An `Elf32_Dyn` entry is exactly 8 bytes in ELF32 little-endian form: signed 32-bit `d_tag` followed by a raw 32-bit value/pointer field.

The current parser deliberately performs only structural validation:

- `p_filesz <= p_memsz` and the supplied guest memory range must fit the 32-bit guest address space;
- the file-backed PT_DYNAMIC byte count must be a multiple of 8;
- each entry is read through `GuestMemory` from the biased guest address;
- raw signed `d_tag` values and raw 32-bit values are preserved without semantic rewriting;
- the first `DT_NULL` terminates the logical array and is retained as the final returned entry;
- bytes after that first `DT_NULL` are ignored as padding;
- reaching the end of `p_filesz` without `DT_NULL` is rejected;
- an unreadable guest range is reported explicitly.

Unknown tags are not rejected or silently reclassified. Preserving them as raw metadata lets a later linker layer decide which semantics it supports.

## Explicit non-goals

This M3 slice does **not**:

- add load bias to pointer-like dynamic values;
- dereference `DT_STRTAB`, `DT_SYMTAB`, `DT_REL`, GNU hash, or any other pointer tag;
- parse strings or symbols;
- load `DT_NEEDED` dependencies;
- apply ARM relocations;
- enforce RELRO;
- process TLS;
- implement symbol lookup or interposition.

Those operations belong to later validated metadata/linker layers rather than the structural parser.

## Validation

Synthetic coverage exercises:

- a valid array ending in `DT_NULL`;
- preservation of ordinary and signed unknown tag values;
- ignoring padding entries after the first `DT_NULL`;
- rejection of a non-8-byte-aligned file-backed range;
- rejection of an unterminated dynamic array;
- defensive invalid-range rejection;
- guest-memory read failure reporting.

The reproducible Android NDK ARMv7 fixture is the integration case. It is first loaded through `elf32_loader`, then parsed from the loader-reported PT_DYNAMIC guest range. The structural test requires the observed SONAME, REL, SYMTAB, STRTAB and GNU-hash-related tags while confirming the freestanding fixture still has no `DT_NEEDED` entry.
