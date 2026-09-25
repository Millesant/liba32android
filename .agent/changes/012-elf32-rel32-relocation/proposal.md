# Proposal — ELF32 ARM R_ARM_REL32 relocation

## Intent

Extend the already-transactional main `DT_REL` path with one additional data relocation, ARM `R_ARM_REL32` (type 3), without broadening into instruction encodings or new relocation table formats.

## Why this slice now

Feature 011 closed the main+PLT transaction boundary. Of the remaining relocation gaps, REL32 reuses the existing symbol-reference and 32-bit data-write machinery and adds only the place-relative formula plus the AAELF32 Thumb discriminator. It is a smaller coherent continuation than COPY, packed relocation decoding, TLS, IFUNC, or instruction relocations.

## Non-goals

No COPY, branch/instruction relocations, RELA/RELR/APS2, TLS, IFUNC, lazy binding, page-permission broadening, namespace/global-group policy, constructors, unload, or guest execution.
