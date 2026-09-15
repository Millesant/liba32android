# Project Context

## Mission

Build a reusable, game-agnostic AArch32 compatibility runtime for AArch64 Android. The intended stack is ARM32 Android ELF -> ELF32 loader/linker -> guest runtime -> A32 execution engine -> AArch64 Android host.

## Architectural boundaries

Keep CPU execution, guest address space, ELF32 loading/linking, AAPCS32/AAPCS64 bridging, compatibility libraries, pthread/TLS, signals, JNI, graphics/audio, instrumentation and application profiles separate. Application-specific work belongs under `profiles/` and must never leak into the generic core.

## Current milestone

M0: choose/integrate CPU engine and prove a tiny A32 execution slice. No ELF loader or Android bridges yet.

## CPU dependency

Dynarmic is selected behind `src/cpu/`, pinned to azahar-emu/dynarmic commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`.

## Build/test entry points

Host smoke:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBA32ANDROID_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Android arm64 is cross-built in GitHub Actions with NDK 27.2.12479018.

## Evidence labels

Use PROVEN, IMPLEMENTED, TESTED, PARTIAL, HYPOTHESIS, NOT IMPLEMENTED and BLOCKED. Never turn an unexecuted test into PASS.
