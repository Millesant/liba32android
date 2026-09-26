# Compatibility spec delta — feature 026

Introduce the first concrete Android platform-service adapter under
`src/compat/`.

The adapter models
`int __android_log_write(int prio, const char* tag, const char* text)` using
AAPCS32 r0/r1/r2 inputs and a signed 32-bit result returned in r0. It is
configured with one exact caller-selected SVC immediate; other IDs are
`Unhandled`.

Guest tag/text bytes are copied only through `GuestMemory` under independent
caller-selected payload ceilings. A zero tag pointer remains null for the sink.
A zero text pointer, unreadable memory, logical-address overflow, or missing NUL
at the ceiling returns `Failed` before sink invocation. Exact-length payloads
terminated immediately after the configured maximum are valid.

The caller-owned sink receives synchronous byte-preserving views and returns the
guest-visible result bits. The compatibility layer does not call host
`liblog`, choose logging/filter policy, generate/export a guest ELF shim, or
implement print/vprint varargs, namespace/search, JNI, graphics, audio, or
general libc behavior.
