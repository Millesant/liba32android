# ELF32 ARM REL relocation evidence — 2026-09-23

Status: observed/reference evidence for feature 007

## Decision supported

Define the first bounded ARM ELF32 main-`DT_REL` relocation feature and choose Android-compatible semantics for the relocation forms in scope.

## Project artifact identity

Source revision: `ecdae1cea991ecd079487e1cda0bc2fe3c7fff99`

GitHub Actions: CI #208 / run `35848346075`

Artifact:

- name: `arm32-loader-fixture-ecdae1cea991ecd079487e1cda0bc2fe3c7fff99`
- ID: `10745105004`
- artifact digest: `sha256:425dff403ae29d3704a62daa254dcdc147d0a068276cf52afe394d66b2db277f`
- fixture SHA-256: `6c2dbda2dec94eaa022ad09391ed3988c3a41828e5c1c065fc0101e5725b84c2`

Tool observation used host `readelf` against the artifact bytes downloaded from the exact run.

## Observed dynamic relocation table

`readelf -dW`:

```text
DT_REL    0x204
DT_RELSZ  16
DT_RELENT 8
DT_SYMTAB 0x154
DT_STRTAB 0x1bc
DT_GNU_HASH 0x194
```

`readelf -rW`:

```text
Relocation section '.rel.dyn' at offset 0x204 contains 2 entries:
Offset     Info       Type            Symbol
000082cc  00000215   R_ARM_GLOB_DAT  fixture_bss
000082d0  00000315   R_ARM_GLOB_DAT  fixture_data
```

Therefore:

- entry count = 2;
- relocation type = 21 for both;
- symbol indexes = 2 and 3;
- linked target offsets = `0x82cc` and `0x82d0`.

The 32-bit words at both linked places are `0x00000000` in the file.

The fixture dynamic symbol table reports:

- symbol 2: `fixture_bss`, value `0x0000c2d8`, GLOBAL DEFAULT OBJECT;
- symbol 3: `fixture_data`, value `0x0000c2d4`, GLOBAL DEFAULT OBJECT.

## ARM ABI reference

Primary normative source:

Arm, *ELF for the Arm Architecture* (AAELF32), release 2025Q4, issued 23 January 2026.

Relevant observations:

- ELF32 relocation `r_info` encodes symbol index and relocation type;
- `R_ARM_ABS32` code 2 uses `(S + A) | T`;
- `R_ARM_GLOB_DAT` code 21 is a dynamic data relocation;
- `R_ARM_JUMP_SLOT` code 22 is the PLT/code-target dynamic relocation;
- `R_ARM_RELATIVE` code 23 uses a base/load-address adjustment plus the REL addend;
- ordinary dynamic data relocation places are word-aligned 32-bit objects;
- relocation expression values are ELF32-width values.

Locator: Arm-software/abi-aa, `aaelf32/aaelf32.rst`, 2025Q4 release.

## Android bionic compatibility reference

Primary implementation source:

Android bionic, `linker/linker_relocate.cpp`, snapshot `25a5023ac2049f1e25a3a0f7ee75da5899ba5049`.

Observed current behavior:

- ARM maps generic absolute to `R_ARM_ABS32`;
- generic GLOB_DAT maps to `R_ARM_GLOB_DAT`;
- generic RELATIVE maps to `R_ARM_RELATIVE`;
- absolute uses the REL addend;
- RELATIVE uses `load_bias + REL addend`;
- bionic explicitly comments that AAELF32 gives ARM GLOB_DAT an addend but bionic does not add it;
- GLOB_DAT therefore writes the resolved symbol address without the REL in-place addend.

This is the basis for feature 007 choosing Android-compatible `R_ARM_GLOB_DAT = S` semantics.

## Evidence classification

### Observed

- The exact current project fixture has two and only two main dynamic REL relocations.
- Both are `R_ARM_GLOB_DAT`.
- They reference `fixture_bss` and `fixture_data`.
- Their in-place words are zero.
- The project already validates the REL table descriptor and resolves both symbols through feature 006.
- Current bionic intentionally suppresses the ARM GLOB_DAT REL addend.

### Inferred

- Applying the two fixture GLOB_DAT relocations should write the feature-006 resolved logical guest values into the two GOT words.
- This can be validated entirely on the Linux host without executing ARM code.

### Not demonstrated

- PLT/JMPREL/JUMP_SLOT behavior.
- REL32, COPY, IRELATIVE, TLS, RELR, or Android packed relocations.
- Protected self-binding or versioned symbol resolution.
- Text relocations or temporary permission changes.
- RELRO enforcement.
- Guest execution after relocation.

## Invalidation conditions

Revisit the feature requirements if:

- the pinned fixture build recipe/toolchain changes and its relocation table changes;
- Android bionic changes ARM GLOB_DAT addend behavior;
- the project deliberately targets generic AAELF32 semantics over Android runtime compatibility;
- a required real target introduces a relocation form currently outside feature 007.
