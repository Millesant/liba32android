# ELF32 spec delta — feature 019

The generic A32 CPU request may carry an optional normalized stop PC. Execution
terminates before fetching that PC and reports stop arrival while retaining the
existing finite instruction ceiling and exception/memory-fault reporting.

A lifecycle executor consumes feature-018 INIT_ARRAY calls in order. It derives
ARM/Thumb state from function bit 0, uses a caller-owned 8-byte-aligned stack
top, sets an interworking LR to a caller-selected stop PC, and bounds each call
independently. Completed constructor side effects persist if a later call
fails; no scratch mappings or persistent constructor-called state are owned by
the executor.

Legacy INIT/PREINIT, destructor execution, dlopen/unload lifecycle, process
arguments/environment, and Android provider policy remain deferred.
