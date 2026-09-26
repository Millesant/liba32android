# ELF32 dependency catalog evidence — 2026-09-26

The supplied VLC ARMv7 set mixes application-local dependencies
(`libvlc.so`, `libc++_shared.so`) with platform names such as `libc.so`,
`libdl.so`, `libm.so`, `liblog.so`, `libandroid.so`, `libEGL.so`, and
`libGLESv2.so`. The supplied ARM32 FMOD image similarly needs Android
platform libraries.

Features 020 and 021 now provide requester-aware lookup plus ordered provider
composition, but the project still lacks a concrete provider implementation
that can expose caller-supplied library images. A finite exact-name catalog is
the smallest implementation that can model extracted application libraries and
a separate caller-maintained platform set without prematurely defining Android
filesystem/archive/namespace policy.

The third-party binaries remain external evidence and are not committed.
