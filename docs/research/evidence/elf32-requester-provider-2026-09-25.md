# ELF32 requester-provider evidence — 2026-09-25

## Supplied target observations

The supplied VLC ARMv7 native set mixes APK-local dependencies
(`libvlc.so`, `libc++_shared.so`) with Android platform names including
`libc.so`, `libdl.so`, `libm.so`, `liblog.so`, `libandroid.so`,
`libEGL.so`, and `libGLESv2.so`. The supplied ARM32 FMOD library likewise
depends on Android platform libraries.

A fresh static readelf check of the four supplied VLC ARMv7 DSOs and
`libfmod.so` found no DT_RPATH or DT_RUNPATH entries. That means the supplied
sample does not justify implementing path-list semantics yet, while
application/platform provider selection remains concrete.

The third-party binaries remain external inputs and are not committed.

## Existing seam gap

`Elf32DependencyProvider::resolve` currently receives only the exact
DT_NEEDED request name and an image-byte ceiling. Recursive loading knows the
identity of the graph object making that request, but the resolver does not
forward it. Consequently an external Android namespace provider cannot make
requester-sensitive accessibility decisions through the current API.

Feature 020 adds only that missing context. Android namespace/search behavior
itself remains a later platform-layer feature.
