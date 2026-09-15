# Next

1. Validate M0 in GitHub Actions.
   - Run Linux ARM and Thumb smoke tests.
   - Run Android arm64-v8a cross-build.
   - DoD: record exact PASS/FAIL evidence in `STATE.md`.

2. If CI fails, fix only the observed bootstrap/integration failure and rerun the narrow job.
   - Do not begin ELF/JNI/graphics work while M0 is red.

3. After M0 is green, design M1/M2 execution and guest-address-space interfaces.
   - Keep callback memory as test scaffolding only.
   - Separately research low-VA/direct mapping before accepting a fastmem strategy.

4. Decide the project's own open-source license before public release.
