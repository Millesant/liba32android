# liba32android

An experimental, game-agnostic AArch32 compatibility runtime intended to execute 32-bit ARM Android native code inside an AArch64 Android process.

The project is intentionally layered: CPU execution, guest memory, ELF32 loading/linking, ABI transitions, compatibility libraries and application profiles are separate concerns. Minecraft PE 0.15.x is a future stress target, not the architecture.

## Current milestone: M0

M0 selects and integrates the CPU execution engine and proves the smallest possible A32 vertical slice. Dynarmic is pinned behind `src/cpu/` and the current tests execute one ARM instruction and one Thumb instruction that each place `42` in guest `r0`.

Nothing in the current tree claims ELF loading, Android libc compatibility, JNI, pthread/TLS, graphics, audio, guest/host ABI bridging or application compatibility yet.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBA32ANDROID_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

GitHub Actions also performs an Android `arm64-v8a` cross-build of `liba32android.so`; the smoke tests execute on the Linux x86-64 hosted runner because GitHub-hosted Android/AArch64 execution hardware is not assumed.

See `docs/architecture/cpu-engine.md`, `docs/research/cpu-engine-evaluation.md`, and `.agent/STATE.md` for evidence and current status.
