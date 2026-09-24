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
