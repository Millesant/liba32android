# Current State

Last updated: 2026-09-26
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`
Latest maintenance result: `project-cleanup-v9@567ab4931d3d0af69edb7680b0a266be7672df64`
Active change: `027-a32-liblog-write-shim-provider` — IMPLEMENTED, exact-head validation NOT RUN.

## Phase

M4 runtime/linker work is accepted through features 011-026. That stack includes
real ARM32 load/link/relocation/execution, requester-aware exact-name provider
composition, resumable SVC state, bounded host-service dispatch, exact-SVC
registry routing, and the bounded AAPCS32 `__android_log_write` host service.

Feature `027-a32-liblog-write-shim-provider` now has its source, real ARM32
fixture builder, catalog helper, integration regression, build/CI registration,
spec/docs, and durable change records prepared. It is not accepted until the
exact-head Linux A32 smoke and both required Android checks pass.

## Implemented runtime

Accepted through feature 026:

- bounded ARM/Thumb execution and resumable SVC state;
- bounded game-agnostic host-service dispatch and exact-SVC registry composition;
- Android `__android_log_write` service bridge with bounded GuestMemory strings,
  null-tag preservation, pre-sink failure semantics, caller-owned sink policy,
  and exact signed r0 return bits;
- logical guest-memory backends with optional high-base fastmem;
- validated ELF32 loading/placement/dynamic metadata/strings;
- exact requester/provider dependency acquisition and persistent graph/link-map
  machinery;
- bounded symbol/version lookup and accepted ARM REL/JUMP_SLOT relocation set;
- combined relocation transaction plus GNU RELRO;
- real fixture load/link/relocate/execute evidence.

Current unverified feature-027 implementation:

- shared private shim ABI constant `0xA0`;
- reproducible freestanding ARMv7 `liblog.so` source exporting only
  `__android_log_write` as `svc #0xa0; bx lr`;
- borrowed exact-name catalog helper for `liblog.so` image bytes;
- real ARM32 consumer -> `DT_NEEDED liblog.so` -> JUMP_SLOT -> shim -> SVC ->
  feature-026 service -> return execution regression.

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

Latest accepted behavior-changing result: feature 026 at
`59fa3eba1d53c6679ef6086cd209198ca7ecb4ac`:

- Linux A32 smoke check `108387154388` — PASS.
- Android x86_64 address-space probe check `108387154358` — PASS.
- Android arm64-v8a cross-build check `108387154270` — PASS.

Feature 027 exact-head validation: NOT RUN. Prepared fixture/test code is not
passing evidence until the implementation revision completes the required
matrix.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
