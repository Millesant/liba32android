# Decisions

## D-0001 — Use Dynarmic behind an internal CPU adapter

Status: Accepted
Date: 2026-09-15

### Context

The runtime needs AArch32 ARM/Thumb execution inside an eventual AArch64 Android process. Rebuilding a decoder, IR, optimizer, AArch64 emitter and code cache is unnecessary if a maintained embeddable engine satisfies the CPU requirements.

### Decision

Use Dynarmic for M0, pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`, and isolate all Dynarmic-specific types under `src/cpu/`.

### Rationale

The selected Dynarmic fork directly exposes A32 guest support, AArch64 host support, Android support, memory callbacks/fastmem/page-table mechanisms, cache invalidation and a C++ embedding API. Dynarmic itself is 0BSD licensed. QEMU and Unicorn remain useful references but impose a larger/QEMU-derived integration and GPLv2 licensing surface; FEX and Box64 target x86-family guests rather than AArch32.

### Consequences

- CPU execution can advance independently from ELF and Android compatibility layers.
- Dynarmic must not leak into higher-level runtime APIs.
- Engine inaccuracies documented upstream require targeted regression tests and reference comparisons.
- Dependency commit and license must remain auditable.

### Supersedes / Superseded by

None.
