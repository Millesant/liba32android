# ARM32 Android symbol-versioning evidence — 2026-09-25

## Project-driving artifact evidence

The supplied VLC Android ARMv7 APK was inspected without adding its third-party binaries to the repository. Its native DSOs declare DT_VERSYM plus VERNEED/VERDEF metadata; Android platform imports use a version named `LIBC`. The inspected relocation families remain within the project's existing RELATIVE/ABS32/GLOB_DAT/JUMP_SLOT support, making symbol versioning the first explicit linker-semantic blocker.

## Android/bionic behavior used by feature 014

Android bionic's VersionTracker resolves requester VERSYM indices through VERNEED and VERDEF records. VERNEED `vn_file` names are validated against direct child SONAMEs. Provider lookup skips hidden definitions for an unversioned request, while explicit requests match VERDEF hash/name and otherwise use global version index 1.

Primary source:
- Android bionic linker source, VersionTracker / versioned lookup: https://android.googlesource.com/platform/bionic/+/master/linker/linker.cpp

Feature 014 intentionally adopted only these version-table matching semantics. At that feature boundary, namespace/global-group and DT_SYMBOLIC ordering were separate accepted gaps. Completed feature 015 implements DT_SYMBOLIC/DF_SYMBOLIC requester-first ordering. Completed feature 016 supplies persistent cross-root lifetime and generic global-group membership; Android namespace/search-path/preload/RTLD policy remains separate work.
