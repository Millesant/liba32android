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

## Use

This directory stores project-owned specification artifacts, not the generic agent workflow. When the routed control-plane workflow or project requirements call for feature-scale specification, create the smallest package that preserves requirements, design decisions, dependency-ordered tasks, and their validation paths.

Tiny and routine changes do not need a package solely for ceremony. `.agent/NEXT.md` owns project priority, and `.agent/STATE.md` records current observed reality.

## Current baseline

`000-current-baseline/` converts the already implemented M0-M3 runtime work into the current spec structure without changing runtime behavior. It is the architectural baseline for subsequent feature packages, not a replacement for Git history or detailed architecture/research evidence.

The next feature-scale package should use the next free numeric ID and a descriptive feature slug. `.agent/NEXT.md` owns priority and points to the package once it exists.
