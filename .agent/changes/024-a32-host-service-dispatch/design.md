# Design — A32 host-service dispatcher

## Layering

Introduce `src/runtime/` above CPU and memory. The layer depends only on the
engine-independent `cpu::ExecutionRequest/Result` contract and
`memory::GuestMemory`; it never includes Dynarmic.

`A32HostServiceHandler` is caller-owned. Its virtual `handle` method can
mutate registers/CPSR and guest memory and returns one of:

- `Handled`: resume guest execution;
- `Unhandled`: stop with explicit service-unhandled error;
- `Failed`: stop with explicit handler-failed error.

## Dispatch loop

`execute_a32_with_services` takes the initial CPU request by value plus a
finite `max_service_calls`.

It tracks one remaining instruction budget initialized from the request.
For each CPU slice:

1. call `cpu::execute` with the remaining total budget;
2. subtract exactly the instructions actually executed;
3. memory fault -> fail;
4. SVC -> check service ceiling, call handler, and on Handled reconstruct the
   next request from returned/mutated registers, logical PC, and CPSR while
   retaining the original stop PC;
5. generic exception without SVC -> fail;
6. reached stop PC -> succeed;
7. no SVC/exception/stop means the current remaining budget completed; succeed
   when no stop target exists, otherwise report instruction-limit exhaustion.

A handled SVC that consumes the final allowed guest instruction still invokes
the host handler. With no stop target that is a successful bounded completion;
with a stop target it cannot claim return completion and reports instruction
exhaustion.

## Result

The runtime result owns:

- error;
- final registers and CPSR;
- total guest instructions executed;
- successfully handled service count;
- stop-PC reached flag;
- optional failing SVC immediate.

It intentionally does not expose Dynarmic diagnostics.

## Non-goals

No service registry, ABI decoding, platform API behavior, guest mapping
allocation, rollback, or implicit permission changes.
