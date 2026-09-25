# Proposal — ELF32 persistent link map and global group

## Intent

Close the next linker-lifetime gap after feature 015 without collapsing Android
filesystem or namespace policy into the generic ELF core.

The new boundary is a caller-owned persistent ELF32 link map: multiple root
loads can reuse already-loaded identities/mappings, retain stable object
indexes, and expose an ordered global-scope list for the existing
requester/global symbol-lookup policy.

## Evidence motivating the slice

The supplied VLC ARMv7 DSOs depend on APK-local libraries plus Android platform
libraries such as `libc.so`, `libdl.so`, `libm.so`, `liblog.so`,
`libandroid.so`, `libEGL.so`, and `libGLESv2.so`. The current one-shot
dependency loader can load one recursive graph, but it has no process-lifetime
object registry across independent root loads.

Modern bionic constructs relocation lookup from a global group plus a local
dependency group. Its global group contains the main executable, LD_PRELOAD
libraries, and objects carrying `DF_1_GLOBAL`; the main executable and
preloads are promoted into that group by loader policy. Feature 015 already
implements the consumer side of that ordering when given an in-graph global
scope. Feature 016 supplies the missing persistent ownership/global-membership
producer boundary.

Primary Android source evidence and target-binary observations are recorded in
`docs/research/evidence/elf32-link-map-global-group-2026-09-25.md`.

## Boundary

- retain `DT_FLAGS_1` and expose `DF_1_GLOBAL` membership;
- add a persistent caller-owned link map containing one accumulated
  `Elf32DependencyGraph`, root identities, and ordered global object indexes;
- append roots transactionally while reusing pre-existing identities;
- allow caller policy to mark a root global; automatically include newly loaded
  `DF_1_GLOBAL` objects;
- preserve stable object indexes so existing relocation/symbol APIs can consume
  the accumulated graph directly;
- keep the existing one-shot graph loader as a compatibility wrapper.

This feature does not choose filesystem paths, implement Android namespace
accessibility/links, parse LD_PRELOAD, model RTLD_GLOBAL/RTLD_LOCAL, unload
objects, implement dlopen/dlsym, or provide libc/JNI/graphics/audio shims.
