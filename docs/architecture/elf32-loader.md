# ELF32 loader architecture

Status: M3 PT_LOAD mapping plus PT_DYNAMIC range discovery implemented and covered by synthetic images plus a reproducible real ARMv7/Android ET_DYN fixture

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
      +----> PT_DYNAMIC guest-range metadata only
      |
      v
 MappedGuestMemory
      |
      v
 logical AArch32 guest VAs
```

Dynamic linking remains a later layer above this mapping primitive rather than being folded into the loader.

## Supported image policy

The current slice accepts:

- ELF32 class;
- little-endian encoding;
- current ELF identification/header version;
- `EM_ARM`;
- `ET_DYN` and `ET_EXEC`;
- standard ELF32 header/program-header sizes;
- `PT_LOAD` segments whose final permissions are `None`, `R`, `RW`, or `RX`;
- either no `PT_DYNAMIC`, or exactly one validated non-empty `PT_DYNAMIC` range contained in a readable `PT_LOAD`.

`ET_EXEC` loads at its fixed guest virtual addresses with `load_bias == 0`.

`ET_DYN` requires the caller to provide an explicit host-page-aligned `dynamic_base`. That value identifies where the lowest host-page-aligned `PT_LOAD` mapping begins. The loader computes `load_bias = dynamic_base - lowest_load_page` and also requires that the resulting load bias preserve every `PT_LOAD p_align` congruence requirement. It returns guest metadata only and does not choose addresses through an allocator yet.

This additional load-bias rule is required even when an ELF segment alignment is larger than the current host page size. A real NDK-generated ARMv7 fixture with `p_align=0x4000` demonstrated that a 4 KiB-aligned base is not necessarily a valid 16 KiB-aligned load bias.

The result exposes guest entry/load-bias/PT_LOAD metadata plus optional `PT_DYNAMIC` guest metadata. Host reservation pointers are not part of the loader API.

## PT_DYNAMIC discovery policy

Missing `PT_DYNAMIC` is valid. This preserves support for ELF images that do not require dynamic-linker metadata.

When `PT_DYNAMIC` is present, the loader validates it before guest mappings are mutated:

- only one `PT_DYNAMIC` program header is accepted;
- `p_memsz` must be non-zero;
- `p_filesz <= p_memsz`;
- its file-backed range must be within the input ELF image;
- its un-biased and biased guest ranges must fit the 32-bit guest address space;
- its complete `p_memsz` range must be contained inside a `PT_LOAD` memory range;
- the containing `PT_LOAD` must be readable;
- when `p_filesz` is non-zero, the `PT_DYNAMIC p_offset`/`p_filesz` range must be exactly the file bytes that the containing `PT_LOAD` maps at the `PT_DYNAMIC p_vaddr`; it may not point at unrelated file bytes or into the load segment's BSS-only tail.

On success, `Elf32LoadResult::dynamic_segment` reports only:

- biased guest virtual address;
- `p_filesz`;
- `p_memsz`.

It deliberately does **not** parse `Elf32_Dyn` entries or interpret `DT_NEEDED`, `DT_REL`, symbol/string tables, GNU hash, or any other tag. Those belong to a later linker layer.

## Validation-before-mapping rule

Before mapping any guest page, the loader validates:

- ELF identity/class/data/version;
- supported type and `EM_ARM` machine;
- header and program-header entry sizes;
- program-header table bounds;
- `PT_LOAD p_filesz <= p_memsz`;
- `PT_LOAD` file ranges within the input byte image;
- 32-bit guest-address overflow;
- ELF segment alignment constraints;
- supported segment permission shapes;
- ET_DYN host-page alignment, load-bias range, and every `PT_LOAD p_align` constraint;
- entry-point overflow;
- the `PT_DYNAMIC` policy above when present;
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
- missing `PT_DYNAMIC` as a valid case;
- valid biased `PT_DYNAMIC` result metadata;
- rejection of multiple, empty, file-out-of-bounds, address-overflowing, outside-PT_LOAD, non-readable, and file-to-load-mismatched `PT_DYNAMIC` ranges;
- proof that malformed PT_DYNAMIC metadata is rejected before guest mappings are created;
- file-byte copy;
- BSS zero-fill;
- final RX/RW permission behavior;
- malformed identity/header fields;
- out-of-bounds program-header tables;
- `PT_LOAD p_filesz > p_memsz`;
- out-of-bounds PT_LOAD file ranges;
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
- one `PT_DYNAMIC` range at pre-bias guest VA `0x826c`, file/memory size `0x60`;
- GNU RELRO and ARM EXIDX program headers.

The real fixture integration test verifies PT_LOAD metadata, exact copied file bytes, BSS zero-fill, final guest permissions, a valid aligned load bias, rejection of a load bias that is host-page-aligned but violates the fixture's 16 KiB `p_align`, and exact biased `PT_DYNAMIC` result metadata. The loader also verifies that the real fixture's PT_DYNAMIC file range is the same file-backed range exposed through its containing PT_LOAD.

Raw/current evidence is documented in `docs/research/evidence/arm32-loader-fixture-ndk-r27d-2026-09-16.md`.

Android CI cross-builds the same loader into `liba32android.so`; actual ELF32 loading on an Android device and operation on an actual 16 KiB Android host page configuration remain NOT RUN.

## Next M3 boundary

The next M3 metadata slice may parse the dynamic array structure itself into validated guest-address metadata needed by the future linker, but it must not perform dependency loading, symbol resolution, or relocations as a side effect of ELF mapping.

## Not implemented in this slice

- `Elf32_Dyn` tag interpretation;
- dynamic symbol/string table parsing as linker metadata;
- DT_NEEDED dependency loading;
- ARM relocations;
- symbol lookup/interposition;
- RELRO enforcement;
- TLS segments;
- GNU/Android-specific dynamic-linker metadata processing;
- automatic guest-VA allocation for `ET_DYN`;
- loading the real fixture through the runtime on a real Android device;
- executing loaded ARM32 fixture symbols.

Those items belong to later M3/M4 work and must preserve the loader/linker separation.