# Compatibility contract

Status: Accepted current project contract
Last reconciled: 2026-09-26

## L32-C001 — Platform compatibility is a separate layer

Platform-library behavior belongs under `src/compat/`, above the
game-agnostic runtime service-dispatch/registry contracts. Compatibility code
may use logical guest addresses and `memory::GuestMemory` but may not expose
host pointers as guest pointers or leak Dynarmic types.

## L32-C002 — Bounded A32 `__android_log_write` service bridge

The Android log-write compatibility service models the C function
`int __android_log_write(int prio, const char* tag, const char* text)` through
the base AAPCS32 register convention: guest r0 carries the 32-bit priority, r1
the tag pointer, r2 the text pointer, and the signed 32-bit return value is
written back to r0.

The service is configured with one exact caller-selected SVC immediate and a
caller-owned sink. Other SVC immediates remain `Unhandled`. Guest strings are
copied only through `GuestMemory` under separate caller-provided maximum tag
and text payload lengths. A tag pointer of zero is preserved as a null tag for
the sink. A null text pointer, unreadable string, address overflow, or missing
terminator within its payload ceiling returns `Failed` before sink invocation.

The sink receives byte-preserving bounded string views valid only for the
synchronous call and returns a signed 32-bit result whose bit pattern is copied
to guest r0. The service does not validate Android logging policy itself and
does not call host liblog directly.

## L32-C003 — Deliberate log-service exclusions

Feature 026 does not implement `__android_log_print`,
`__android_log_vprint`, varargs/stack marshalling, Android log filtering,
guest ELF symbol export/provider generation, namespace/search policy, JNI,
graphics, audio, or general libc emulation.

## L32-C004 — ARM32 guest `liblog.so` write shim/provider

The compatibility layer defines one private guest/host SVC protocol for its
first log shim. `LIBA32ANDROID_A32_ANDROID_LOG_WRITE_SHIM_SVC` and
`kA32AndroidLogWriteShimSvcImmediate` both represent immediate `0xA0`.
Guest shim source and host integration code must consume that shared definition
rather than independently choosing service numbers.

The reproducible freestanding ARM32 shim has SONAME `liblog.so`, exports
`__android_log_write` as a function, preserves its incoming AAPCS32
registers, executes exactly the private SVC, and returns with `bx lr`.
A companion real ARM32 consumer may depend on `liblog.so` and resolve that
symbol through ordinary eager `R_ARM_JUMP_SLOT` handling.

`make_a32_android_log_shim_catalog_entry` returns an exact-name
`Elf32DependencyCatalogEntry` using SONAME `liblog.so` and stable opaque
identity `liba32android-compat-liblog`. The returned entry borrows the supplied
image bytes; their backing storage must outlive the provider operation.

The runtime does not embed or own the generated shim DSO. Automatic platform
catalog installation, filesystem/namespace search policy,
`__android_log_print`/`__android_log_vprint`, and general `liblog.so`
compatibility remain separate work.
