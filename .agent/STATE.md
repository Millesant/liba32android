# Current State

Last updated: 2026-09-26
Integration branch: `bleeding`
Control-plane round: `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`
Active change: `028-a32-android-platform-provider` — IMPLEMENTED, exact-head validation NOT RUN.

## Phase

Features 011-027 are accepted. The stack joins real ARM32
load/link/relocation/execution, requester-aware provider composition, resumable
SVC dispatch, the bounded Android log-write service, and a generated
`liblog.so` guest shim.

Feature 028 now adds the first explicit Android platform-library provider policy
seam. Source/tests/docs are prepared, but the change is not accepted until its
exact-head validation matrix passes.

## Implemented runtime

Accepted through feature 027:

- bounded ARM/Thumb execution, resumable SVC state, bounded host-service
  dispatch, and exact-SVC registry composition;
- bounded AAPCS32 `__android_log_write` service;
- reproducible ARMv7 `liblog.so` write shim plus exact catalog identity;
- real consumer -> dependency -> JUMP_SLOT -> shim -> service -> return
  execution evidence;
- logical guest-memory, ELF32 load/placement/metadata/dependency/linker,
  relocation, RELRO, lifecycle, and symbol/version seams described in current
  specs.

Current unverified feature-028 implementation:

- `A32AndroidPlatformProvider` recognizes only exact `liblog.so`;
- a caller-owned access policy receives requester identity/request name
  byte-for-byte and returns Allow, NotFound, or Failed;
- Allow reuses exact catalog image/ceiling validation;
- unknown platform names bypass policy and remain NotFound;
- context-free lookup presents an empty requester;
- provider owns its entry but borrows policy and image, and is
  non-copyable/non-movable because its catalog span is self-referential;
- the real log-shim integration now uses application-catalog first, platform
  provider second, and asserts requester `android-log-consumer` reaches the
  policy unchanged.

## Deferred / partial

Still outside accepted implementation:

- Android namespace names/linked namespaces/accessibility rules, public library
  allowlists, permitted paths, filesystem/APK search, RUNPATH/RPATH,
  LD_LIBRARY_PATH, preload/RTLD behavior, and automatic platform-provider
  installation;
- `__android_log_print`/`__android_log_vprint` varargs and broader
  libc/JNI/graphics/audio compatibility;
- stable public embedding API/error contract;
- lazy binding, broader relocation families/formats, TLS/IFUNC;
- destructor/unload/dlopen/dlsym lifecycle;
- Android-device end-to-end evidence and general application compatibility.

## Validation and repository-truth evidence

Latest accepted behavior-changing result: feature 027 at
`37624f389bb34197669a763c32631a1174d099a6`:

- ARM32 liblog shim integration `108389674586` — PASS.
- Linux A32 smoke `108389675056` — PASS.
- Android x86_64 probe `108389675036` — PASS.
- Android arm64-v8a cross-build `108389674910` — PASS.

Feature 028 exact-head validation: NOT RUN.

## Current blockers / external evidence gaps

- Android native tombstone/backtrace coexistence: BLOCKED on an accessible device environment.
- AArch64 16 KiB Android runtime execution: NOT RUN.
- Project license selection: BLOCKED on maintainer choice.
