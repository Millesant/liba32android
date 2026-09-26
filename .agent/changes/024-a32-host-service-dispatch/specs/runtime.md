# Runtime spec delta — feature 024

Add a game-agnostic runtime service-dispatch layer above the engine-independent
CPU and GuestMemory seams.

A caller-owned service handler receives an exact SVC immediate plus mutable
guest registers/CPSR and GuestMemory and returns Handled, Unhandled, or Failed.
The dispatcher carries one total finite guest-instruction budget across all
feature-023 resumptions and separately bounds successfully handled service
calls.

Handled traps resume from returned post-SVC PC/register/CPSR state while
preserving the caller's stop-PC target. Memory faults, ordinary exceptions,
unhandled/failed services, service-limit exhaustion, and failure to reach a
requested stop PC before the total instruction budget expires are distinct
results. Completed handler side effects are not rolled back.

No Android API/service registry, ABI marshalling, compatibility shim, syscall
ABI, or namespace/search policy is part of this contract.
