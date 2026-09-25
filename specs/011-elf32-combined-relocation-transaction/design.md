# Design — ELF32 combined main+PLT relocation transaction

Status: DONE — exact-head implementation CI PASSed at `600fad8edc3ac2a8f64fc2607e088b263adca902`

The shared application engine is split into a read-only preparation phase and a write/rollback phase. Single-table APIs prepare one table then apply it. The combined API prepares main and PLT, rejects duplicate pending target addresses across tables, concatenates main writes before PLT writes, and applies that sequence as one reverse-rollback domain. Table identity is additive metadata on writes and failures so relocation indexes remain unambiguous across tables.
