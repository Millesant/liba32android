# A32 host-service dispatch

Status: feature 024 implemented; exact-head verification pending

## Boundary

`src/runtime/a32_service_dispatch.*` is a game-agnostic orchestration layer
above the engine-independent CPU and GuestMemory contracts. It knows neither
Dynarmic nor Android APIs.

The caller owns an `A32HostServiceHandler`. For each SVC trap the dispatcher
passes:

- `memory::GuestMemory&`;
- the exact SVC immediate reported by feature 023;
- mutable A32 general registers, including the logical guest PC;
- mutable CPSR.

The handler returns `Handled`, `Unhandled`, or `Failed`. No service-number
registry or ABI interpretation exists in this layer.

## Bounded loop

`execute_a32_with_services` takes an initial
`cpu::ExecutionRequest`. Its `instruction_count` is one total budget shared
across every CPU resumption. A separate `max_service_calls` bounds successful
host-service invocations.

Each CPU slice runs with the remaining instruction budget. After a handled SVC,
the next slice is reconstructed from the returned/mutated registers, logical
PC, and CPSR. The original stop-PC target is preserved. This means ARM and
Thumb service stubs can trap to the host and then continue through ordinary
guest instructions such as `bx lr`.

A handled service on the final permitted guest instruction is still delivered
to the handler. With no stop target, that is a successful fixed-budget
completion. With a requested stop target that has not been reached, the result
is instruction-limit exhaustion.

## Failure semantics

The runtime result distinguishes:

- guest memory fault;
- ordinary CPU exception without SVC;
- handled-service ceiling exhaustion;
- unhandled service;
- failed service;
- total instruction budget exhausted before a requested stop PC.

When a service-specific error occurs, the failing SVC immediate is retained.
Final registers/CPSR, total guest instructions, successful service count, and
stop state are always returned.

The dispatcher is not a transaction. Register or guest-memory changes from
completed handlers remain visible if a later service or CPU step fails. It
never maps, protects, unmaps, or broadens guest permissions.

## Validation scope

Synthetic runtime coverage exercises ARM and Thumb service programs that trap,
let the handler mutate return registers/guest memory, resume in the correct
instruction set, and return to an unmapped logical stop PC. Additional cases
cover service ceilings, unhandled/failed handlers, completed-side-effect
preservation, memory faults, non-SVC CPU exceptions, requested-stop instruction
exhaustion, and no-stop fixed-budget compatibility.

No Android service implementation, AAPCS stack marshalling, shim ELF, syscall
ABI, or namespace/provider policy is claimed by this feature.
