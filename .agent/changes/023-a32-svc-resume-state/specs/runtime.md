# Runtime spec delta — feature 023

The engine-independent A32 execution result may report an exact SVC immediate
in addition to the existing generic exception flag. SVC therefore remains
backward-compatible with callers that treat it as an exception while becoming
distinguishable for future service dispatch.

Execution requests may optionally supply a full initial CPSR snapshot. If
absent, the adapter retains the existing Arm/Thumb user-mode initialization.
Returned registers, logical PC, and CPSR after an SVC are valid input to a
follow-up bounded execution request, allowing execution to resume after the
trap without exposing Dynarmic types.

Host service dispatch, ABI marshalling, compatibility shims, syscall emulation,
and Android API behavior remain outside this feature.
