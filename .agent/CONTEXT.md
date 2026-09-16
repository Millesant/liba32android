# Project Context

## Mission

Build a reusable, game-agnostic AArch32 compatibility runtime for AArch64 Android. The intended stack is ARM32 Android ELF -> ELF32 loader/linker -> guest runtime -> A32 execution engine -> AArch64 Android host.

## Architectural boundaries

Keep CPU execution, guest address space, ELF32 loading/linking, AAPCS32/AAPCS64 bridging, compatibility libraries, pthread/TLS, signals, JNI, graphics/audio, instrumentation and application profiles separate. Application-specific work belongs under `profiles/` and must never leak into the generic core.

## Current milestone

M3 ELF32 loading. M2 guest address space is complete for its current scope: the generic memory seam has callback and mapped 4 GiB implementations, Dynarmic fastmem integration, and direct Android/AArch64 evidence for both mapped fastmem data access and fastmem fault -> callback fallback on the known Android 16 / SDK 36 Termux environment. Broader device compatibility remains separate evidence work.

M3 begins with ELF32 validation, `PT_LOAD` mapping, BSS zero-fill, page permissions, and load bias. Dynamic symbol resolution/linking remains later work.

## CPU dependency

Dynarmic is selected behind `src/cpu/`, pinned to azahar-emu/dynarmic commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.

## Current memory seam

`memory::GuestMemory` is the engine-independent memory contract. Guest virtual addresses remain logical 32-bit values.

- `LinearGuestMemory` is the deterministic callback/correctness implementation used by focused tests.
- `MappedGuestMemory` owns a contiguous 4 GiB high-host-VA reservation, page mapping/permission metadata, and page-aligned map/protect/unmap lifecycle operations.
- The mapped backend can expose its reservation base only through the internal `fastmem_base()` capability used by the CPU adapter; loader/ABI/runtime interfaces must not expose host pointers as guest pointers.
- Callback access remains the mandatory correctness fallback when fastmem is absent or faults.

D-0004 selects high-base contiguous fastmem as the preferred first Android acceleration path based on real Android/AArch64 evidence; direct low-VA pointer identity remains optional and outside the generic contract.

## Build/test entry points

Host tests:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBA32ANDROID_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Android `arm64-v8a` is cross-built in GitHub Actions with NDK `27.3.13750724`. CI also publishes `android_runtime_smoke` bundled with `liba32android.so` and a Termux launcher for real-device A32 execution validation.

## Evidence labels

Use PROVEN, IMPLEMENTED, TESTED, PARTIAL, HYPOTHESIS, NOT IMPLEMENTED and BLOCKED. Never turn an unexecuted test into PASS.
