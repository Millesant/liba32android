# Runtime contract

Status: Accepted current project contract
Last reconciled: 2026-09-23

## L32-R001 — Game-agnostic runtime

The generic runtime remains application-agnostic. Application/game-specific behavior belongs under `profiles/` and must not leak into CPU, memory, ELF, ABI, compatibility-library, or platform contracts.

## L32-R002 — Logical guest addresses

AArch32 guest addresses are logical 32-bit values. CPU, memory, ELF, ABI, and runtime APIs must not expose host pointers as guest pointer values.

## L32-R003 — CPU isolation

Dynarmic remains behind `src/cpu/`. Higher layers depend on engine-independent contracts and may not expose Dynarmic types.

## L32-R004 — Guest-memory seam

`memory::GuestMemory` is the generic memory contract. `LinearGuestMemory` is the deterministic correctness/test backend. `MappedGuestMemory` owns mapped guest pages, permissions, and optional high-base 4 GiB fastmem backing.

## L32-R005 — Fastmem is optional

High-base contiguous fastmem is the preferred first acceleration path when available, but callback-backed access remains the correctness fallback. Direct low-VA host-pointer identity is not a correctness requirement.

## L32-R006 — Permission integrity

Runtime layers must not silently broaden guest permissions to make fixtures or compatibility cases pass. Mapping, relocation, and post-relocation hardening remain explicit stages.

## L32-R007 — Shared-library identity

The runtime target produces exactly `liba32android.so`. Android cross-build validation targets `arm64-v8a`; host validation remains separate from Android device/runtime claims.

## L32-R008 — Evidence scope

A passing cross-build, emulator sample, or device sample proves only the stated revision/environment. Broader Android compatibility claims require broader evidence.
