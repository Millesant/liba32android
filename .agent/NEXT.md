# Next Work

Repository state is merged on `bleeding` through PR #32 / runtime commit `17c2aa78535adbd2084c9396f525750e10c0eff8`. `005-elf32-dependency-loading` is DONE. Final PR-head CI #199 PASSed all three jobs with 39/39 CTest, and the squash merge preserved the exact validated source tree.

1. Specify the next M4 feature: bounded ELF32 symbol resolution.
   - Status: NEXT EXECUTABLE FEATURE SELECTED; spec package not yet created.
   - New feature package: `specs/006-elf32-symbol-resolution/`.
   - Existing prerequisites now available: validated/rebased `SYMTAB` + `STRTAB` metadata, bounded linker-string access, and an owned recursive dependency graph with deterministic object/edge order.
   - Required boundary: add bounded symbol-table/hash consumption and graph-level lookup semantics without applying relocations yet.
   - Required specification questions: symbol-table extent/count derivation, SysV/GNU hash handling, `st_name`/binding/type/visibility validation, undefined/absolute/common symbol treatment, weak-vs-global lookup behavior, graph lookup order/scope, resource limits, malformed-table failures, and which versioning semantics remain deferred.
   - Non-goals for this feature: ARM relocation writes, PLT/JMPREL execution, TLS/RELRO, constructors/destructors, Android/bionic pathname/namespace policy, `dlopen`/unload, and guest execution.
   - Exact next action: create readiness-checked `requirements.md -> design.md -> tasks.md` for `006`, then implement its smallest dependency-ordered slice.
   - DoD for spec round: lookup inputs/ownership, bounded table/hash parsing, deterministic scope/order, weak/global/undefined behavior, error/resource model, compatibility boundaries, and validation are explicit with no unresolved blocker.

2. Capture an Android native crash backtrace/tombstone for the opt-in crash test when an accessible device channel is available.
   - Status: BLOCKED on accessible Android device/environment.

3. Resolve the project license when the maintainer is ready to choose one.
   - Status: BLOCKED on maintainer choice.
