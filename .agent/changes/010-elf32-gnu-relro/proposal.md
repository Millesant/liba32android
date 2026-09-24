# Proposal — ELF32 GNU RELRO protection

## Intent

Close the next bounded M4 dynamic-linker hardening gap by validating ELF32 `PT_GNU_RELRO` program-header ranges and providing an explicit post-relocation operation that seals those guest pages read-only.

This feature preserves the current eager-linking model. It does not introduce lazy PLT resolution or a new process-wide linker orchestrator.

## Why this slice now

Features 007 and 009 now complete bounded eager main-REL and PLT JUMP_SLOT application. The existing pinned ARMv7 fixture already contains a GNU RELRO program header, and `MappedGuestMemory` already exposes page-granular protection changes.

The smallest coherent continuation is therefore to expose validated GNU RELRO ranges through the loader result, then seal them only after relocation application. This advances Android-compatible linker behavior without broadening symbol scope, relocation families, or guest execution.

## Machine/runtime posture

- Architecture: Arm AArch32 / ELF32 little-endian `EM_ARM`.
- Program header: `PT_GNU_RELRO` (`0x6474e552`).
- Granularity: current `MappedGuestMemory::page_size()`.
- Range rule: page-start of `p_vaddr` through page-end of `p_vaddr + p_memsz`.
- Loader behavior: validate and publish metadata only; do not seal during load.
- Sealing behavior: explicit caller action after relocations, producing read-only pages without permission broadening.
- Android compatibility target: bionic applies GNU RELRO after relocations by page-rounding the program-header range and protecting it read-only.

## Planned slices

- T001: validate/expose GNU RELRO ranges through the shared load plan and loader result, including the existing real fixture oracle.
- T002: add bounded transactional GNU RELRO page sealing with preflight validation and idempotence.
- T003: apply real fixture relocations, seal its observed GNU RELRO, and prove relocated bytes remain intact while writes are rejected.
- T004: converge docs/spec/state/evidence and pass the final exact-head CI gate.

## Non-goals

No lazy binding or `DT_PLTGOT` protocol, RELRO sharing/serialization through Android extension file descriptors, symbol-version matching, namespace/global-group/process-wide interposition policy, protected requester semantics, TLS, IFUNC, constructors, broader relocation forms, text-relocation permission broadening, automatic guest execution, or a combined process-wide relocation+RELRO transaction.
