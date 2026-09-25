# ELF32 persistent link-map/global-group evidence — 2026-09-25

## Target artifact observations

The supplied VLC Android 3.7.2 Beta 2 APK contains four ARMv7 DSOs. Their
`DT_NEEDED` sets cross the APK/platform boundary:

- `libc++_shared.so` -> `libc.so`, `libdl.so`;
- `libmla.so` -> APK-local `libc++_shared.so` and `libvlc.so`, plus
  `libc.so`, `libdl.so`, `liblog.so`, `libm.so`;
- `libvlc.so` -> APK-local `libc++_shared.so`, plus `libEGL.so`,
  `libGLESv2.so`, `libandroid.so`, `libc.so`, `libdl.so`,
  `liblog.so`, `libm.so`;
- `libvlcjni.so` -> APK-local `libvlc.so`, plus `libc.so`,
  `libdl.so`, `liblog.so`, `libm.so`.

The supplied ARM32 `libfmod.so` similarly needs `libc.so`, `libdl.so`,
`liblog.so`, `libm.so`, and `libstdc++.so`.

These observations show that future application loading needs object lifetime
beyond a single isolated dependency graph and a provider layer capable of
mixing application-local and platform-provided identities.

## Android linker behavior used as compatibility evidence

Modern bionic builds each relocation lookup list from a global group plus the
root's local dependency group. The global group consists of the main
executable, LD_PRELOAD libraries, and objects with `DF_1_GLOBAL`. Bionic
forces the main executable and preloads into that membership and keeps the
group ordered in namespace load order.

Primary sources:

- Android bionic `linker/linker.cpp` (master):
  https://android.googlesource.com/platform/bionic/+/master/linker/linker.cpp
- Android bionic `linker/linker_namespaces.cpp`:
  https://android.googlesource.com/platform/bionic/+/master/linker/linker_namespaces.cpp
- Android bionic `linker/linker_main.cpp`:
  https://android.googlesource.com/platform/bionic/+/main/linker/linker_main.cpp

Feature 016 adopts only the ownership/global-membership boundary needed by the
current runtime. Filesystem search, namespace accessibility, linked namespaces,
RTLD flags, preload environment parsing, and unload remain separate work.

## Existing project seam

Feature 015 already accepts an ordered caller-owned
`global_scope_objects` span whose indices belong to one
`Elf32DependencyGraph`. The existing dependency loader already provides
transactional mapping, identity-based deduplication, stable per-call indexes,
and rollback. The missing step is to preserve those invariants across
successive root loads instead of reconstructing them per call.
