# Proposal — ELF32 requester/global symbol scope policy

## Intent

Implement the smallest requester-sensitive lookup slice needed by ordinary Android ARM32 DSOs without introducing a process-wide linker or namespace manager.

## Evidence motivating the slice

The supplied ARM32 `libfmod.so` advertises both `DT_SYMBOLIC` and `DF_SYMBOLIC` in `DT_FLAGS`. The supplied VLC APK's ARMv7 `libvlc.so` does the same. Their observed relocation sets are otherwise inside the runtime's implemented REL/JUMP_SLOT subset, so requester/global lookup ordering is a concrete compatibility gap rather than speculative infrastructure.

Modern bionic represents the normal lookup list as global-group entries followed by local-group entries and reserves a requester-first slot when `DT_SYMBOLIC` applies. This change models only that ordering boundary.

## Boundary

- retain `DT_SYMBOLIC` / `DF_SYMBOLIC` as validated linker metadata;
- let callers provide an explicit ordered global-scope list of object indices from the already-loaded dependency graph;
- for ordinary relocation references, search explicit global scope before the requester's graph-local breadth-first closure;
- for symbolic requesters, search the requester first, then explicit global scope, then the remaining local closure;
- deduplicate searched objects and charge all unique candidates to the existing scope ceiling;
- keep plain graph lookup unchanged.

This does not create Android namespaces, preloads, independent graph merging, process-wide link-map lifetime, `dlopen`/`dlsym`, constructors, TLS/IFUNC, or platform-library compatibility shims.
