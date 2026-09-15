# liba32android

An experimental, game-agnostic AArch32 compatibility runtime intended to execute 32-bit ARM Android native code inside an AArch64 Android process.

The project is intentionally layered: CPU execution, guest memory, ELF32 loading/linking, ABI transitions, compatibility libraries and application profiles are separate concerns. Minecraft PE 0.15.x is a future stress target, not the architecture.

## Current milestone: M0

M0 selects and integrates the CPU execution engine and proves the smallest possible A32 vertical slice. Dynarmic is pinned behind `src/cpu/`.

Current GitHub Actions evidence:

- **PASS:** ARM guest smoke executes `mov r0, #42` and observes guest `r0 == 42`.
- **PASS:** Thumb guest smoke executes `movs r0, #42` and observes guest `r0 == 42`.
- **PASS:** Android `arm64-v8a` cross-build with the Dynarmic AArch64 backend and NDK `27.3.13750724`.
- **NOT RUN:** executing the smoke test on actual Android/AArch64 hardware. The guest execution tests currently run on the Linux x86-64 GitHub runner; the Android job proves compile/link, not on-device execution.

Validated evidence is recorded in `.agent/STATE.md`.

Nothing in the current tree claims ELF loading, Android libc compatibility, JNI, pthread/TLS, graphics, audio, guest/host ABI bridging or application compatibility yet.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBA32ANDROID_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

GitHub Actions also cross-builds the shared runtime for Android `arm64-v8a`.

See `docs/architecture/cpu-engine.md`, `docs/research/cpu-engine-evaluation.md`, and `.agent/STATE.md` for architecture, research and evidence status.
