# Android runtime smoke evidence — Termux/AArch64 — 2026-09-16

Raw evidence: `android-runtime-smoke-termux-arm64-2026-09-16.log`

## Observed

- The process reported `arch=aarch64` on Android release 16 / runtime SDK 36, with a 4096-byte host page size.
- `MappedGuestMemory` obtained a contiguous fastmem reservation and reported `fastmem.available=true` with base `0x6c42255000`.
- A32 `mov r0,#42` executed through the Android arm64 runtime path with `fastmem_enabled=true`; `r0` was 42 and `a32.return42.status=PASS`.
- The instruction fetch used one code callback (`a32.return42.code_callbacks=1`).
- A32 `STR`/`LDR` completed with `r2=0x12345678` and memory containing `0x12345678`.
- The mapped data path reported zero data-read callbacks and zero data-write callbacks, with `a32.memory.fastmem_direct=true` and `a32.memory.status=PASS`.
- The smoke reached `runtime_smoke.complete=true` without emitting a crash marker.
- The optional fastmem fault/fallback mode was not executed: `a32.fastmem_fault.status=NOT_RUN`.

## Inferred

- On this Android/AArch64 Termux environment, the pinned Dynarmic AArch64 backend can execute the tested A32 instruction sequences through `liba32android`.
- On this environment, the high-base contiguous 4 GiB reservation is usable by Dynarmic fastmem for the tested mapped data load/store path.
- Zero data callbacks together with the correct load/store result is strong evidence that the tested data accesses used the configured direct fastmem path rather than callback memory.

## Not demonstrated / remaining boundary

- Fastmem fault -> callback fallback on Android remains NOT RUN until `./run.sh --exercise-fastmem-fault` is executed successfully.
- Fatal `A32CRASH` marker behavior and Android tombstone coexistence remain NOT RUN.
- This is one Android/vendor/kernel environment and is not evidence of broad Android-device compatibility.
- Broader A32/Thumb/VFP/NEON correctness and real ELF/application compatibility are outside this smoke test.
