# Design — dependency-first ELF32 INIT_ARRAY planning

## Traversal state

The planner allocates one byte of transient visit state per graph object:
unseen, visiting, or complete. A root-scoped DFS marks an object visiting
before following its stored dependency edges. Re-entering a visiting object
terminates that cycle edge; reaching a complete object suppresses a shared
dependency duplicate.

Each first visit consumes the caller's unique-object budget. Dependency edges
are checked before recursion. After all dependencies complete, the current
object contributes its INIT_ARRAY entries.

## Entry accounting

Feature 017 remains the only raw decoder. The planner passes the remaining
total entry budget to each array decode, so sentinel entries consume the same
resource ceiling even though they do not become calls. Decoder failure maps to
a planning failure with the defining object index and nested decoder error.

For each decoded value other than 0 or 0xffffffff, the plan records:

- defining object index;
- zero-based INIT_ARRAY entry index;
- raw logical 32-bit function value.

No ARM/Thumb normalization occurs; bit 0 is preserved for the future execution
layer.

## Failure semantics

Invalid options/root/edge, object-ceiling exhaustion, entry-ceiling exhaustion,
range/read failure, or malformed descriptor input returns no successful partial
call list. The graph and guest memory are never mutated.

## Non-goals

No legacy DT_INIT, PREINIT_ARRAY, FINI_ARRAY/destructor planning, persisted
constructor-called flag, CPU invocation, stack/sentinel setup, dlopen/unload,
or process argument/environment ABI.
