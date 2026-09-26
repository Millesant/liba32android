# Project cleanup v9

## Intent

Audit current repository truth after features 011-024 and remove stale
documentation/state that can make completed work look incomplete or make
historical snapshots look current.

This is a behavior-preserving maintenance pass. The focus is truthfulness:
canonical state, current architecture docs, diagnostics wording, historical-spec
orientation, and build/test registration evidence must agree with the current
tree.

## Audit findings that drive the cleanup

- Feature 024 was implemented and exact-head green but canonical state still
  called it ACTIVE/pending; that feature is now closed before this change.
- `.agent/STATE.md` still carried an older control-plane revision and an M4
  summary ending before features 019-024.
- `.agent/NEXT.md` still called INIT_ARRAY/FINI_ARRAY lifecycle metadata a
  remaining requirement despite features 017-019 being complete.
- several current architecture docs still used phrases such as "future linker"
  or "future graph/loader" for downstream layers that now exist.
- root `specs/010-elf32-gnu-relro/` intentionally preserves a historical
  ACTIVE/NOT-RUN snapshot even though feature 010 is complete; the historical
  index needs to make that trap explicit rather than rewriting history.
- diagnostics say the public runtime API is absent; that remains true for a
  stable embedding API, but the wording now needs to distinguish it from the
  implemented internal `src/runtime/` orchestration layer.
- code-search audit found no TODO, FIXME, placeholder, GTEST_SKIP, or
  DISABLED_ markers.
- a complete repository-tree/CMake audit at
  `f45da9d530524a2701b053e84c36f5860e607582` found 17 source .cpp files and
  35 test .cpp files, with zero unreferenced source/test .cpp files.

## Non-goals

- no runtime/linker semantic changes;
- no deletion or retroactive rewriting of historical root numbered specs;
- no renaming of test targets/CTest names;
- no new feature work or speculative compatibility infrastructure.
