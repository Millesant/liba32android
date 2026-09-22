#include "elf/elf32_symbol_lookup.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace liba32android::elf {
namespace {

constexpr std::uint32_t kElf32SymbolEntrySize = 16;
constexpr std::uint64_t kGuestAddressSpaceSize = std::uint64_t{1} << 32;
constexpr std::size_t kReadValidationChunkSize = 256;

[[nodiscard]] Elf32SymbolIndexResult failure(Elf32SymbolIndexError error) {
    Elf32SymbolIndexResult result;
    result.error = error;
    return result;
}

[[nodiscard]] std::uint32_t decode_u32_le(
    const std::array<std::uint8_t, 4>& bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

[[nodiscard]] bool read_u32(const memory::GuestMemory& memory,
                            std::uint32_t address,
                            std::uint32_t& value) {
    std::array<std::uint8_t, 4> bytes{};
    if (!memory.read(address, bytes)) return false;
    value = decode_u32_le(bytes);
    return true;
}

[[nodiscard]] bool checked_add(std::uint32_t base,
                               std::uint64_t offset,
                               std::uint32_t& result) {
    const std::uint64_t value = static_cast<std::uint64_t>(base) + offset;
    if (value > std::numeric_limits<std::uint32_t>::max()) return false;
    result = static_cast<std::uint32_t>(value);
    return true;
}

[[nodiscard]] bool range_fits(std::uint32_t address, std::uint64_t size) {
    return static_cast<std::uint64_t>(address) + size <=
           kGuestAddressSpaceSize;
}

[[nodiscard]] bool readable_range(const memory::GuestMemory& memory,
                                  std::uint32_t address,
                                  std::uint64_t size) {
    if (size == 0) return true;
    if (!range_fits(address, size)) return false;

    std::array<std::uint8_t, kReadValidationChunkSize> buffer{};
    std::uint64_t offset = 0;
    while (offset < size) {
        const std::size_t chunk = static_cast<std::size_t>(
            std::min<std::uint64_t>(buffer.size(), size - offset));
        const std::uint64_t current =
            static_cast<std::uint64_t>(address) + offset;
        if (current > std::numeric_limits<std::uint32_t>::max() ||
            !memory.read(static_cast<std::uint32_t>(current),
                         std::span<std::uint8_t>(buffer.data(), chunk))) {
            return false;
        }
        offset += chunk;
    }
    return true;
}

[[nodiscard]] bool is_power_of_two(std::uint32_t value) noexcept {
    return value != 0 && (value & (value - 1U)) == 0;
}

[[nodiscard]] bool read_indexed_word(const memory::GuestMemory& memory,
                                     std::uint32_t base,
                                     std::uint32_t index,
                                     std::uint32_t& value) {
    std::uint32_t address = 0;
    if (!checked_add(base, static_cast<std::uint64_t>(index) * 4U, address)) {
        return false;
    }
    return read_u32(memory, address, value);
}

}  // namespace

Elf32SymbolIndexResult build_elf32_symbol_index(
    const memory::GuestMemory& memory,
    const Elf32LinkerMetadata& metadata,
    const Elf32SymbolLookupOptions& options) {
    if (options.max_symbols == 0 ||
        options.max_hash_buckets == 0 ||
        options.max_gnu_bloom_words == 0) {
        return failure(Elf32SymbolIndexError::InvalidOptions);
    }
    if (!metadata.string_table.has_value()) {
        return failure(Elf32SymbolIndexError::MissingStringTable);
    }
    if (!metadata.symbol_table.has_value()) {
        return failure(Elf32SymbolIndexError::MissingSymbolTable);
    }
    if (!metadata.sysv_hash_table.has_value() &&
        !metadata.gnu_hash_table.has_value()) {
        return failure(Elf32SymbolIndexError::MissingHashTable);
    }

    Elf32SymbolIndexResult result;
    std::optional<std::uint32_t> sysv_symbol_count;

    if (metadata.sysv_hash_table.has_value()) {
        const std::uint32_t hash_address =
            metadata.sysv_hash_table->guest_address;

        std::uint32_t bucket_count = 0;
        std::uint32_t chain_count = 0;
        if (!read_u32(memory, hash_address, bucket_count)) {
            return failure(Elf32SymbolIndexError::HashHeaderReadFailed);
        }
        std::uint32_t second_word = 0;
        if (!checked_add(hash_address, 4U, second_word) ||
            !read_u32(memory, second_word, chain_count)) {
            return failure(Elf32SymbolIndexError::HashHeaderReadFailed);
        }

        if (bucket_count == 0 || chain_count == 0) {
            return failure(Elf32SymbolIndexError::InvalidSysvHash);
        }
        if (bucket_count > options.max_hash_buckets) {
            return failure(Elf32SymbolIndexError::HashLimitExceeded);
        }
        if (chain_count > options.max_symbols) {
            return failure(Elf32SymbolIndexError::SymbolCountExceeded);
        }

        const std::uint64_t bucket_bytes =
            static_cast<std::uint64_t>(bucket_count) * 4U;
        const std::uint64_t chain_bytes =
            static_cast<std::uint64_t>(chain_count) * 4U;
        const std::uint64_t total_bytes = 8U + bucket_bytes + chain_bytes;
        if (!range_fits(hash_address, total_bytes)) {
            return failure(Elf32SymbolIndexError::HashRangeOverflow);
        }

        std::uint32_t buckets_address = 0;
        std::uint32_t chains_address = 0;
        if (!checked_add(hash_address, 8U, buckets_address) ||
            !checked_add(buckets_address, bucket_bytes, chains_address)) {
            return failure(Elf32SymbolIndexError::HashRangeOverflow);
        }
        if (!readable_range(memory, hash_address, total_bytes)) {
            return failure(Elf32SymbolIndexError::HashReadFailed);
        }

        for (std::uint32_t i = 0; i < bucket_count; ++i) {
            std::uint32_t symbol_index = 0;
            if (!read_indexed_word(memory, buckets_address, i, symbol_index)) {
                return failure(Elf32SymbolIndexError::HashReadFailed);
            }
            if (symbol_index >= chain_count && symbol_index != 0) {
                return failure(Elf32SymbolIndexError::HashIndexOutOfRange);
            }
        }
        for (std::uint32_t i = 0; i < chain_count; ++i) {
            std::uint32_t next_index = 0;
            if (!read_indexed_word(memory, chains_address, i, next_index)) {
                return failure(Elf32SymbolIndexError::HashReadFailed);
            }
            if (next_index >= chain_count && next_index != 0) {
                return failure(Elf32SymbolIndexError::HashIndexOutOfRange);
            }
        }

        result.index.sysv_hash = Elf32SysvHashIndex{
            .bucket_count = bucket_count,
            .chain_count = chain_count,
            .buckets_guest_address = buckets_address,
            .chains_guest_address = chains_address,
        };
        sysv_symbol_count = chain_count;
        result.index.symbol_count = chain_count;
    }

    if (metadata.gnu_hash_table.has_value()) {
        const std::uint32_t hash_address =
            metadata.gnu_hash_table->guest_address;

        std::array<std::uint32_t, 4> header{};
        for (std::uint32_t i = 0; i < header.size(); ++i) {
            std::uint32_t address = 0;
            if (!checked_add(hash_address,
                             static_cast<std::uint64_t>(i) * 4U,
                             address) ||
                !read_u32(memory, address, header[i])) {
                return failure(Elf32SymbolIndexError::HashHeaderReadFailed);
            }
        }

        const std::uint32_t bucket_count = header[0];
        const std::uint32_t symbol_offset = header[1];
        const std::uint32_t bloom_word_count = header[2];
        const std::uint32_t bloom_shift = header[3];

        if (bucket_count == 0 || symbol_offset == 0 ||
            !is_power_of_two(bloom_word_count)) {
            return failure(Elf32SymbolIndexError::InvalidGnuHash);
        }
        if (bucket_count > options.max_hash_buckets ||
            bloom_word_count > options.max_gnu_bloom_words) {
            return failure(Elf32SymbolIndexError::HashLimitExceeded);
        }
        if (symbol_offset > options.max_symbols) {
            return failure(Elf32SymbolIndexError::SymbolCountExceeded);
        }

        const std::uint64_t bloom_bytes =
            static_cast<std::uint64_t>(bloom_word_count) * 4U;
        const std::uint64_t bucket_bytes =
            static_cast<std::uint64_t>(bucket_count) * 4U;
        const std::uint64_t prefix_bytes = 16U + bloom_bytes + bucket_bytes;
        if (!range_fits(hash_address, prefix_bytes)) {
            return failure(Elf32SymbolIndexError::HashRangeOverflow);
        }

        std::uint32_t bloom_address = 0;
        std::uint32_t buckets_address = 0;
        std::uint32_t chains_address = 0;
        if (!checked_add(hash_address, 16U, bloom_address) ||
            !checked_add(bloom_address, bloom_bytes, buckets_address) ||
            !checked_add(buckets_address, bucket_bytes, chains_address)) {
            return failure(Elf32SymbolIndexError::HashRangeOverflow);
        }
        if (!readable_range(memory, hash_address, prefix_bytes)) {
            return failure(Elf32SymbolIndexError::HashReadFailed);
        }

        std::uint32_t maximum_bucket = 0;
        for (std::uint32_t i = 0; i < bucket_count; ++i) {
            std::uint32_t symbol_index = 0;
            if (!read_indexed_word(memory, buckets_address, i, symbol_index)) {
                return failure(Elf32SymbolIndexError::HashReadFailed);
            }
            if (symbol_index == 0) continue;
            if (symbol_index < symbol_offset) {
                return failure(Elf32SymbolIndexError::InvalidGnuHash);
            }
            if (symbol_index >= options.max_symbols) {
                return failure(Elf32SymbolIndexError::SymbolCountExceeded);
            }
            if (sysv_symbol_count.has_value() &&
                symbol_index >= *sysv_symbol_count) {
                return failure(Elf32SymbolIndexError::HashIndexOutOfRange);
            }
            maximum_bucket = std::max(maximum_bucket, symbol_index);
        }

        if (sysv_symbol_count.has_value()) {
            if (symbol_offset > *sysv_symbol_count) {
                return failure(Elf32SymbolIndexError::InvalidGnuHash);
            }
            const std::uint64_t chain_count =
                static_cast<std::uint64_t>(*sysv_symbol_count) -
                symbol_offset;
            const std::uint64_t chain_bytes = chain_count * 4U;
            if (!range_fits(chains_address, chain_bytes)) {
                return failure(Elf32SymbolIndexError::HashRangeOverflow);
            }
            if (!readable_range(memory, chains_address, chain_bytes)) {
                return failure(Elf32SymbolIndexError::HashReadFailed);
            }

            // SysV nchain supplies the authoritative dynsym extent, but every
            // nonempty GNU bucket must still reach a low-bit chain terminator
            // inside that same finite extent. Validate this now so a later GNU
            // lookup cannot silently accept inconsistent dual-hash metadata.
            for (std::uint32_t bucket_index = 0;
                 bucket_index < bucket_count; ++bucket_index) {
                std::uint32_t bucket_symbol = 0;
                if (!read_indexed_word(memory, buckets_address, bucket_index,
                                       bucket_symbol)) {
                    return failure(Elf32SymbolIndexError::HashReadFailed);
                }
                if (bucket_symbol == 0) continue;

                bool terminated = false;
                for (std::uint32_t symbol_index = bucket_symbol;
                     symbol_index < *sysv_symbol_count; ++symbol_index) {
                    const std::uint64_t chain_index =
                        static_cast<std::uint64_t>(symbol_index) -
                        symbol_offset;
                    std::uint32_t chain_hash = 0;
                    if (!read_indexed_word(
                            memory, chains_address,
                            static_cast<std::uint32_t>(chain_index),
                            chain_hash)) {
                        return failure(Elf32SymbolIndexError::HashReadFailed);
                    }
                    if ((chain_hash & 1U) != 0) {
                        terminated = true;
                        break;
                    }
                }
                if (!terminated) {
                    return failure(
                        Elf32SymbolIndexError::UnterminatedGnuChain);
                }
            }
        } else if (maximum_bucket == 0) {
            result.index.symbol_count = symbol_offset;
        } else {
            std::uint32_t symbol_index = maximum_bucket;
            bool terminated = false;
            while (symbol_index < options.max_symbols) {
                const std::uint64_t chain_index =
                    static_cast<std::uint64_t>(symbol_index) - symbol_offset;
                std::uint32_t chain_address = 0;
                if (!checked_add(chains_address, chain_index * 4U,
                                 chain_address)) {
                    return failure(Elf32SymbolIndexError::HashRangeOverflow);
                }

                std::uint32_t chain_hash = 0;
                if (!read_u32(memory, chain_address, chain_hash)) {
                    return failure(Elf32SymbolIndexError::HashReadFailed);
                }
                if ((chain_hash & 1U) != 0) {
                    if (symbol_index ==
                        std::numeric_limits<std::uint32_t>::max()) {
                        return failure(
                            Elf32SymbolIndexError::SymbolCountExceeded);
                    }
                    result.index.symbol_count = symbol_index + 1U;
                    terminated = true;
                    break;
                }
                ++symbol_index;
            }
            if (!terminated) {
                return failure(
                    Elf32SymbolIndexError::UnterminatedGnuChain);
            }
        }

        result.index.gnu_hash = Elf32GnuHashIndex{
            .bucket_count = bucket_count,
            .symbol_offset = symbol_offset,
            .bloom_word_count = bloom_word_count,
            .bloom_shift = bloom_shift,
            .bloom_guest_address = bloom_address,
            .buckets_guest_address = buckets_address,
            .chains_guest_address = chains_address,
        };
    }

    if (result.index.symbol_count == 0) {
        return failure(Elf32SymbolIndexError::InvalidGnuHash);
    }
    if (result.index.symbol_count > options.max_symbols) {
        return failure(Elf32SymbolIndexError::SymbolCountExceeded);
    }

    const std::uint64_t symbol_bytes =
        static_cast<std::uint64_t>(result.index.symbol_count) *
        kElf32SymbolEntrySize;
    if (!range_fits(metadata.symbol_table->guest_address, symbol_bytes)) {
        return failure(Elf32SymbolIndexError::SymbolRangeOverflow);
    }
    if (!readable_range(memory, metadata.symbol_table->guest_address,
                        symbol_bytes)) {
        return failure(Elf32SymbolIndexError::SymbolReadFailed);
    }

    return result;
}

const char* to_string(Elf32SymbolIndexError error) noexcept {
    switch (error) {
    case Elf32SymbolIndexError::None: return "none";
    case Elf32SymbolIndexError::InvalidOptions: return "invalid_options";
    case Elf32SymbolIndexError::MissingStringTable: return "missing_string_table";
    case Elf32SymbolIndexError::MissingSymbolTable: return "missing_symbol_table";
    case Elf32SymbolIndexError::MissingHashTable: return "missing_hash_table";
    case Elf32SymbolIndexError::HashHeaderReadFailed: return "hash_header_read_failed";
    case Elf32SymbolIndexError::HashRangeOverflow: return "hash_range_overflow";
    case Elf32SymbolIndexError::HashReadFailed: return "hash_read_failed";
    case Elf32SymbolIndexError::InvalidSysvHash: return "invalid_sysv_hash";
    case Elf32SymbolIndexError::InvalidGnuHash: return "invalid_gnu_hash";
    case Elf32SymbolIndexError::HashLimitExceeded: return "hash_limit_exceeded";
    case Elf32SymbolIndexError::HashIndexOutOfRange: return "hash_index_out_of_range";
    case Elf32SymbolIndexError::UnterminatedGnuChain: return "unterminated_gnu_chain";
    case Elf32SymbolIndexError::SymbolCountExceeded: return "symbol_count_exceeded";
    case Elf32SymbolIndexError::SymbolRangeOverflow: return "symbol_range_overflow";
    case Elf32SymbolIndexError::SymbolReadFailed: return "symbol_read_failed";
    }
    return "unknown";
}

}  // namespace liba32android::elf
