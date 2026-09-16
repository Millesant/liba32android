# Current State

Last updated: 2026-09-16
Current milestone: M2 guest address space
Integration branch: `bleeding`

## Working / proven

- Dynarmic remains isolated behind the internal CPU adapter and pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.
- D-0003 remains accepted: AArch32 guest VAs are logical 32-bit values; guest pointer == host pointer is not a generic-runtime requirement.
- D-0004 remains accepted: prefer a contiguous high-base 4 GiB reservation as the first Android fastmem acceleration path when available, while retaining callback memory as the mandatory correctness fallback.
- `memory::GuestMemory` is the engine-independent CPU/memory boundary and separates instruction reads from data reads/writes plus an optional internal `fastmem_base()` capability.
- `LinearGuestMemory` remains the deterministic callback/correctness implementation and exposes no fastmem capability.
- `MappedGuestMemory` is IMPLEMENTED. It owns one contiguous 4 GiB host reservation, keeps unmapped guest pages `PROT_NONE`, tracks page mapping/permission metadata, and provides page-aligned map/protect/unmap lifecycle operations.
- The mapped backend accepts the initial ELF-like permission shapes needed for `R`, `RW`, and `RX`; write-only and execute-only mappings are intentionally rejected by this first direct-fastmem backend.
- Dynarmic fastmem integration is IMPLEMENTED behind the generic memory capability. When a backend exposes a reservation, the adapter sets `UserConfig::fastmem_pointer` and keeps `recompile_on_fastmem_failure=true`.
- Linux tests prove callback-only and fastmem-capable backends produce the expected guest-visible A32 load/store behavior, including fastmem fault -> callback fallback for unmapped guest data.
- `android_runtime_smoke` is IMPLEMENTED as an Android arm64 executable linked to the real `liba32android.so` and packaged with a Termux launcher.
- Raw Android runtime-smoke evidence is stored in `docs/research/evidence/android-runtime-smoke-termux-arm64-2026-09-16.log`; the evidence classification is stored beside it in `.md` form.
- On the observed Android 16 / runtime SDK 36 / AArch64 Termux environment, A32 `mov r0,#42` executed through `liba32android` / Dynarmic's AArch64 backend and returned 42.
- On that same environment, mapped A32 `STR`/`LDR` completed with `r2=0x12345678` and stored `0x12345678`; both data callback counts were zero and `a32.memory.fastmem_direct=true`.
- The Android runtime smoke reached `runtime_smoke.complete=true` without a crash marker.
- The shared runtime produces exactly `liba32android.so`; CI rejects the former duplicated filename.
- Raw first-device address-space evidence remains stored in `docs/research/evidence/android-termux-arm64-2026-09-16.log` with analysis beside it.

## Validation / evidence status

### Real Android/AArch64 runtime evidence (2026-09-16)

Environment reported by the runtime smoke:

- Android runtime SDK: 36
- Android release: 16
- Architecture: `aarch64`
- Host page size: 4096 bytes
- Kernel release: `6.6.102-android15-8-abA566EXXSDCZHB-4k`
- Fastmem reservation: PASS / available at high host VA `0x6c42255000`
- A32 return-42 through Dynarmic AArch64 backend: PASS
- `a32.return42.r0=42`: PASS
- A32 mapped `STR`/`LDR`: PASS
- Direct fastmem data path (`data_read_callbacks=0`, `data_write_callbacks=0`): PASS
- `a32.memory.fastmem_direct=true`: PASS
- Runtime smoke completion: PASS
- Optional fastmem fault -> callback fallback on Android: NOT RUN
- `A32CRASH|...` fatal-signal emission: NOT RUN
- Android tombstone/backtrace coexistence: NOT RUN

### Real Android/AArch64 primitive evidence (2026-09-16)

- Address-space probe execution from Termux: PASS
- Complete explicit file log creation: PASS
- `/proc/sys/vm/mmap_min_addr` read: BLOCKED by `EACCES`
- Contiguous 4 GiB reservation: PASS at a high host VA
- Commit/read/write a page inside the 4 GiB reservation: PASS
- Unmap 4 GiB reservation: PASS
- Sampled low-VA `MAP_FIXED_NOREPLACE`: PASS at all six requested addresses from `0x10000` through `0x80000000`
- Collision behavior at sampled low VAs: PASS / `EEXIST`
- RW->RX transition: PASS
- Generated AArch64 execution: PASS, result 42

### Mapped-memory / fastmem GitHub validation

Final pre-evidence implementation head `686c06cf5fb4cd804bf039faf6f110bb2533ba24`: GitHub Actions run `35081829133` (#35) completed PASS.

- Linux configure/build: PASS
- Linux shared-library filename check: PASS
- Linux CTest: 10/10 PASS
  - existing 8 ARM/Thumb/memory tests: PASS
  - `mapped_guest_memory_lifecycle`: PASS
  - `dynarmic_fastmem_and_fallback`: PASS
- Linux callback-only A32 load/store baseline: PASS
- Linux mapped A32 fastmem load/store with zero data callbacks: PASS
- Linux unmapped fastmem access -> callback fallback -> `memory_fault`: PASS
- Android `arm64-v8a` runtime + address-space probe + runtime-smoke configure/build/link: PASS
- Android shared-library filename check: PASS
- Android runtime-smoke diagnostic marker verification: PASS
- Android runtime-smoke `DT_NEEDED` reference to `liba32android.so`: PASS
- Android runtime, probe, and runtime-smoke artifact publication: PASS

Earlier implementation checkpoint run `35080732804` (#27) also passed Linux 10/10 and Android compile/link/package. Run #25 (`35080280754`) failed only a CI assertion requiring `$ORIGIN` RPATH; packaging was corrected to an explicit `LD_LIBRARY_PATH` launcher. Run #26 was superseded/cancelled.

## Evidence boundary

The Android runtime-smoke result directly proves the tested A32 return-42 and mapped STR/LDR paths execute through the pinned Dynarmic AArch64 backend on one Android 16 / SDK 36 Termux environment. The zero data-callback counts plus correct memory result directly support the tested direct-fastmem data path on that environment.

This does not establish broad Android compatibility, broad ISA correctness, ELF/application compatibility, or Android fastmem-fault recovery. The optional device fault/fallback test remains NOT RUN even though that behavior is TESTED/PASS on Linux.

Low-VA identity remains optional even though sampled low addresses succeeded in one process. The generic loader/ABI/runtime continues to use logical guest VAs.

## Partially working / not implemented

- M1 instruction coverage is PARTIAL: basic integer register state, ARM branch/call, stack and load/store paths are tested; broader Thumb/Thumb-2, VFP/NEON, exception and edge-case coverage remains future work.
- M2 guest address space is substantially advanced but still PARTIAL: mapped lifecycle and fastmem seam are implemented; tested execution is proven on Linux and one Android/AArch64 environment; wider-device compatibility and Android fastmem-fault fallback remain unproven.
- Callback memory: IMPLEMENTED / TESTED as correctness path.
- High-base 4 GiB fastmem strategy: IMPLEMENTED; primitive feasibility and tested A32 direct-fastmem execution PROVEN on one Android environment.
- Dynarmic page-table integration: NOT IMPLEMENTED; retained as a possible secondary acceleration path if later evidence requires it.
- Direct low-VA pointer identity: sampled on one device process but remains experimental and outside the correctness contract.
- ELF32 loader/linker and ARM relocations: NOT IMPLEMENTED.
- Guest AAPCS32 <-> host AAPCS64 ABI bridge and `host_add(20,22)` proof: NOT IMPLEMENTED.
- libc/libm/libdl/pthread/TLS/signals/JNI/EGL/GLES/OpenSL bridges: NOT IMPLEMENTED.
- Public/app-integrated runtime logging API: NOT IMPLEMENTED; current `A32ERR`/`A32CRASH` behavior is scoped to diagnostics executables.
- Application profiles and Minecraft-specific compatibility work: NOT IMPLEMENTED.
- Project-level open-source license selection: NOT IMPLEMENTED.

## Current blocker

No external blocker remains for normal A32 execution or mapped direct-fastmem proof on the known Android environment. The next device-only validation is the optional fastmem fault -> callback fallback smoke mode. Broader compatibility still requires additional Android/vendor/kernel samples.
