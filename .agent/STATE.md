# Current State

Last updated: 2026-09-15
Current milestone: M0 Architecture / Bootstrap
Current branch: m0-bootstrap

## Working

- Repository initialized.
- Dynarmic CPU strategy researched and selected.
- M0 CPU adapter, ARM smoke, Thumb smoke, CMake and GitHub Actions are IMPLEMENTED in this branch.

## Partially working

- Android arm64 build configuration is IMPLEMENTED but not yet validated by this repository's CI.

## Broken / failing

- None known before the first CI run.

## Test status

- `guest_arm_return_42`: NOT RUN
- `guest_thumb_return_42`: NOT RUN
- Android arm64-v8a cross-build: NOT RUN

## Current blocker

- First GitHub Actions run has not completed yet.

## Active work

- M0 bootstrap and first CI validation.

## Important temporary facts

- Dynarmic pinned commit: e77b1ba0b7da7cbe93021b01a663acfe7c4dd516.
- Project license is not selected yet; dependency-license notes are documented separately.
