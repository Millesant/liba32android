# Proposal — resumable A32 SVC state

## Intent

Expose the smallest engine-independent trap state needed for future
guest-to-host compatibility services.

Dynarmic already reports A32 SVC instructions through its callback, but the
adapter currently collapses that event into the same boolean used for ordinary
exceptions. Future ARM32 shim stubs need to identify the SVC immediate, inspect
guest registers, let a host layer perform a service, and resume at the next
guest instruction.

## Scope

- report the exact SVC immediate in `ExecutionResult`;
- keep `exception_raised` true for SVC to preserve existing behavior;
- allow `ExecutionRequest` to seed an exact CPSR snapshot;
- prove ARM and Thumb trap state can be resumed from returned registers/PC/CPSR.

No host-service dispatcher or ABI bridge is added yet.

## Motivation from supplied binaries

A fresh static scan of the supplied VLC ARMv7 and FMOD images shows external
platform dependencies including `libc.so`, `libdl.so`, `libm.so`,
`liblog.so`, `libandroid.so`, `libEGL.so`, `libGLESv2.so`, and
`libstdc++.so`. Their imports include pthread/libc/libdl, Android logging,
EGL/GLES, and C++ ABI entry points. Features 020-022 can now supply exact-name
shim images, but there is no generic guest-to-host call trap to back those
shims.

## Non-goals

No service-number registry, host callback API, AAPCS marshalling, guest stack
argument decoding, libc/log/EGL implementation, Linux syscall ABI, or Android
namespace policy.
