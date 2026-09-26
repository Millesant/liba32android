# Design — bounded constructor invocation

## CPU stop-PC seam

`ExecutionRequest` gains an optional normalized `stop_pc`.
`execute` checks the JIT PC before the first instruction and after every
instruction. If it equals `stop_pc`, execution terminates before fetching the
target and `ExecutionResult::stop_pc_reached` is true.

When no stop PC is supplied, existing fixed-instruction behavior is unchanged.
Exception and memory-fault termination remain higher priority failure signals.

## Constructor executor

The lifecycle executor receives a span of `Elf32InitCall` and immutable
options. The stack top must be 8-byte aligned, the shared return PC must be word
aligned so it is valid for both ARM and Thumb constructors, and the per-call
instruction ceiling must be nonzero.

For each call:

1. reject zero/all-ones or instruction-set-invalid function values even though
   the feature-018 planner normally filters sentinels;
2. derive Thumb from function bit 0 and normalize entry PC;
3. zero deterministic register state, set SP to the caller stack top and LR to
   return PC with the constructor's interworking bit;
4. execute under the per-call ceiling with `stop_pc = return_pc`;
5. fail with call/object provenance on memory fault, exception, or missing
   return before the ceiling. If the CPU backend reports both a memory fault
   and a generic exception for one step, the executor preserves the more
   specific memory-fault classification.

The same caller stack top is restored for each constructor call. Guest memory
writes performed by completed constructors remain visible to later calls.

## Failure semantics

No later constructor is attempted after a failure. `calls_completed` counts
only calls that reached the return stop. The executor itself never maps,
protects, unmaps, or rolls back guest memory.
