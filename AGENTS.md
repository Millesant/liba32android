# Repository Agent Instructions

These instructions are the project-specific overlay for this repository. Generic workflow, runtime capability rules, governance, and reusable engineering process live in the private control plane `Millesant/.gpt`; do not vendor or copy that control-plane material into this repository.

## Bootstrap substantial work

For implementation, debugging, testing, CI, reverse engineering, repository modification, research, or continuation work:

1. Load `BOOTSTRAP.md` from `Millesant/.gpt` on its current `bleeding` control-plane revision and follow only the routed modules required for the task.
2. Recover project-local state from `.agent/STATE.md` and `.agent/NEXT.md`; read only the relevant parts of `.agent/CONTEXT.md`, `.agent/DECISIONS.md`, and the active `specs/<id>-<feature>/` package when one exists.
3. Reconcile those summaries against the current repository branch, source, tests, CI, and artifacts before acting.
4. Keep the active objective bounded and make its validation path explicit.

This repository stores project facts, project policy, specs, decisions, and durable state. Do not persist transient host/tool capability claims as project policy.

## Engineering invariants

- The runtime remains game-agnostic. Application-specific behavior belongs under `profiles/` and must not leak into the generic core.
- Guest virtual addresses are logical 32-bit values. Do not expose host pointers as guest pointers through CPU, ELF, ABI, or runtime interfaces.
- Dynarmic stays behind `src/cpu/`; higher layers depend on engine-independent contracts.
- `memory::GuestMemory` remains the generic memory seam. Fastmem is an optimization; callback-backed access remains the correctness fallback.
- Keep ELF mapping, structural dynamic metadata, and dynamic-linker semantics as separate layers.
- Do not silently broaden ELF permissions or compatibility behavior to make a fixture pass.

## Project state and specifications

- `.agent/CONTEXT.md` is the compact project orientation map.
- `.agent/STATE.md` records current observed project reality and durable validation evidence.
- `.agent/NEXT.md` records dependency-ordered project work, not assumed branch or PR topology.
- `.agent/DECISIONS.md` records project-specific rationale that future work must preserve.
- Feature-scale project specs live under `specs/<id>-<feature>/` as `requirements.md`, `design.md`, and `tasks.md` when the routed workflow calls for that level of structure.
- `specs/000-current-baseline/` remains the converted specification of runtime work completed through PR #11.

Keep these files factual and compact. Git history remains the historical record.

## Git integration

`bleeding` is the project integration branch. This repository does not impose a branch-per-feature, branch-per-task, or PR-per-change rule. Branch, PR, and history decisions follow the current user instruction, repository protection/conventions, and the central control-plane guardrails.

Before finishing substantial project changes, reconcile any materially affected `.agent/` state/spec/docs so a fresh session can resume from repository state without chat history.
