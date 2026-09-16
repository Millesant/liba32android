# ELF32 loader architecture

Status: first M3 PT_LOAD slice implemented and host-tested

## Boundary

The ELF32 loader is a guest-address-space component. It consumes an in-memory ELF32 byte image and maps logical 32-bit guest virtual addresses through `memory::MappedGuestMemory`.

It must remain independent from Dynarmic types, host pointers, dynamic symbol resolution, ARM relocation application, Android compatibility bridges, JNI, graphics/audio and application profiles.

Current dependency direction:

```text
ELF32 byte image
      |
      v
 ELF32 validator / PT_LOAD planner
      |
      v
 MappedGuestMemory
      |
      v
 logical AArch32 guest VAs
```

Dynamic linking will be a later layer above this mapping primitive rather than being folded into the initial loader.

## Supported image policy

The first slice accepts:

- ELF32 class;
- little-endian encoding;
- current ELF identification/header version;
- `EM_ARM`;
- `ET_DYN` and `ET_EXEC`;
- standard ELF32 header/program-header sizes;
- `PT_LOAD` segments whose final permissions are `None`, `R`, `RW`, or `RX`.

`ET_EXEC` loads at its fixed guest virtual addresses with `load_bias == 0`.

`ET_DYN` requires the caller to provide an explicit page-aligned `dynamic_base`. That value identifies where the lowest page-aligned `PT_LOAD` mapping begins. The loader computes `load_bias = dynamic_base - lowest_load_page` and returns guest metadata only; it does not choose addresses through an allocator yet.

The result exposes guest entry/load-bias/segment metadata. Host reservation pointers are not part of the loader API.

## Validation-before-mapping rule

Before mapping any guest page, the loader validates:

- ELF identity/class/data/version;
- supported type and `EM_ARM` machine;
- header and program-header entry sizes;
- program-header table bounds;
- `p_filesz <= p_memsz`;
- file ranges within the input byte image;
- 32-bit guest-address overflow;
- ELF segment alignment constraints;
- supported segment permission shapes;
- ET_DYN base alignment/load-bias range;
- entry-point overflow;
- page-overlapping `PT_LOAD` ranges;
- conflicts with pages already mapped in `MappedGuestMemory`.

The first slice deliberately rejects page-overlapping `PT_LOAD` ranges. Handling shared boundary pages with merged permissions/content is deferred until a real compatibility requirement demonstrates the need; silent permission broadening is not allowed.

## Mapping lifecycle

For each validated non-empty `PT_LOAD` segment:

1. compute the page-aligned guest mapping range;
2. map the range temporarily `RW` for initialization;
3. copy exactly `p_filesz` bytes from the image to the biased guest virtual address;
4. explicitly zero `p_memsz - p_filesz` bytes for BSS;
5. apply the final ELF-derived `R`, `RW`, `RX`, or `None` protection.

If mapping, copying, zeroing, or final protection fails, the loader attempts to unmap every range it created in that load attempt. Existing mappings are checked before mutation and are not owned or removed by the loader.

## Current tests

Synthetic host tests cover:

- valid `ET_DYN` load-bias calculation;
- valid fixed-address `ET_EXEC` loading;
- file-byte copy;
- BSS zero-fill;
- final RX/RW permission behavior;
- malformed identity/header fields;
- out-of-bounds program-header tables;
- `p_filesz > p_memsz`;
- out-of-bounds file ranges;
- guest-address overflow;
- bad segment alignment;
- unsupported RWX segment permissions;
- page-overlapping load segments;
- collisions with pre-existing guest mappings.

Android CI cross-builds the same loader into `liba32android.so`; actual ELF32 loading on Android is not yet a separate device smoke.

## Not implemented in this slice

- `PT_DYNAMIC` interpretation;
- dynamic symbol/string tables;
- DT_NEEDED dependency loading;
- ARM relocations;
- symbol lookup/interposition;
- RELRO handling;
- TLS segments;
- GNU/Android-specific dynamic-linker metadata;
- automatic guest-VA allocation for `ET_DYN`;
- loading a real Android ARM32 library and executing its entry/symbols.

Those items belong to later M3/M4 work and must preserve the loader/linker separation.
