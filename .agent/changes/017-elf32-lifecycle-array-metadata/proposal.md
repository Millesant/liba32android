# Proposal — ELF32 init/fini array metadata

## Intent

Add the next lifecycle boundary required by the supplied ARMv7 VLC binaries
without jumping directly to constructor execution.

Feature 017 validates and exposes `DT_INIT_ARRAY` / `DT_FINI_ARRAY`
metadata and adds a bounded read-only decoder for their raw ELF32 function
pointer entries. Execution order across objects and CPU invocation remain
separate work.

## Evidence

The supplied VLC Android 3.7.2 Beta 2 APK contains four ARMv7 DSOs. Three
carry `DT_INIT_ARRAY`; all four carry `DT_FINI_ARRAY`. None of those four
sample DSOs exposes legacy `DT_INIT` or `DT_FINI`.

Modern Android bionic calls dependencies' constructors before the current
object, calls `DT_INIT` before `DT_INIT_ARRAY`, walks `DT_FINI_ARRAY` in
reverse order before `DT_FINI`, and ignores null/all-ones function pointers.
Those execution semantics are evidence for later lifecycle work, not behavior
implemented by this feature.

Primary source:
- Android bionic `linker/linker_soinfo.cpp`:
  https://android.googlesource.com/platform/bionic/+/master/linker/linker_soinfo.cpp

## Boundary

- recognize INIT_ARRAY/INIT_ARRAYSZ and FINI_ARRAY/FINI_ARRAYSZ as singleton
  pairs;
- rebase and validate their guest ranges exactly once;
- require byte sizes divisible by four;
- decode raw 32-bit entries through `GuestMemory` under a caller-selected
  entry ceiling;
- preserve entry order and raw sentinel values.

No function is executed. No constructor-called state, dependency lifecycle
order, preinit semantics, process argv/envp model, unload, or legacy
INIT/FINI function handling is introduced.
