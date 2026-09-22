#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "elf/elf32_linker_metadata.h"
#include "elf/elf32_linker_strings.h"
#include "memory/guest_memory.h"

namespace liba32android::elf {

// Caller-selected resource ceilings for symbol/hash processing. Index
// construction uses max_symbols/max_hash_buckets/max_gnu_bloom_words. Later
// lookup/scope operations also use max_scope_objects/max_name_bytes.
// All values are counts/payload bytes, never host addresses.
struct Elf32SymbolLookupOptions {
    std::uint32_t max_symbols{};
    std::uint32_t max_hash_buckets{};
    std::uint32_t max_gnu_bloom_words{};
    std::uint32_t max_scope_objects{};
    std::uint32_t max_name_bytes{};
};

struct Elf32SysvHashIndex {
    std::uint32_t bucket_count{};
    std::uint32_t chain_count{};
    std::uint32_t buckets_guest_address{};
    std::uint32_t chains_guest_address{};
};

struct Elf32GnuHashIndex {
    std::uint32_t bucket_count{};
    std::uint32_t symbol_offset{};
    std::uint32_t bloom_word_count{};
    std::uint32_t bloom_shift{};
    std::uint32_t bloom_guest_address{};
    std::uint32_t buckets_guest_address{};
    std::uint32_t chains_guest_address{};
};

struct Elf32SymbolIndex {
    // Exact dynamic-symbol count established from DT_HASH nchain or from a
    // bounded terminating GNU hash chain when GNU hash is the only format.
    std::uint32_t symbol_count{};
    std::optional<Elf32SysvHashIndex> sysv_hash;
    std::optional<Elf32GnuHashIndex> gnu_hash;
};

enum class Elf32SymbolIndexError : std::uint8_t {
    None = 0,
    InvalidOptions,
    MissingStringTable,
    MissingSymbolTable,
    MissingHashTable,
    HashHeaderReadFailed,
    HashRangeOverflow,
    HashReadFailed,
    InvalidSysvHash,
    InvalidGnuHash,
    HashLimitExceeded,
    HashIndexOutOfRange,
    UnterminatedGnuChain,
    SymbolCountExceeded,
    SymbolRangeOverflow,
    SymbolReadFailed,
};

struct Elf32SymbolIndexResult {
    Elf32SymbolIndexError error{Elf32SymbolIndexError::None};
    Elf32SymbolIndex index;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32SymbolIndexError::None;
    }
};

struct Elf32Symbol {
    std::uint32_t name_offset{};
    std::uint32_t value{};
    std::uint32_t size{};
    std::uint8_t binding{};
    std::uint8_t type{};
    std::uint8_t visibility{};
    std::uint16_t section_index{};
};

struct Elf32ResolvedSymbol {
    std::uint32_t symbol_index{};
    std::string name;
    Elf32Symbol symbol;
    // Logical 32-bit guest value. This is never a host pointer.
    std::uint32_t guest_value{};
};

enum class Elf32SymbolLookupError : std::uint8_t {
    None = 0,
    InvalidOptions,
    InvalidMetadata,
    InvalidLookupName,
    UnsupportedVersioning,
    HashReadFailed,
    HashIndexOutOfRange,
    InvalidHashChain,
    SymbolReadFailed,
    StringReadFailed,
    SymbolNotFound,
    UnsupportedBinding,
    UnsupportedType,
    UnsupportedVisibility,
    UnsupportedSectionIndex,
    ValueOverflow,
};

struct Elf32ObjectSymbolLookupResult {
    Elf32SymbolLookupError error{Elf32SymbolLookupError::None};
    Elf32LinkerStringError string_error{Elf32LinkerStringError::None};
    Elf32ResolvedSymbol symbol;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32SymbolLookupError::None;
    }
};

// Build a read-only, bounded index for one already-loaded object's dynamic
// symbol table. No section headers or host pointers are used. The function
// reads only through GuestMemory, never mutates guest bytes/mappings, and
// validates the complete symbol-table range implied by the selected hash
// format before returning success.
[[nodiscard]] Elf32SymbolIndexResult build_elf32_symbol_index(
    const memory::GuestMemory& memory,
    const Elf32LinkerMetadata& metadata,
    const Elf32SymbolLookupOptions& options);

// Resolve one exact non-empty byte name within one already-indexed object.
// GNU hash is preferred when both formats exist; a GNU miss is not masked by
// SysV fallback. GLOBAL/WEAK DEFAULT/PROTECTED normal or SHN_ABS definitions
// are supported. LOCAL/UNDEF/HIDDEN/INTERNAL entries are ordinary misses.
// Matching TLS/COMMON/XINDEX/IFUNC/nonstandard semantics fail explicitly.
// Versioned metadata is rejected until version matching is implemented.
// The operation is read-only and uses only logical guest addresses.
[[nodiscard]] Elf32ObjectSymbolLookupResult lookup_elf32_symbol(
    const memory::GuestMemory& memory,
    std::uint32_t load_bias,
    const Elf32LinkerMetadata& metadata,
    const Elf32SymbolIndex& index,
    std::string_view name,
    const Elf32SymbolLookupOptions& options);

[[nodiscard]] const char* to_string(Elf32SymbolIndexError error) noexcept;
[[nodiscard]] const char* to_string(Elf32SymbolLookupError error) noexcept;

}  // namespace liba32android::elf
