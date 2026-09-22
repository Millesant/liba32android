# Next Work

Repository integration remains on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. Active feature work is existing draft PR #32 / `005-elf32-dependency-loading` on `m4-elf32-dependency-loading`.

1. T004 — integrate the pinned real ARM32 fixture through the dependency graph API.
   - Status: READY; T003 is validated by latest containing-head CI #193 at `61c9ee214d1d8a965be9a1f187b76b894b942002`, with Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all PASS.
   - Scope: add fixture-backed graph-loader integration without removing existing explicit-base tests.
   - Required evidence: automatic placement/load through the graph API, required `0x4000` load-bias alignment, zero dependency edges, zero provider calls, and neighboring real-fixture regressions remain green.
   - Exact next action: add the focused real-fixture graph-loader integration test and CMake/CTest wiring, then gate it on exact-head CI.
   - DoD: fixture graph integration and all three CI jobs PASS.

2. T005 — converge architecture/docs/state/spec and run the final exact-head feature gate.
   - Start after T004 PASS.
   - Update README/current phase and dependency-loading architecture documentation.
   - Reconcile requirements/design/tasks/code/tests/state and keep any remaining deviations explicit.
   - DoD: Linux CTest, Android x86_64 probe, and Android arm64-v8a cross-build PASS on the final PR head.

Hardware evidence remains BLOCKED until an accessible Android environment is available. Project license remains BLOCKED on maintainer choice.
