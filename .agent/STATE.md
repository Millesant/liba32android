# Current State

Last updated: 2026-09-26
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`
Latest maintenance result: `project-cleanup-v9@567ab4931d3d0af69edb7680b0a266be7672df64`

## Phase

M4 runtime/linker work is accepted through features 011-027. The stack now
joins real ARM32 load/link/relocation/execution, requester-aware exact-name
provider composition, resumable SVC state, bounded host-service dispatch,
exact-SVC registry routing, the bounded AAPCS32 `__android_log_write` service,
and a real generated ARM32 `liblog.so` write shim/provider path.

Features `011-elf32-combined-relocation-transaction` through
`027-a32-liblog-write-shim-provider` are DONE.

## Implemented runtime

- bounded ARM/Thumb execution and resumable SVC state;
- bounded game-agnostic host-service dispatch and exact-SVC registry composition;
- Android `__android_log_write` service bridge with bounded GuestMemory strings,
  null-tag preservation, pre-sink failure semantics, caller-owned sink policy,
  and exact signed r0 return bits;
- shared private shim SVC ABI `0xA0`;
- reproducible freestanding ARMv7 `liblog.so` source exporting
  `__android_log_write` as `svc #0xa0; bx lr`;
- borrowed exact-name catalog helper for the generated `liblog.so` image;
- real consumer -> `DT_NEEDED liblog.so` -> eager JUMP_SLOT -> shim -> SVC ->
  feature-026 service -> requested-stop execution evidence;
- logical guest-memory backends with optional high-base fastmem;
- validated ELF32 loading/placement/dynamic metadata/strings;
- exact requester/provider dependency acquisition and persistent graph/link-map
  machinery;
- bounded symbol/version lookup and accepted ARM REL/JUMP_SLOT relocation set;
- combined relocation transaction plus GNU RELRO;
- real fixture load/link/relocate/execute evidence.

## Deferred / partial

Still outside accepted implementation:

- automatic platform-library catalog installation plus Android
  filesystem/search-path/namespace/pathname/accessibility policy;
- `__android_log_print`/`__android_log_vprint` varargs/stack marshalling and
  broader libc/JNI/graphics/audio services;
- stable public embedding/runtime API and runtime-wide public error contract;
- lazy binding, broader relocation formats/families, TLS/IFUNC;
- persisted constructor/destructor/unload lifecycle and `dlopen`/`dlsym`;
- Android-device end-to-end runtime evidence;
- general application/game compatibility.

## Validation and repository-truth evidence

Latest behavior-changing result: feature 027 at
`37624f389bb34197669a763c32631a1174d099a6`:

- ARM32 liblog shim integration check `108389674586` — PASS.
- Linux A32 smoke check `108389675056` — PASS.
- Android x86_64 address-space probe check `108389675036` — PASS.
- Android arm64-v8a cross-build check `108389674910` — PASS.

The real shim integration validates deterministic generated DSOs,
`liblog.so` SONAME/DT_NEEDED/export/JUMP_SLOT metadata, exact-name catalog
loading, relocation, service dispatch, AAPCS argument preservation, sink return,
and requested-stop completion. It does not claim automatic namespace/provider
installation or general FMOD/VLC compatibility.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
