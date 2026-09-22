#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "elf/elf32_symbol_lookup.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32HashTableMetadata;
using liba32android::elf::Elf32LinkerMetadata;
using liba32android::elf::Elf32StringTableMetadata;
using liba32android::elf::Elf32ObjectSymbolLookupResult;
using liba32android::elf::Elf32SymbolIndexError;
using liba32android::elf::Elf32SymbolLookupError;
using liba32android::elf::Elf32SymbolLookupOptions;
using liba32android::elf::Elf32SymbolTableMetadata;
using liba32android::elf::build_elf32_symbol_index;
using liba32android::elf::lookup_elf32_symbol;
using liba32android::memory::LinearGuestMemory;

constexpr std::uint32_t kMemoryBase = 0x1000;
constexpr std::uint32_t kStringTable = 0x1100;
constexpr std::uint32_t kSymbolTable = 0x1800;
constexpr std::uint32_t kSysvHash = 0x2800;
constexpr std::uint32_t kGnuHash = 0x3800;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

Elf32SymbolLookupOptions options() {
    return Elf32SymbolLookupOptions{
        .max_symbols = 16,
        .max_hash_buckets = 8,
        .max_gnu_bloom_words = 8,
        .max_scope_objects = 16,
        .max_name_bytes = 64,
    };
}

Elf32LinkerMetadata metadata(bool sysv, bool gnu) {
    Elf32LinkerMetadata result;
    result.string_table = Elf32StringTableMetadata{
        .guest_address = kStringTable,
        .size = 0x100,
    };
    result.symbol_table = Elf32SymbolTableMetadata{
        .guest_address = kSymbolTable,
        .entry_size = 16,
    };
    if (sysv) {
        result.sysv_hash_table =
            Elf32HashTableMetadata{.guest_address = kSysvHash};
    }
    if (gnu) {
        result.gnu_hash_table =
            Elf32HashTableMetadata{.guest_address = kGnuHash};
    }
    return result;
}

bool write_u32(LinearGuestMemory& memory,
               std::uint32_t address,
               std::uint32_t value) {
    const std::array<std::uint8_t, 4> bytes{
        static_cast<std::uint8_t>(value),
        static_cast<std::uint8_t>(value >> 8U),
        static_cast<std::uint8_t>(value >> 16U),
        static_cast<std::uint8_t>(value >> 24U),
    };
    return memory.write(address, bytes);
}

bool stage_sysv(LinearGuestMemory& memory,
                std::uint32_t bucket_count,
                std::uint32_t chain_count,
                std::array<std::uint32_t, 2> buckets,
                std::array<std::uint32_t, 4> chains) {
    if (!write_u32(memory, kSysvHash, bucket_count) ||
        !write_u32(memory, kSysvHash + 4U, chain_count)) {
        return false;
    }
    for (std::uint32_t i = 0; i < buckets.size(); ++i) {
        if (!write_u32(memory, kSysvHash + 8U + i * 4U, buckets[i])) {
            return false;
        }
    }
    const std::uint32_t chains_address =
        kSysvHash + 8U + bucket_count * 4U;
    for (std::uint32_t i = 0; i < chains.size(); ++i) {
        if (!write_u32(memory, chains_address + i * 4U, chains[i])) {
            return false;
        }
    }
    return true;
}

bool stage_gnu(LinearGuestMemory& memory,
               std::uint32_t bucket_count,
               std::uint32_t symbol_offset,
               std::uint32_t bloom_words,
               std::uint32_t bloom_shift,
               std::array<std::uint32_t, 2> buckets,
               std::array<std::uint32_t, 4> chains) {
    if (!write_u32(memory, kGnuHash, bucket_count) ||
        !write_u32(memory, kGnuHash + 4U, symbol_offset) ||
        !write_u32(memory, kGnuHash + 8U, bloom_words) ||
        !write_u32(memory, kGnuHash + 12U, bloom_shift)) {
        return false;
    }
    for (std::uint32_t i = 0; i < bloom_words; ++i) {
        if (!write_u32(memory, kGnuHash + 16U + i * 4U, 0)) {
            return false;
        }
    }
    const std::uint32_t buckets_address =
        kGnuHash + 16U + bloom_words * 4U;
    for (std::uint32_t i = 0; i < buckets.size(); ++i) {
        if (!write_u32(memory, buckets_address + i * 4U, buckets[i])) {
            return false;
        }
    }
    const std::uint32_t chains_address =
        buckets_address + bucket_count * 4U;
    for (std::uint32_t i = 0; i < chains.size(); ++i) {
        if (!write_u32(memory, chains_address + i * 4U, chains[i])) {
            return false;
        }
    }
    return true;
}

int test_required_inputs_and_options() {
    LinearGuestMemory memory(0x6000, kMemoryBase);
    auto valid = options();

    if (build_elf32_symbol_index(memory, metadata(true, false), {})
            .error != Elf32SymbolIndexError::InvalidOptions) {
        return fail("zero index limits were not rejected");
    }

    Elf32LinkerMetadata missing;
    if (build_elf32_symbol_index(memory, missing, valid).error !=
        Elf32SymbolIndexError::MissingStringTable) {
        return fail("missing STRTAB was not rejected");
    }
    missing.string_table =
        Elf32StringTableMetadata{.guest_address = kStringTable, .size = 16};
    if (build_elf32_symbol_index(memory, missing, valid).error !=
        Elf32SymbolIndexError::MissingSymbolTable) {
        return fail("missing SYMTAB was not rejected");
    }
    missing.symbol_table =
        Elf32SymbolTableMetadata{.guest_address = kSymbolTable,
                                 .entry_size = 16};
    if (build_elf32_symbol_index(memory, missing, valid).error !=
        Elf32SymbolIndexError::MissingHashTable) {
        return fail("missing dynamic hash was not rejected");
    }
    return 0;
}

int test_valid_sysv_index() {
    LinearGuestMemory memory(0x6000, kMemoryBase);
    if (!stage_sysv(memory, 2, 4, {1, 0}, {0, 2, 3, 0})) {
        return fail("could not stage valid SysV hash");
    }

    const auto result =
        build_elf32_symbol_index(memory, metadata(true, false), options());
    if (!result || result.index.symbol_count != 4 ||
        !result.index.sysv_hash.has_value() ||
        result.index.gnu_hash.has_value() ||
        result.index.sysv_hash->bucket_count != 2 ||
        result.index.sysv_hash->chain_count != 4 ||
        result.index.sysv_hash->buckets_guest_address != kSysvHash + 8U ||
        result.index.sysv_hash->chains_guest_address != kSysvHash + 16U) {
        return fail("valid SysV hash did not produce the expected symbol index");
    }
    return 0;
}

int test_sysv_failures() {
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!stage_sysv(memory, 0, 4, {0, 0}, {0, 0, 0, 0})) {
            return fail("could not stage zero-bucket SysV hash");
        }
        if (build_elf32_symbol_index(memory, metadata(true, false), options())
                .error != Elf32SymbolIndexError::InvalidSysvHash) {
            return fail("zero SysV bucket count was not rejected");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_u32(memory, kSysvHash, 9) ||
            !write_u32(memory, kSysvHash + 4U, 4)) {
            return fail("could not stage oversized SysV bucket count");
        }
        if (build_elf32_symbol_index(memory, metadata(true, false), options())
                .error != Elf32SymbolIndexError::HashLimitExceeded) {
            return fail("SysV bucket ceiling was not enforced");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_u32(memory, kSysvHash, 1) ||
            !write_u32(memory, kSysvHash + 4U, 17)) {
            return fail("could not stage oversized SysV symbol count");
        }
        if (build_elf32_symbol_index(memory, metadata(true, false), options())
                .error != Elf32SymbolIndexError::SymbolCountExceeded) {
            return fail("SysV nchain symbol ceiling was not enforced");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!stage_sysv(memory, 2, 4, {4, 0}, {0, 0, 0, 0})) {
            return fail("could not stage bad SysV bucket index");
        }
        if (build_elf32_symbol_index(memory, metadata(true, false), options())
                .error != Elf32SymbolIndexError::HashIndexOutOfRange) {
            return fail("out-of-range SysV bucket index was not rejected");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!stage_sysv(memory, 2, 4, {1, 0}, {0, 2, 4, 0})) {
            return fail("could not stage bad SysV chain index");
        }
        if (build_elf32_symbol_index(memory, metadata(true, false), options())
                .error != Elf32SymbolIndexError::HashIndexOutOfRange) {
            return fail("out-of-range SysV chain index was not rejected");
        }
    }
    return 0;
}

int test_valid_gnu_index() {
    LinearGuestMemory memory(0x6000, kMemoryBase);
    if (!stage_gnu(memory, 2, 1, 1, 5, {1, 3},
                   {0x100, 0x101, 0x201, 0})) {
        return fail("could not stage valid GNU hash");
    }

    const auto result =
        build_elf32_symbol_index(memory, metadata(false, true), options());
    if (!result || result.index.symbol_count != 4 ||
        result.index.sysv_hash.has_value() ||
        !result.index.gnu_hash.has_value() ||
        result.index.gnu_hash->bucket_count != 2 ||
        result.index.gnu_hash->symbol_offset != 1 ||
        result.index.gnu_hash->bloom_word_count != 1 ||
        result.index.gnu_hash->bloom_shift != 5 ||
        result.index.gnu_hash->bloom_guest_address != kGnuHash + 16U ||
        result.index.gnu_hash->buckets_guest_address != kGnuHash + 20U ||
        result.index.gnu_hash->chains_guest_address != kGnuHash + 28U) {
        return fail("valid GNU hash did not produce the expected symbol index");
    }

    LinearGuestMemory empty_memory(0x6000, kMemoryBase);
    if (!stage_gnu(empty_memory, 2, 1, 1, 5, {0, 0},
                   {0, 0, 0, 0})) {
        return fail("could not stage empty GNU buckets");
    }
    const auto empty =
        build_elf32_symbol_index(empty_memory, metadata(false, true), options());
    if (!empty || empty.index.symbol_count != 1) {
        return fail("all-zero GNU buckets did not preserve symoffset count");
    }
    return 0;
}

int test_gnu_failures() {
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!stage_gnu(memory, 1, 1, 3, 5, {1, 0},
                       {1, 0, 0, 0})) {
            return fail("could not stage non-power-of-two GNU bloom count");
        }
        if (build_elf32_symbol_index(memory, metadata(false, true), options())
                .error != Elf32SymbolIndexError::InvalidGnuHash) {
            return fail("non-power-of-two GNU bloom count was not rejected");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_u32(memory, kGnuHash, 1) ||
            !write_u32(memory, kGnuHash + 4U, 1) ||
            !write_u32(memory, kGnuHash + 8U, 1) ||
            !write_u32(memory, kGnuHash + 12U, 32)) {
            return fail("could not stage invalid GNU bloom shift");
        }
        if (build_elf32_symbol_index(memory, metadata(false, true), options())
                .error != Elf32SymbolIndexError::InvalidGnuHash) {
            return fail("out-of-range GNU bloom shift was not rejected");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_u32(memory, kGnuHash, 9) ||
            !write_u32(memory, kGnuHash + 4U, 1) ||
            !write_u32(memory, kGnuHash + 8U, 1) ||
            !write_u32(memory, kGnuHash + 12U, 5)) {
            return fail("could not stage oversized GNU bucket count");
        }
        if (build_elf32_symbol_index(memory, metadata(false, true), options())
                .error != Elf32SymbolIndexError::HashLimitExceeded) {
            return fail("GNU bucket ceiling was not enforced");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!stage_gnu(memory, 1, 2, 1, 5, {1, 0},
                       {1, 0, 0, 0})) {
            return fail("could not stage GNU bucket below symoffset");
        }
        if (build_elf32_symbol_index(memory, metadata(false, true), options())
                .error != Elf32SymbolIndexError::InvalidGnuHash) {
            return fail("GNU bucket below symoffset was not rejected");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!stage_gnu(memory, 1, 1, 1, 5, {16, 0},
                       {1, 0, 0, 0})) {
            return fail("could not stage oversized GNU symbol index");
        }
        if (build_elf32_symbol_index(memory, metadata(false, true), options())
                .error != Elf32SymbolIndexError::SymbolCountExceeded) {
            return fail("GNU symbol ceiling was not enforced");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!stage_gnu(memory, 1, 1, 1, 5, {2, 0},
                       {0, 0x100, 0x100, 0})) {
            return fail("could not stage unterminated GNU chain");
        }
        auto limited = options();
        limited.max_symbols = 4;
        if (build_elf32_symbol_index(memory, metadata(false, true), limited)
                .error != Elf32SymbolIndexError::UnterminatedGnuChain) {
            return fail("unterminated GNU-only chain was not rejected");
        }
    }
    return 0;
}

int test_dual_hash_consistency() {
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_u32(memory, kSysvHash, 1) ||
            !write_u32(memory, kSysvHash + 4U, 5) ||
            !write_u32(memory, kSysvHash + 8U, 1)) {
            return fail("could not stage dual-hash SysV header");
        }
        for (std::uint32_t i = 0; i < 5; ++i) {
            if (!write_u32(memory, kSysvHash + 12U + i * 4U, 0)) {
                return fail("could not stage dual-hash SysV chains");
            }
        }
        if (!stage_gnu(memory, 1, 1, 1, 5, {4, 0},
                       {0, 0, 0, 1})) {
            return fail("could not stage dual-hash GNU table");
        }

        const auto result =
            build_elf32_symbol_index(memory, metadata(true, true), options());
        if (!result || result.index.symbol_count != 5 ||
            !result.index.sysv_hash.has_value() ||
            !result.index.gnu_hash.has_value()) {
            return fail("valid dual hash did not retain SysV symbol extent");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_u32(memory, kSysvHash, 1) ||
            !write_u32(memory, kSysvHash + 4U, 5) ||
            !write_u32(memory, kSysvHash + 8U, 1)) {
            return fail("could not stage dual-hash bound header");
        }
        for (std::uint32_t i = 0; i < 5; ++i) {
            if (!write_u32(memory, kSysvHash + 12U + i * 4U, 0)) {
                return fail("could not stage dual-hash bound chains");
            }
        }
        if (!stage_gnu(memory, 1, 1, 1, 5, {5, 0},
                       {0, 0, 0, 1})) {
            return fail("could not stage out-of-range GNU bucket");
        }
        if (build_elf32_symbol_index(memory, metadata(true, true), options())
                .error != Elf32SymbolIndexError::HashIndexOutOfRange) {
            return fail("GNU bucket outside SysV nchain was not rejected");
        }
    }
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_u32(memory, kSysvHash, 1) ||
            !write_u32(memory, kSysvHash + 4U, 5) ||
            !write_u32(memory, kSysvHash + 8U, 1)) {
            return fail("could not stage unterminated dual SysV header");
        }
        for (std::uint32_t i = 0; i < 5; ++i) {
            if (!write_u32(memory, kSysvHash + 12U + i * 4U, 0)) {
                return fail("could not stage unterminated dual SysV chains");
            }
        }
        if (!stage_gnu(memory, 1, 1, 1, 5, {4, 0},
                       {0, 0, 0, 0})) {
            return fail("could not stage unterminated dual GNU chain");
        }
        if (build_elf32_symbol_index(memory, metadata(true, true), options())
                .error != Elf32SymbolIndexError::UnterminatedGnuChain) {
            return fail("unterminated GNU chain with SysV count was not rejected");
        }
    }
    return 0;
}

int test_read_failures_and_symbol_range() {
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        auto bad = metadata(true, false);
        bad.sysv_hash_table =
            Elf32HashTableMetadata{.guest_address = 0x8000};
        if (build_elf32_symbol_index(memory, bad, options()).error !=
            Elf32SymbolIndexError::HashHeaderReadFailed) {
            return fail("unreadable hash header was not classified");
        }
    }
    {
        LinearGuestMemory memory(0x1000, kMemoryBase);
        Elf32LinkerMetadata bad;
        bad.string_table =
            Elf32StringTableMetadata{.guest_address = 0x1100, .size = 0x20};
        bad.symbol_table =
            Elf32SymbolTableMetadata{.guest_address = 0x1ff0,
                                     .entry_size = 16};
        bad.sysv_hash_table =
            Elf32HashTableMetadata{.guest_address = 0x1200};
        if (!write_u32(memory, 0x1200, 1) ||
            !write_u32(memory, 0x1204, 2) ||
            !write_u32(memory, 0x1208, 1) ||
            !write_u32(memory, 0x120c, 0) ||
            !write_u32(memory, 0x1210, 0)) {
            return fail("could not stage symbol-range failure table");
        }
        if (build_elf32_symbol_index(memory, bad, options()).error !=
            Elf32SymbolIndexError::SymbolReadFailed) {
            return fail("unreadable full symbol range was not rejected");
        }
    }
    return 0;
}

bool write_string(LinearGuestMemory& memory,
                  std::uint32_t offset,
                  std::string_view value) {
    std::vector<std::uint8_t> bytes(value.begin(), value.end());
    bytes.push_back(0);
    return memory.write(kStringTable + offset, bytes);
}

bool write_symbol(LinearGuestMemory& memory,
                  std::uint32_t index,
                  std::uint32_t name_offset,
                  std::uint32_t value,
                  std::uint32_t size,
                  std::uint8_t binding,
                  std::uint8_t type,
                  std::uint8_t other,
                  std::uint16_t section_index) {
    std::array<std::uint8_t, 16> bytes{};
    const auto put_u32 = [&](std::size_t offset, std::uint32_t word) {
        bytes[offset] = static_cast<std::uint8_t>(word);
        bytes[offset + 1] = static_cast<std::uint8_t>(word >> 8U);
        bytes[offset + 2] = static_cast<std::uint8_t>(word >> 16U);
        bytes[offset + 3] = static_cast<std::uint8_t>(word >> 24U);
    };
    put_u32(0, name_offset);
    put_u32(4, value);
    put_u32(8, size);
    bytes[12] = static_cast<std::uint8_t>((binding << 4U) | (type & 0x0fU));
    bytes[13] = other;
    bytes[14] = static_cast<std::uint8_t>(section_index);
    bytes[15] = static_cast<std::uint8_t>(section_index >> 8U);
    return memory.write(kSymbolTable + index * 16U, bytes);
}

bool stage_sysv_lookup(LinearGuestMemory& memory,
                       std::uint32_t first_symbol,
                       const std::vector<std::uint32_t>& chains) {
    const std::uint32_t count = static_cast<std::uint32_t>(chains.size());
    if (count == 0 ||
        !write_u32(memory, kSysvHash, 1) ||
        !write_u32(memory, kSysvHash + 4U, count) ||
        !write_u32(memory, kSysvHash + 8U, first_symbol)) {
        return false;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        if (!write_u32(memory, kSysvHash + 12U + i * 4U, chains[i])) {
            return false;
        }
    }
    return true;
}

std::uint32_t test_gnu_hash(std::string_view name) {
    std::uint32_t hash = 5381U;
    for (const unsigned char byte : name) {
        hash = hash * 33U + byte;
    }
    return hash;
}

bool stage_gnu_lookup(LinearGuestMemory& memory,
                      const std::vector<std::uint32_t>& chains) {
    if (chains.empty() ||
        !write_u32(memory, kGnuHash, 1) ||
        !write_u32(memory, kGnuHash + 4U, 1) ||
        !write_u32(memory, kGnuHash + 8U, 1) ||
        !write_u32(memory, kGnuHash + 12U, 5) ||
        !write_u32(memory, kGnuHash + 16U, 0xffffffffU) ||
        !write_u32(memory, kGnuHash + 20U, 1)) {
        return false;
    }
    for (std::uint32_t i = 0; i < chains.size(); ++i) {
        if (!write_u32(memory, kGnuHash + 24U + i * 4U, chains[i])) {
            return false;
        }
    }
    return true;
}

int test_indexing_is_read_only() {
    LinearGuestMemory memory(0x6000, kMemoryBase);
    if (!stage_sysv(memory, 2, 4, {1, 0}, {0, 2, 3, 0})) {
        return fail("could not stage read-only SysV hash");
    }
    const std::array<std::uint8_t, 4> sentinel{0xde, 0xad, 0xbe, 0xef};
    if (!memory.write(kStringTable, sentinel)) {
        return fail("could not stage read-only sentinel");
    }

    const auto result =
        build_elf32_symbol_index(memory, metadata(true, false), options());
    if (!result) return fail("read-only indexing setup failed");

    std::array<std::uint8_t, 4> after{};
    if (!memory.read(kStringTable, after) || after != sentinel) {
        return fail("symbol indexing mutated guest memory");
    }
    return 0;
}

int test_sysv_exact_lookup_and_collision() {
    LinearGuestMemory memory(0x6000, kMemoryBase);
    if (!write_string(memory, 1, "other") ||
        !write_string(memory, 16, "target") ||
        !write_symbol(memory, 1, 1, 0x100, 4, 1, 2, 0, 1) ||
        !write_symbol(memory, 2, 16, 0x200, 8, 1, 2, 0, 1) ||
        !stage_sysv_lookup(memory, 1, {0, 2, 0})) {
        return fail("could not stage SysV lookup fixture");
    }

    const auto index =
        build_elf32_symbol_index(memory, metadata(true, false), options());
    if (!index) return fail("SysV lookup index construction failed");

    const auto found = lookup_elf32_symbol(
        memory, 0x4000, metadata(true, false), index.index, "target", options());
    if (!found || found.symbol.symbol_index != 2 ||
        found.symbol.name != "target" ||
        found.symbol.symbol.binding != 1 ||
        found.symbol.symbol.type != 2 ||
        found.symbol.guest_value != 0x4200) {
        return fail("SysV exact-name lookup did not return the expected symbol");
    }

    const auto missing = lookup_elf32_symbol(
        memory, 0x4000, metadata(true, false), index.index, "missing", options());
    if (missing.error != Elf32SymbolLookupError::SymbolNotFound) {
        return fail("SysV lookup miss was not classified");
    }
    return 0;
}

int test_gnu_exact_lookup() {
    LinearGuestMemory memory(0x6000, kMemoryBase);
    constexpr std::string_view first_name = "alpha";
    constexpr std::string_view second_name = "target";
    const std::uint32_t first_hash = test_gnu_hash(first_name);
    const std::uint32_t second_hash = test_gnu_hash(second_name);

    if (!write_string(memory, 1, first_name) ||
        !write_string(memory, 16, second_name) ||
        !write_symbol(memory, 1, 1, 0x100, 4, 1, 1, 0, 1) ||
        !write_symbol(memory, 2, 16, 0x280, 4, 1, 2, 0, 1) ||
        !stage_gnu_lookup(memory, {first_hash & ~1U, second_hash | 1U})) {
        return fail("could not stage GNU lookup fixture");
    }

    const auto index =
        build_elf32_symbol_index(memory, metadata(false, true), options());
    if (!index || index.index.symbol_count != 3) {
        return fail("GNU lookup index construction failed");
    }

    const auto found = lookup_elf32_symbol(
        memory, 0x4000, metadata(false, true), index.index,
        second_name, options());
    if (!found || found.symbol.symbol_index != 2 ||
        found.symbol.guest_value != 0x4280) {
        return fail("GNU exact-name lookup did not return the expected symbol");
    }
    return 0;
}

int test_eligibility_and_weak_first() {
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_string(memory, 1, "target") ||
            !write_symbol(memory, 1, 1, 0x100, 4, 0, 2, 0, 1) ||
            !write_symbol(memory, 2, 1, 0x200, 4, 1, 2, 0, 0) ||
            !write_symbol(memory, 3, 1, 0x300, 4, 1, 2, 2, 1) ||
            !write_symbol(memory, 4, 1, 0x400, 4, 1, 2, 1, 1) ||
            !write_symbol(memory, 5, 1, 0x555, 4, 2, 2, 3, 1) ||
            !stage_sysv_lookup(memory, 1, {0, 2, 3, 4, 5, 0})) {
            return fail("could not stage eligibility lookup fixture");
        }
        const auto index =
            build_elf32_symbol_index(memory, metadata(true, false), options());
        const auto found = lookup_elf32_symbol(
            memory, 0x4000, metadata(true, false), index.index,
            "target", options());
        if (!found || found.symbol.symbol_index != 5 ||
            found.symbol.symbol.binding != 2 ||
            found.symbol.symbol.visibility != 3 ||
            found.symbol.guest_value != 0x4555) {
            return fail("local/undefined/hidden filtering or weak/protected lookup failed");
        }
    }

    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_string(memory, 1, "target") ||
            !write_symbol(memory, 1, 1, 0x100, 4, 2, 2, 0, 1) ||
            !write_symbol(memory, 2, 1, 0x200, 4, 1, 2, 0, 1) ||
            !stage_sysv_lookup(memory, 1, {0, 2, 0})) {
            return fail("could not stage weak-first fixture");
        }
        const auto index =
            build_elf32_symbol_index(memory, metadata(true, false), options());
        const auto found = lookup_elf32_symbol(
            memory, 0x4000, metadata(true, false), index.index,
            "target", options());
        if (!found || found.symbol.symbol_index != 1 ||
            found.symbol.symbol.binding != 2) {
            return fail("later strong symbol incorrectly replaced earlier weak definition");
        }
    }
    return 0;
}

int test_absolute_and_value_overflow() {
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_string(memory, 1, "absolute") ||
            !write_symbol(memory, 1, 1, 0x12345678U, 4, 1, 1, 0, 0xfff1) ||
            !stage_sysv_lookup(memory, 1, {0, 0})) {
            return fail("could not stage absolute-symbol fixture");
        }
        const auto index =
            build_elf32_symbol_index(memory, metadata(true, false), options());
        const auto found = lookup_elf32_symbol(
            memory, 0xf0000000U, metadata(true, false), index.index,
            "absolute", options());
        if (!found || found.symbol.guest_value != 0x12345678U) {
            return fail("SHN_ABS symbol was incorrectly rebased");
        }
    }

    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_string(memory, 1, "target") ||
            !write_symbol(memory, 1, 1, 0x40, 4, 1, 1, 0, 1) ||
            !stage_sysv_lookup(memory, 1, {0, 0})) {
            return fail("could not stage value-overflow fixture");
        }
        const auto index =
            build_elf32_symbol_index(memory, metadata(true, false), options());
        const auto result = lookup_elf32_symbol(
            memory, 0xfffffff0U, metadata(true, false), index.index,
            "target", options());
        if (result.error != Elf32SymbolLookupError::ValueOverflow) {
            return fail("resolved guest-value overflow was not rejected");
        }
    }
    return 0;
}

int test_matching_unsupported_forms() {
    struct Case {
        std::uint8_t binding;
        std::uint8_t type;
        std::uint8_t other;
        std::uint16_t section_index;
        Elf32SymbolLookupError expected;
    };
    constexpr std::array cases{
        Case{3, 2, 0, 1, Elf32SymbolLookupError::UnsupportedBinding},
        Case{1, 6, 0, 1, Elf32SymbolLookupError::UnsupportedType},
        Case{1, 10, 0, 1, Elf32SymbolLookupError::UnsupportedType},
        Case{1, 1, 4, 1, Elf32SymbolLookupError::UnsupportedVisibility},
        Case{1, 1, 0, 0xfff2, Elf32SymbolLookupError::UnsupportedSectionIndex},
        Case{1, 1, 0, 0xffff, Elf32SymbolLookupError::UnsupportedSectionIndex},
    };

    for (const Case& item : cases) {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_string(memory, 1, "target") ||
            !write_symbol(memory, 1, 1, 0x100, 4,
                          item.binding, item.type, item.other,
                          item.section_index) ||
            !stage_sysv_lookup(memory, 1, {0, 0})) {
            return fail("could not stage unsupported-symbol fixture");
        }
        const auto index =
            build_elf32_symbol_index(memory, metadata(true, false), options());
        const auto result = lookup_elf32_symbol(
            memory, 0x4000, metadata(true, false), index.index,
            "target", options());
        if (result.error != item.expected) {
            return fail("matching unsupported symbol form was not rejected explicitly");
        }
    }
    return 0;
}

int test_version_string_and_chain_failures() {
    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_string(memory, 1, "target") ||
            !write_symbol(memory, 1, 1, 0x100, 4, 1, 2, 0, 1) ||
            !stage_sysv_lookup(memory, 1, {0, 0})) {
            return fail("could not stage versioning fixture");
        }
        auto md = metadata(true, false);
        const auto index = build_elf32_symbol_index(memory, md, options());
        md.has_symbol_versioning = true;
        if (lookup_elf32_symbol(memory, 0x4000, md, index.index,
                                "target", options()).error !=
            Elf32SymbolLookupError::UnsupportedVersioning) {
            return fail("versioned name-only lookup was not rejected");
        }
        if (lookup_elf32_symbol(memory, 0x4000, metadata(true, false),
                                index.index, "", options()).error !=
            Elf32SymbolLookupError::InvalidLookupName) {
            return fail("empty lookup name was not rejected");
        }
    }

    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_symbol(memory, 1, 0x100, 0x100, 4, 1, 2, 0, 1) ||
            !stage_sysv_lookup(memory, 1, {0, 0})) {
            return fail("could not stage bad-name fixture");
        }
        const auto index =
            build_elf32_symbol_index(memory, metadata(true, false), options());
        const auto result = lookup_elf32_symbol(
            memory, 0x4000, metadata(true, false), index.index,
            "target", options());
        if (result.error != Elf32SymbolLookupError::StringReadFailed ||
            result.string_error !=
                liba32android::elf::Elf32LinkerStringError::StringOffsetOutOfRange) {
            return fail("candidate string-table failure was not preserved");
        }
    }

    {
        LinearGuestMemory memory(0x6000, kMemoryBase);
        if (!write_string(memory, 1, "other") ||
            !write_symbol(memory, 1, 1, 0x100, 4, 1, 2, 0, 1) ||
            !write_symbol(memory, 2, 1, 0x200, 4, 1, 2, 0, 1) ||
            !stage_sysv_lookup(memory, 1, {0, 2, 1})) {
            return fail("could not stage cyclic SysV chain");
        }
        const auto index =
            build_elf32_symbol_index(memory, metadata(true, false), options());
        const auto result = lookup_elf32_symbol(
            memory, 0x4000, metadata(true, false), index.index,
            "missing", options());
        if (result.error != Elf32SymbolLookupError::InvalidHashChain) {
            return fail("cyclic SysV hash chain was not bounded/rejected");
        }
    }
    return 0;
}

int test_lookup_is_read_only() {
    LinearGuestMemory memory(0x6000, kMemoryBase);
    if (!write_string(memory, 1, "target") ||
        !write_symbol(memory, 1, 1, 0x100, 4, 1, 2, 0, 1) ||
        !stage_sysv_lookup(memory, 1, {0, 0})) {
        return fail("could not stage read-only lookup fixture");
    }

    const std::array<std::uint8_t, 4> sentinel{0xde, 0xad, 0xbe, 0xef};
    constexpr std::uint32_t sentinel_address = 0x5000;
    if (!memory.write(sentinel_address, sentinel)) {
        return fail("could not stage lookup sentinel");
    }

    const auto index =
        build_elf32_symbol_index(memory, metadata(true, false), options());
    const auto result = lookup_elf32_symbol(
        memory, 0x4000, metadata(true, false), index.index,
        "target", options());
    if (!result) return fail("read-only lookup setup failed");

    std::array<std::uint8_t, 4> after{};
    if (!memory.read(sentinel_address, after) || after != sentinel) {
        return fail("symbol lookup mutated unrelated guest bytes");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_required_inputs_and_options(); status != 0) return status;
    if (const int status = test_valid_sysv_index(); status != 0) return status;
    if (const int status = test_sysv_failures(); status != 0) return status;
    if (const int status = test_valid_gnu_index(); status != 0) return status;
    if (const int status = test_gnu_failures(); status != 0) return status;
    if (const int status = test_dual_hash_consistency(); status != 0) return status;
    if (const int status = test_read_failures_and_symbol_range(); status != 0) return status;
    if (const int status = test_indexing_is_read_only(); status != 0) return status;
    if (const int status = test_sysv_exact_lookup_and_collision(); status != 0) return status;
    if (const int status = test_gnu_exact_lookup(); status != 0) return status;
    if (const int status = test_eligibility_and_weak_first(); status != 0) return status;
    if (const int status = test_absolute_and_value_overflow(); status != 0) return status;
    if (const int status = test_matching_unsupported_forms(); status != 0) return status;
    if (const int status = test_version_string_and_chain_failures(); status != 0) return status;
    if (const int status = test_lookup_is_read_only(); status != 0) return status;
    return 0;
}
