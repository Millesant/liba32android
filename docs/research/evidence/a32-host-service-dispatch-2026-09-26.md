# A32 host-service dispatch evidence — 2026-09-26

Feature 023 established that ARM and Thumb SVC traps expose exact immediates and
can resume from returned registers/PC/CPSR. Features 020-022 established
requester-aware dependency-provider composition and exact-name image catalogs.

The supplied VLC/FMOD ARM32 inputs still require platform services such as
Android logging, pthread/libc/libdl routines, EGL/GLES, and C++ runtime
functions. Before implementing any of those APIs, the runtime needs one
bounded game-agnostic mechanism that lets a thin guest stub trap to a host
handler and continue guest execution.

Feature 024 supplies only that mechanism. Platform service meaning and ABI
marshalling remain later work.
