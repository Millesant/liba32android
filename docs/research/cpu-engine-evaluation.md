# CPU engine evaluation

Research date: 2026-09-15

## Requirement

The runtime needs an embeddable engine for AArch32/ARMv7 guest code with ARM and Thumb support, targeting an AArch64 Android host. The engine should leave ELF loading, Android ABI emulation and host API bridging to this project.

## Dynarmic

Assessment: **selected for M0**.

Directly observed in the pinned upstream fork:

- guest versions include ARMv7A and 32-bit ARMv8;
- host backends include AArch64 and x86-64;
- Android is listed among supported systems;
- A32 callbacks cover memory reads/writes, SVC, exceptions, timing and interpreter fallback;
- configuration exposes page-table and 4 GiB fastmem mechanisms;
- JIT API exposes `Run`, `Step`, `ClearCache` and `InvalidateCacheRange`;
- source is C++20 and Dynarmic itself is licensed 0BSD.

The Azahar organization showed the fork updated on 2026-06-24. Azahar CI in 2026 also produces Android arm64 artifacts, which is corroborating evidence for the surrounding toolchain, not proof that this repository's integration builds until our own CI runs.

## QEMU user mode

Assessment: **reference/alternative, not selected as the embedded M0 engine**.

QEMU has mature ARM user-mode translation and is valuable as a behavioral reference. The QEMU emulator as a whole is GPLv2, and its user-mode/process integration surface is substantially broader than the narrow CPU-library boundary desired here. Selecting it would change both integration complexity and licensing obligations.

## Unicorn

Assessment: **credible embedding/reference alternative, not selected**.

Unicorn exposes an embeddable multi-architecture CPU emulation API, supports ARM and Android, and is based on QEMU. Its project states GPLv2 licensing. For this project Dynarmic is a closer fit because the needed A32-to-AArch64 JIT path and memory hooks are available with a permissive core license and without adopting a QEMU-derived engine boundary.

## FEX and Box64

Assessment: **useful architectural references, not CPU-engine candidates for this guest ISA**.

FEX targets x86/x86-64 guests on Arm64. Box64 targets x86-64 Linux guests on non-x86-64 64-bit hosts such as Arm64. Their thunking, library-forwarding and memory-model ideas are relevant research material, but their guest ISA is not AArch32.

## MAMBO / DBT research

Assessment: **research reference only for now**.

MAMBO-family work is relevant to dynamic binary translation and instrumentation concepts, but M0 prefers a maintained embeddable A32 engine with a direct library API and existing Android/AArch64 evidence. No MAMBO component is integrated.

## Licensing note

This document records dependency licenses, not the eventual license of `liba32android` itself. The project license has not yet been selected and must be decided before a public release.

## Sources

- Dynarmic: https://github.com/azahar-emu/dynarmic
- Dynarmic pinned commit: https://github.com/azahar-emu/dynarmic/commit/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516
- QEMU license: https://github.com/qemu/qemu/blob/master/LICENSE
- Unicorn: https://github.com/unicorn-engine/unicorn
- FEX: https://github.com/FEX-Emu/FEX
- Box64: https://github.com/ptitSeb/box64
