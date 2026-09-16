# ELF32 loader architecture

Status: first M3 PT_LOAD slice implemented and tested with synthetic images plus a reproducible real ARMv7/Android ET_DYN fixture

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

The current slice accepts:

- ELF32 class;
- little-endian encoding;
- current ELF identification/header version;
- `EM_ARM`;
- `ET_DYN` and `ET_EXEC`;
- standard ELF32 header/program-header sizes;
- `PT_LOAD` segments whose final permissions are `None`, `R`, `RW`, or `RX`.

`ET_EXEC` loads at its fixed guest virtual addresses with `load_bias == 0`.

`ET_DYN` requires the caller to provide an explicit host-page-aligned `dynamic_base`. That value identifies where the lowest host-page-aligned `PT_LOAD` mapping begins. The loader computes `load_bias = dynamic_base - lowest_load_page` and also requires that the resulting load bias preserve every `PT_LOAD p_align` congruence requirement. It returns guest metadata only and does not choose addresses through an allocator yet.

This additional load-bias rule is required even when an ELF segment alignment is larger than the current host page size. A real NDK-generated ARMv7 fixture with `p_align=0x4000` demonstrated that a 4 KiB-aligned base is not necessarily a valid 16 KiB-aligned load bias.

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
- ET_DYN host-page alignment, load-bias range, and every `PT_LOAD p_align` constraint;
- entry-point overflow;
- page-overlapping `PT_LOAD` ranges;
- conflicts with pages already mapped in `MappedGuestMemory`.

The current slice deliberately rejects page-overlapping `PT_LOAD` ranges. Handling shared boundary pages with merged permissions/content is deferred until a concrete compatibility requirement demonstrates the need; silent permission broadening is not allowed.

The current real fixture does not require shared-page handling. Its four `PT_LOAD` ranges are separate at both the observed 4 KiB host granularity and, by calculation from the program headers, at 16 KiB granularity. That 16 KiB statement is an inference from the headers, not a 16 KiB-host runtime test.

## Mapping lifecycle

For each validated non-empty `PT_LOAD` segment:

1. compute the host-page-aligned guest mapping range;
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

A reproducible real fixture is generated from `tests/fixtures/arm32_loader_fixture.c` using the project-pinned Android NDK r27d / API 26 compiler and `-z max-page-size=16384`. CI builds it twice and requires byte-identical output before testing it.

Observed real fixture properties include:

- ARM ELF32 `ET_DYN`;
- four `PT_LOAD` segments;
- all `PT_LOAD p_align=0x4000`;
- R, RX and RW load segments;
- BSS;
- `PT_DYNAMIC`;
- GNU RELRO and ARM EXIDX program headers.

The real fixture integration test verifies PT_LOAD metadata, exact copied file bytes, BSS zero-fill, final guest permissions, a valid aligned load bias, and rejection of a load bias that is host-page-aligned but violates the fixture's 16 KiB `p_align`.

Raw/current evidence is documented in `docs/research/evidence/arm32-loader-fixture-ndk-r27d-2026-09-16.md`.

Android CI cross-builds the same loader into `liba32android.so`; actual ELF32 loading on an Android device and operation on an actual 16 KiB Android host page configuration remain NOT RUN.

## Next M3 boundary

The real fixture contains `PT_DYNAMIC` and real dynamic-table metadata. The next loader step may identify and return the loaded `PT_DYNAMIC` guest range/metadata location so a later linker layer can consume it.

That step must remain metadata discovery only. Dynamic tags, DT_NEEDED dependencies, symbols and relocations must not be resolved as a side effect of PT_LOAD mapping.

## Not implemented in this slice

- `PT_DYNAMIC` interpretation beyond observing it in the fixture;
- dynamic symbol/string tables as loader output;
- DT_NEEDED dependency loading;
- ARM relocations;
- symbol lookup/interposition;
- RELRO enforcement;
- TLS segments;
- GNU/Android-specific dynamic-linker metadata processing;
- automatic guest-VA allocation for `ET_DYN`;
- loading this fixture through the runtime on a real Android device;
- executing loaded ARM32 fixture symbols.

Those items belong to later M3/M4 work and must preserve the loader/linker separation.
