# Android runtime smoke — fastmem fault/fallback evidence

Date: 2026-09-16
Environment: Termux on Android 16 / runtime SDK 36 / AArch64
Raw log: `android-runtime-smoke-fastmem-fallback-termux-arm64-2026-09-16.log`

## Observed

- `fastmem.available=true` with a high host reservation base (`0x6d2c814000`).
- A32 return-42 remained PASS through the pinned Dynarmic AArch64 backend.
- The mapped A32 `STR`/`LDR` case remained PASS with zero data callbacks and `a32.memory.fastmem_direct=true`.
- The optional fault case reported `a32.fastmem_fault.memory_fault=true`.
- The fault case reported `a32.fastmem_fault.data_read_callbacks=1`.
- The fault case reported `a32.fastmem_fault.status=PASS`.
- The process reached `runtime_smoke.complete=true` and did not emit `A32CRASH`.

## Inferred from the observed behavior and implementation

- On this environment, a protected/unmapped guest-data access taken through Dynarmic fastmem was recovered by Dynarmic's fastmem exception path, re-routed to the ordinary guest-memory callback path, and surfaced as a guest `memory_fault` rather than terminating the Android process.
- This supplies the missing on-device evidence for the mandatory callback correctness fallback selected by D-0004.

## Not demonstrated

- Broad Android/vendor/kernel compatibility; this remains one Android 16 / SDK 36 AArch64 Termux environment.
- Broad A32/Thumb/VFP/NEON correctness.
- Fatal `A32CRASH` marker plus Android tombstone/backtrace coexistence; no fatal process crash occurred.
- ELF32 loader, relocations, dynamic linking, ABI bridge, Android API bridges, or application compatibility.

## M2 consequence

For the known Android environment, the M2 address-space requirements exercised by the current design now have direct device evidence for contiguous high-base reservation, mapped direct-fastmem data access, and fastmem fault -> callback fallback. Direct low-VA host/guest pointer identity remains outside the correctness contract.
