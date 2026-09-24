# Proposal — ELF32 ARM eager JUMP_SLOT relocation application

## Intent

Close the next bounded M4 dynamic-linker gap above feature 008 by consuming the validated PLT REL descriptor and eagerly resolving/applying ARM `R_ARM_JUMP_SLOT` entries.

This is not a lazy-binding feature. The runtime resolves every supported PLT slot during the explicit apply call, using the same graph-local symbol scope and logical guest-address model already validated for feature 007.

## Why this slice now

Feature 007 proved bounded symbol-bearing relocation resolution plus transactional guest writes for the main `DT_REL` table. Feature 008 then validated `DT_JMPREL` / `DT_PLTRELSZ` / `DT_PLTREL=DT_REL` as a distinct guest-only table descriptor.

The smallest coherent continuation is therefore to reuse those contracts for the single ARM PLT relocation type required for eager function binding rather than broadening into lazy resolver state, process-wide namespaces, or additional relocation families.

## Machine/ABI posture

- Architecture: Arm AArch32 / ELF32 little-endian `EM_ARM`.
- Dynamic PLT form: `Elf32_Rel`.
- Supported PLT relocation: `R_ARM_JUMP_SLOT` (22).
- Resolution scope: existing feature-006 graph-local BFS.
- Write model: logical 32-bit guest values through `GuestMemory`.
- Android compatibility target: bionic eager result writes the resolved symbol address without a REL in-place addend.
- AAELF32: REL-form `R_ARM_JUMP_SLOT` has addend zero and resolves to the symbol address.

## Non-goals

No lazy binding, PLT resolver trampoline, `DT_PLTGOT` protocol, process-wide link-map lifetime, namespace/global-group policy, symbol-version matching, protected requester semantics, IFUNC execution, TLS, RELRO, COPY/REL32/instruction relocations, Android packed relocations, constructors, or guest execution.
