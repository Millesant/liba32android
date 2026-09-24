# ELF32 loader architecture

Status: current through feature 010; validated `PT_LOAD`/`PT_DYNAMIC`/`PT_GNU_RELRO` planning and mapping metadata are implemented, with automatic `ET_DYN` placement and post-load RELRO sealing kept as separate layers

## Boundary

The ELF32 loader is a guest-address-space component. It consumes an in-memory ELF32 byte image and maps logical 32-bit guest virtual addresses through `memory::MappedGuestMemory`.

It remains independent from Dynarmic types, host pointers, dynamic symbol resolution, ARM relocation application, Android compatibility bridges, JNI, graphics/audio and application profiles.

Current dependency direction:

```text
ELF32 byte image
      |
      v
 elf32_load_plan
 validated PT_LOAD/PT_DYNAMIC layout
      |
      +----> elf32_dynamic_placement ----> explicit dynamic_base
      |                                      |
      v                                      v
 elf32_loader -----------------------> MappedGuestMemory
      |                                      |
      +----> validated PT_DYNAMIC guest range|
      |                    |                 v
      v                    v            logical guest VAs
 mapped guest image   elf32_dynamic
                     structural raw tags
                            |
                            v
                      future linker layer
```

The loader itself does not parse `Elf32_Dyn` entries. Structural parsing is the responsibility of `src/elf/elf32_dynamic.*`, and dynamic-linker semantics remain a later layer above both mapping and structural parsing.

## Supported image policy

The loader currently accepts:

- ELF32 class;
- little-endian encoding;
- current ELF identification/header version;
- `EM_ARM`;
- `ET_DYN` and `ET_EXEC`;
- standard ELF32 header/program-header sizes;
- `PT_LOAD` segments whose final permissions are `None`, `R`, `RW`, or `RX`;
- either no `PT_DYNAMIC`, or exactly one validated non-empty `PT_DYNAMIC` range contained in a readable `PT_LOAD`;
- zero or more validated non-empty `PT_GNU_RELRO` ranges whose host-page-rounded coverage is already contained in readable `PT_LOAD` mappings.

`ET_EXEC` loads at fixed guest virtual addresses with `load_bias == 0`.

`ET_DYN` mapping still requires an explicit host-page-aligned `dynamic_base`. That value identifies where the lowest host-page-aligned `PT_LOAD` mapping begins. The loader computes `load_bias = dynamic_base - lowest_load_page` and requires the resulting load bias to preserve every `PT_LOAD p_align` congruence requirement. Callers that do not already own a process-layout policy may obtain that value from the separate `elf32_dynamic_placement` layer; the loader itself does not choose addresses.

That extra rule matters when ELF segment alignment is larger than the current host page size. The reproducible ARMv7 NDK fixture uses `p_align=0x4000`; a merely 4 KiB-aligned base is therefore not necessarily a valid load bias.

The result exposes guest entry/load-bias/PT_LOAD metadata, optional `PT_DYNAMIC` guest metadata, and ordered load-biased `PT_GNU_RELRO` descriptors containing both exact and host-page-rounded guest ranges. Host reservation pointers are never part of the loader API. The loader does not seal RELRO; relocation-time writability is preserved until the explicit downstream hardening call.

## PT_DYNAMIC discovery policy

Missing `PT_DYNAMIC` is valid.

When `PT_DYNAMIC` is present, the loader validates it before guest mappings are mutated:

- only one `PT_DYNAMIC` program header is accepted;
- `p_memsz` must be non-zero;
- `p_filesz <= p_memsz`;
- its file-backed range must be within the input image;
- its un-biased and biased guest ranges must fit the 32-bit guest address space;
- its complete `p_memsz` range must be contained inside a `PT_LOAD` memory range;
- the containing `PT_LOAD` must be readable;
- when `p_filesz` is non-zero, the `PT_DYNAMIC p_offset`/`p_filesz` range must be exactly the file bytes that the containing `PT_LOAD` maps at the same `PT_DYNAMIC p_vaddr`, not unrelated bytes or a BSS-only tail.

On success, `Elf32LoadResult::dynamic_segment` reports only:

- biased guest virtual address;
- `p_filesz`;
- `p_memsz`.

`elf32_dynamic` may then consume that range through `GuestMemory`. The loader does not interpret `DT_NEEDED`, `DT_REL`, symbols, strings, GNU hash, or any other tag semantics.

## Shared load planning and automatic placement

`src/elf/elf32_load_plan.*` owns pre-mutation ELF identity/program-header/`PT_LOAD`/`PT_DYNAMIC`/`PT_GNU_RELRO` validation. Its immutable result exposes the ELF type, entry, lowest mapped load page, maximum mapped end, combined required load-bias alignment, validated load segments, optional validated dynamic segment, and ordered validated RELRO ranges.

`src/elf/elf32_dynamic_placement.*` consumes that same plan plus `MappedGuestMemory` state. It performs a caller-bounded, deterministic low-to-high first-fit search through `memory::find_free_guest_range`, preserving host-page alignment and every accepted `PT_LOAD p_align` constraint. Placement is non-mutating: it returns only a `dynamic_base`; `load_elf32` remains the mapping authority and can still report `AddressConflict` if memory changes after placement.

The default placement search begins at guest VA `0x10000` and ends at the 32-bit guest-address-space limit. Callers may provide a narrower explicit window. Recursive dependency graph/link-map ownership and complete process-layout policy remain later layers.

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
- `ET_DYN` host-page alignment, load-bias range and every `PT_LOAD p_align` constraint;
- entry-point overflow;
- the `PT_DYNAMIC` policy above when present;
- every `PT_GNU_RELRO` exact/range overflow condition and page-rounded readable-`PT_LOAD` coverage requirement;
- page-overlapping `PT_LOAD` ranges;
- conflicts with pages already mapped in `MappedGuestMemory`.

The loader deliberately rejects page-overlapping `PT_LOAD` ranges. Handling shared boundary pages with merged content/permissions is deferred until a concrete compatibility requirement demonstrates the need; silent permission broadening is not allowed.

The current real fixture does not require shared-page handling. Its four `PT_LOAD` ranges occupy distinct windows at the observed 4 KiB Linux host granularity and, by calculation from the program headers, at 16 KiB granularity. That 16 KiB statement is an inference from ELF metadata, not a 16 KiB-host runtime test.

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
- rejection of multiple, empty, file-out-of-bounds, address-overflowing, outside-`PT_LOAD`, non-readable and file-to-load-mismatched `PT_DYNAMIC` ranges;
- proof that malformed `PT_DYNAMIC` metadata is rejected before guest mappings are created;
- missing, valid, multiple, empty, overflowing, outside-load and non-readable GNU RELRO metadata cases, including proof that ordinary load leaves writable RELRO pages writable;
- file-byte copy and BSS zero-fill;
- final RX/RW permission behavior;
- malformed identity/header fields;
- out-of-bounds program-header tables;
- `PT_LOAD p_filesz > p_memsz`;
- out-of-bounds `PT_LOAD` file ranges;
- guest-address overflow;
- bad segment alignment;
- unsupported RWX permissions;
- page-overlapping load segments;
- collisions with pre-existing guest mappings.

A reproducible real fixture is generated from `tests/fixtures/arm32_loader_fixture.c` using the pinned Android NDK r27d / API 26 compiler and `-z max-page-size=16384`. CI builds it twice and requires byte-identical output before testing it.

Observed real-fixture properties include:

- ARM ELF32 `ET_DYN`;
- four `PT_LOAD` segments;
- all `PT_LOAD p_align=0x4000`;
- R, RX and RW load segments;
- BSS;
- one `PT_DYNAMIC` at pre-bias guest VA `0x826c`, with file/memory size `0x60`;
- GNU RELRO and ARM EXIDX program headers.

The real-fixture loader integration test verifies `PT_LOAD` metadata, exact copied bytes, BSS zero-fill, final guest permissions, a valid aligned load bias, rejection of a host-page-aligned bias that violates the fixture's 16 KiB `p_align`, exact biased `PT_DYNAMIC` metadata, and the observed load-biased GNU RELRO descriptor while proving it remains writable immediately after load. A separate automatic-placement integration plans the same fixture, requires `required_load_bias_alignment=0x4000`, selects a non-mutating first-fit `dynamic_base`, passes that exact base to `load_elf32`, and verifies the mapped result.

Structural dynamic-array behavior for the same fixture is tested separately by `tests/elf32_dynamic_real_fixture.cpp` and documented in `docs/architecture/elf32-dynamic.md`.

At integration commit `ac008b2d2a3158ffa4cb285e88cfabacea2ca4a2`, GitHub Actions run `35204765081` completed 20/20 CTest cases successfully, including both real-fixture loader and structural dynamic-array integration tests. Android CI also cross-built the same runtime/diagnostics successfully. Loading the fixture on a real Android device and actual 16 KiB Android host-page behavior remain NOT RUN.

Raw fixture evidence is documented under `docs/research/evidence/`.

## Downstream boundary

The loader boundary is intentionally stable. Recursive dependency graph loading, symbol lookup, main/PLT relocation application, and GNU RELRO sealing are implemented by separate downstream layers. The loader continues to own only structural image validation, guest mapping, and additive guest-only metadata needed by those layers.

## Not implemented in the loader

- dynamic string/symbol table semantics;
- `DT_NEEDED` dependency loading;
- ARM relocations;
- symbol lookup/interposition;
- RELRO enforcement;
- TLS processing;
- GNU/Android-specific linker semantics;
- loading the real fixture through the runtime on a real Android device;
- executing loaded ARM32 fixture symbols.

Those belong to separate linker/runtime layers and must preserve the established address-space and loader boundaries. Several are already implemented downstream; they intentionally remain outside `load_elf32` itself.
