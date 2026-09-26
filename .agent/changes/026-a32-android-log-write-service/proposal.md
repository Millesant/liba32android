# Proposal — bounded A32 Android log-write service

## Intent

Implement the smallest concrete Android platform service justified by the
supplied ARM32 FMOD/VLC evidence: `__android_log_write`.

Keep platform behavior out of `src/runtime/`. The new `src/compat/` layer
consumes the already-verified resumable SVC dispatcher and exact-SVC registry
rather than introducing a second execution path.

## Authoritative call shape

AOSP declares:

`int __android_log_write(int prio, const char* tag, const char* text);`

AAPCS32 places these three 32-bit arguments in r0-r2 and returns the 32-bit
function result in r0. Feature 026 therefore needs no stack or variadic
marshalling.

## Service boundary

The service is configured with:

- one exact caller-selected SVC immediate;
- a caller-owned sink;
- independent tag/text maximum payload lengths.

The sink, not the compatibility service, owns host logging/filter behavior and
chooses the signed 32-bit return value exposed to the guest.

## Guest strings

All guest pointers remain logical 32-bit addresses. Non-null strings are copied
only through `GuestMemory`, byte-preserving, and must terminate within the
configured payload ceiling. The terminating NUL is not part of the payload
limit.

A null tag is preserved as null because Android may apply default-tag policy.
A null text pointer, unreadable byte, logical-address overflow, or missing NUL
within the ceiling fails before the sink is invoked.

## Validation

Use an ARM `svc #0xa0; bx lr` test stub with seeded r0-r2 arguments, route it
through feature 025 and feature 024, and verify exact string/return behavior plus
bounded failure cases.

## Non-goals

No guest ELF `liblog.so`, symbol export/provider selection, direct host
`liblog` call, Android namespace/search policy, print/vprint varargs, JNI,
graphics, audio, or general libc behavior.
