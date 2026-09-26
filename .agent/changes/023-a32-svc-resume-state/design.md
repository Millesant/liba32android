# Design — SVC reporting and resumable CPU state

## Execution request

`ExecutionRequest` gains optional `initial_cpsr`.

- absent: preserve the current behavior exactly, deriving ARMv7 user-mode CPSR
  from `instruction_set`;
- present: initialize Dynarmic with that exact caller-supplied snapshot.

This permits a follow-up execution request to reuse `ExecutionResult::cpsr`
without exposing engine types.

## SVC reporting

The Dynarmic callback stores the exact SVC immediate and continues to mark the
existing generic exception flag. `ExecutionResult` gains an optional
`svc_immediate`.

The outer step loop therefore still terminates on SVC exactly as today. The
additional field only classifies the terminal event.

## Resume contract

After a reported SVC, callers can construct a new request from:

- returned general registers;
- `regs[15]` as the next logical entry PC;
- returned CPSR as `initial_cpsr`;
- a fresh finite instruction budget/stop PC.

Focused tests prove both ARM and Thumb instructions continue after the SVC.
The Thumb case deliberately leaves `instruction_set` at its default on resume
so the preserved CPSR T bit is what restores Thumb state.

## Non-goals

No direct callback from inside Dynarmic into compatibility code, no service
dispatch loop, no register mutation API during a trap, no ABI decoding, and no
platform services.
