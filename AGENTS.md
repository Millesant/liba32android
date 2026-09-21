# Repository Agent Instructions

These instructions apply to the whole repository unless a deeper path adds a more specific instruction file.

## Bootstrap substantial work

For implementation, debugging, testing, CI, reverse engineering, repository modification, or continuation work:

1. Read `.agent/STATE.md` and `.agent/NEXT.md`.
2. Read only the relevant parts of `.agent/CONTEXT.md` and `.agent/DECISIONS.md`.
3. Read the active `specs/<id>-<feature>/` package when one exists.
4. Reconcile those summaries against the current branch, source, tests, CI, and artifacts before acting.
5. Pick one bounded objective and its validation path.

Do not ask the user to choose between local and GitHub execution. Work inline with the capabilities that are actually available, and never claim background workers, sub-agents, commits, pushes, tests, or CI that did not actually happen.

## Work sizing and specs

Use the smallest process that keeps intent checkable.

- Tiny/mechanical work: inspect, change, run the smallest relevant validation, persist material state changes.
- Bounded fixes/features: state expected behavior and acceptance criteria, then implement and validate the focused slice.
- Feature/subsystem/architecture work: use `requirements.md -> design.md -> tasks.md` under `specs/<id>-<feature>/` before substantial implementation.
- Program-scale work: decompose into independently verifiable feature packages instead of one giant spec.

`specs/000-current-baseline/` is the converted specification of the runtime work already completed through PR #11. New feature-scale work should use the same structure. Optional research/contracts/checklists belong in a feature package only when they contain durable information.

## Engineering invariants

- The runtime remains game-agnostic. Application-specific behavior belongs under `profiles/` and must not leak into the generic core.
- Guest virtual addresses are logical 32-bit values. Do not expose host pointers as guest pointers through CPU, ELF, ABI, or runtime interfaces.
- Dynarmic stays behind `src/cpu/`; higher layers depend on engine-independent contracts.
- `memory::GuestMemory` remains the generic memory seam. Fastmem is an optimization; callback-backed access remains the correctness fallback.
- Keep ELF mapping, structural dynamic metadata, and dynamic-linker semantics as separate layers.
- Do not silently broaden ELF permissions or compatibility behavior to make a fixture pass.

## Validation vocabulary

Use these labels precisely:

- `PASS`: the check actually ran and succeeded.
- `FAIL`: the check actually ran and failed.
- `BLOCKED`: required execution cannot proceed because of a concrete external/environment limitation.
- `NOT RUN`: the check was not executed.

Inspection or reasoning alone is never `PASS`.

## Git and persistence

- Treat `bleeding` as the integration branch. Make substantial changes on one scoped branch per real feature/fix/PR and land them through a pull request unless the user explicitly directs otherwise.
- Do not create task-by-task branches or standalone `state/reconcile-*` branches for routine bookkeeping. Close durable state on the feature branch before merge when practical; tiny factual post-merge reconciliation may land directly on `bleeding` when safe and authorized.
- Delete merged source branches promptly when repository tooling supports it; do not keep merged branches as pseudo-state.
- Before ending substantial work, reconcile `.agent/STATE.md` and `.agent/NEXT.md`; update `.agent/DECISIONS.md` only for rationale that future work must preserve.
- Keep `STATE.md` factual and current, `NEXT.md` dependency-ordered and actionable, and Git history responsible for historical detail.
- A fresh session should be able to continue from repository state without relying on chat history.
