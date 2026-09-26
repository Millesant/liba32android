# ELF32 provider-chain evidence — 2026-09-26

The supplied VLC ARMv7 set mixes application-local dependencies such as
`libvlc.so` and `libc++_shared.so` with platform names such as `libc.so`,
`libdl.so`, `libm.so`, `liblog.so`, `libandroid.so`, `libEGL.so`, and
`libGLESv2.so`. The supplied ARM32 FMOD library likewise needs platform
libraries.

Feature 020 supplies requester-aware acquisition but still exposes only one
provider object to the generic resolver. A finite ordered provider chain is the
smallest generic mechanism that lets a future Android layer place an
application-local provider before a platform provider while keeping all
filesystem/archive/namespace policy outside the ELF core.

The third-party binaries remain external evidence and are not committed.
