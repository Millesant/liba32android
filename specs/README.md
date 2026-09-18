# Feature Specifications

This directory holds the durable requirements/design/tasks chain for feature-scale and larger work.

## Layout

Use the smallest useful package:

```text
specs/<id>-<feature>/
├── requirements.md
├── design.md
├── tasks.md
├── research.md       # optional
├── contracts/        # optional
└── checklists/       # optional
```

Do not create optional files unless they preserve information that would otherwise need to be rediscovered.

## Lifecycle

1. Define requirements and acceptance criteria.
2. Resolve only the research needed to remove material uncertainty.
3. Record the chosen design and invariants.
4. Break implementation into dependency-ordered tasks with validation.
5. Check that every acceptance criterion has an implementation/validation path.
6. Implement and validate coherent slices.
7. Converge code/docs/state against the spec, then keep `.agent/STATE.md` and `.agent/NEXT.md` current.

Tiny and routine changes do not need a full package.

## Current baseline

`000-current-baseline/` converts the already implemented M0-M3 runtime work into the current spec structure without changing runtime behavior. It is the architectural baseline for subsequent feature packages, not a replacement for Git history or detailed architecture/research evidence.

The next feature-scale package should use the next free numeric ID and a descriptive feature slug. `.agent/NEXT.md` owns priority and points to the package once it exists.
