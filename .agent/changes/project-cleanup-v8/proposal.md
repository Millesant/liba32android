# Project cleanup v8

## Intent

Perform a repository-wide maintenance pass without changing accepted runtime behavior. The goal is to make ownership visible from the tree, reduce monolithic build/test registration, separate development tooling by purpose, and make current documentation easier to navigate.

## Child changes

1. `project-source-test-layout-v8`
   - group ELF implementation files by runtime layer;
   - move the private byte helper under an internal directory;
   - group tests by subsystem and synthetic vs real-fixture evidence;
   - split CMake test registration into CPU, memory, and ELF modules.

2. `project-tooling-layout-v8`
   - separate Android diagnostics from fixture builders;
   - update CMake, GitHub Actions, and current docs to the new tool paths;
   - keep generated fixture semantics and diagnostic target names unchanged.

3. `project-documentation-v8`
   - replace the oversized top-level README with current project navigation;
   - add docs indexes plus repository-layout and build/test guidance;
   - add focused README files for ELF source, tests, and tools;
   - compact project context/state so Git history remains the historical record.

## Non-goals

- no new ELF/runtime compatibility behavior;
- no public ABI expansion;
- no C++ standard/toolchain change;
- no test target or CTest renaming;
- no rewriting historical root `specs/` packages or old evidence records merely to reflect present-day paths.
