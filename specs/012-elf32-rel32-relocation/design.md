# Design — ELF32 ARM R_ARM_REL32 relocation

Status: ACTIVE — implementation prepared; exact-head validation NOT RUN

## Formula source

AAELF32 defines relocation type 3 as `((S + A) | T) - P`. It also defines relocation `S` for `STT_FUNC` without the symbol's bit-zero Thumb discriminator, while `T` is one only when the defining function symbol addresses Thumb code.

## Integration

The current relocation reference retained the request symbol plus the defining object's guest value/index identity. REL32 additionally needs the defining symbol's type/raw value, so successful graph resolution now stores additive `defining_symbol` metadata.

During final-word preparation:

1. start from the resolved guest value;
2. if the defining symbol is a Thumb `STT_FUNC`, clear bit zero from relocation `S` and set `T=1`;
3. compute `((S + A) | T) - P` with unsigned 32-bit wraparound;
4. enqueue the normal pending write.

Unresolved weak references have no defining symbol and therefore use `S=0,T=0`.

No transaction or permission machinery changes.
