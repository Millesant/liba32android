# Proposal — bounded A32 host-service dispatch

## Intent

Add the smallest game-agnostic runtime loop that can consume feature 023's
resumable SVC traps.

The CPU adapter remains an execution primitive. A higher runtime layer owns the
policy-free loop: execute under a total instruction ceiling, classify SVC,
offer it to a caller-owned handler, apply handler register/CPSR/memory effects,
and resume from the post-SVC state.

## Handler boundary

The handler receives:

- `memory::GuestMemory&`;
- exact SVC immediate;
- mutable A32 general registers including logical PC;
- mutable CPSR.

It returns `Handled`, `Unhandled`, or `Failed`.

The handler may model future ABI/platform services, but feature 024 supplies no
service table and interprets no argument register.

## Bounded execution

The initial `cpu::ExecutionRequest::instruction_count` is the total guest
instruction budget across all resumptions, not a fresh budget per trap.
`max_service_calls` independently limits successful handler invocations.

The original stop-PC target is carried across every resume. If a stop target is
present and the total instruction budget expires first, dispatch fails. If no
stop target is present, ordinary fixed-budget completion remains a success.

## Failure/side-effect semantics

Memory fault, ordinary CPU exception, unhandled service, failed service,
service-ceiling exhaustion, and stop-PC instruction exhaustion are explicit
errors. The result retains final registers/CPSR, total instructions, handled
service count, and failing SVC when relevant.

No rollback is attempted. Guest-memory or register effects from already
completed host services remain visible after a later failure.

## Non-goals

No Android service IDs, service registry, ABI marshalling, stack argument
decoding, shim ELF generation, Linux syscall layer, namespace policy, or
platform behavior.
