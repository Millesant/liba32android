# Historical Feature Specifications

This tree preserves the numbered requirements/design/tasks packages used by the pre-v7 project workflow. They remain useful historical design records and validation context, but they are no longer the canonical location for accepted current project truth.

## Current workflow

- Accepted current contracts live under `.agent/specs/`.
- Substantial work is tracked under `.agent/changes/<change-id>/`.
- `.agent/STATE.md` records observed reality and validation evidence.
- `.agent/NEXT.md` records dependency-ordered next work.
- `.agent/DECISIONS.md` records durable project rationale.

Do not create new active feature packages in this directory. When historical details are needed, read the smallest relevant numbered package and reconcile it against current specs/source/tests before relying on it.

`000-current-baseline/` through later numbered packages are intentionally retained without retroactive rewriting; Git history remains the authoritative historical record.


## Historical status-line warning

Status lines inside numbered packages are snapshots from the moment those
packages were active. They are deliberately not rewritten when later work
completes or supersedes them.

Do **not** interpret an embedded `ACTIVE`, `NOT RUN`, or "does not yet"
statement in this tree as current project state without reconciling it against
`.agent/STATE.md`, `.agent/specs/`, completed `.agent/changes/`, current
source/tests, and exact-head evidence.

Concrete example: `010-elf32-gnu-relro/` still preserves its historical
"ACTIVE / exact-head validation NOT RUN" text, while current architecture/state
record feature 010 GNU RELRO as complete and verified. The historical text is
retained as provenance, not as an open implementation task.
