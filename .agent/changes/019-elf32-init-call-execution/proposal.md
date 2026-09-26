# Proposal — bounded ELF32 INIT_ARRAY call execution

## Intent

Execute feature-018 constructor call plans through the generic A32 CPU seam
without carrying the integration fixture's executable sentinel loop into
runtime lifecycle code.

The enabling CPU change is an optional exact stop-PC condition. Because the CPU
adapter already executes one instruction at a time under a finite ceiling, it
can stop immediately when guest PC reaches a caller-selected return address,
including after the final permitted instruction, without fetching code from
that address.

## Lifecycle boundary

The lifecycle executor accepts a completed INIT_ARRAY call plan plus:

- one caller-owned 8-byte-aligned guest stack top;
- one word-aligned normalized logical return-stop PC;
- one finite instruction ceiling per constructor.

For each planned function, bit 0 selects Thumb versus ARM and is removed from
entry PC. LR carries the return-stop address with the same interworking bit as
the constructor. Calls execute in plan order. Completed guest side effects are
not rolled back if a later constructor fails.

## Non-goals

No guest stack allocation/mapping, executable sentinel page, DT_INIT,
PREINIT_ARRAY, FINI_ARRAY/destructor execution, persistent constructor-called
state, process argv/envp, dlopen/unload lifecycle, or Android provider policy.
