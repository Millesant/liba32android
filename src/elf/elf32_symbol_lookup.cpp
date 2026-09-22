#include "elf/elf32_symbol_lookup.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <span>
#include <vector>

namespace liba32android::elf {
namespace {

constexpr std::uint32_t kElf32SymbolEntrySize = 16;
constexpr std::uint32_t kElf32WordBits = 32;
constexpr std::uint8_t kStbLocal = 0;
constexpr std::uint8_t kStbGlobal = 1;
constexpr std::uint8_t kStbWeak = 2;
constexpr std::uint8_t kSttNotype = 0;
constexpr std::uint8_t kSttObject = 1;
constexpr std::uint8_t kSttFunc = 2;
constexpr std::uint8_t kSttGnuIfunc = 10;
constexpr std::uint8_t kStvDefault = 0;
constexpr std::uint8_t kStvInternal = 1;
constexpr std::uint8_t kStvHidden = 2;
constexpr std::uint8_t kStvProtected = 3;
constexpr std::uint16_t kShnUndef = 0;
constexpr std::uint16_t kShnLoReserve = 0xff00;
constexpr std::uint16_t kShnAbs = 0xfff1;
constexpr std::uint16_t kShnCommon = 0xfff2;
constexpr std::uint16_t kShnXindex = 0xffff;
constexpr std::uint64_t kGuestAddressSpaceSize = std::uint64_t{1} << 32;
constexpr std::size_t kReadValidationChunkSize = 256;

[[nodiscard]] Elf32SymbolIndexResult failure(Elf32SymbolIndexError error) {
    Elf32SymbolIndexResult result;
    result.error = error;
    return result;
}

[[nodiscard]] Elf32ObjectSymbolLookupResult lookup_failure(
    Elf32SymbolLookupError error,
    Elf32LinkerStringError string_error = Elf32LinkerStringError::None) {
    Elf32ObjectSymbolLookupResult result;
    result.error = error;
    result.string_error = string_error;
    return result;
}

[[nodiscard]] Elf32GraphSymbolLookupResult graph_failure(
    Elf32GraphSymbolLookupError error,
    std::optional<std::size_t> failing_object = std::nullopt) {
    Elf32GraphSymbolLookupResult result;
    result.error = error;
    result.failing_object = failing_object;
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

[[nodiscard]] std::uint16_t decode_u16_le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint16_t>(bytes[0]) |
           (static_cast<std::uint16_t>(bytes[1]) << 8U);
}

[[nodiscard]] std::uint32_t decode_u32_le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

[[nodiscard]] std::uint32_t sysv_hash(std::string_view name) noexcept {
    std::uint32_t hash = 0;
    for (const unsigned char byte : name) {
        hash = (hash << 4U) + byte;
        const std::uint32_t high = hash & 0xf0000000U;
        if (high != 0) {
            hash ^= high >> 24U;
            hash &= ~high;
        }
    }
    return hash;
}

[[nodiscard]] std::uint32_t gnu_hash(std::string_view name) noexcept {
    std::uint32_t hash = 5381U;
    for (const unsigned char byte : name) {
        hash = hash * 33U + byte;
    }
    return hash;
}

[[nodiscard]] bool read_symbol(const memory::GuestMemory& memory,
                               const Elf32SymbolTableMetadata& table,
                               std::uint32_t symbol_index,
                               Elf32Symbol& symbol) {
    std::uint32_t address = 0;
    if (!checked_add(table.guest_address,
                     static_cast<std::uint64_t>(symbol_index) *
                         kElf32SymbolEntrySize,
                     address)) {
        return false;
    }

    std::array<std::uint8_t, kElf32SymbolEntrySize> bytes{};
    if (!memory.read(address, bytes)) return false;

    symbol.name_offset = decode_u32_le(bytes.data());
    symbol.value = decode_u32_le(bytes.data() + 4);
    symbol.size = decode_u32_le(bytes.data() + 8);
    symbol.binding = static_cast<std::uint8_t>(bytes[12] >> 4U);
    symbol.type = static_cast<std::uint8_t>(bytes[12] & 0x0fU);
    symbol.visibility = static_cast<std::uint8_t>(bytes[13] & 0x03U);
    symbol.section_index = decode_u16_le(bytes.data() + 14);
    return true;
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
            !is_power_of_two(bloom_word_count) ||
            bloom_shift >= kElf32WordBits) {
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

Elf32ObjectSymbolLookupResult lookup_elf32_symbol(
    const memory::GuestMemory& memory,
    std::uint32_t load_bias,
    const Elf32LinkerMetadata& metadata,
    const Elf32SymbolIndex& index,
    std::string_view name,
    const Elf32SymbolLookupOptions& options) {
    if (options.max_name_bytes == 0) {
        return lookup_failure(Elf32SymbolLookupError::InvalidOptions);
    }
    if (name.empty()) {
        return lookup_failure(Elf32SymbolLookupError::InvalidLookupName);
    }
    if (!metadata.string_table.has_value() ||
        !metadata.symbol_table.has_value() ||
        index.symbol_count == 0 ||
        (!index.gnu_hash.has_value() && !index.sysv_hash.has_value())) {
        return lookup_failure(Elf32SymbolLookupError::InvalidMetadata);
    }
    if (metadata.has_symbol_versioning) {
        return lookup_failure(Elf32SymbolLookupError::UnsupportedVersioning);
    }

    const auto evaluate_candidate =
        [&](std::uint32_t symbol_index)
            -> std::optional<Elf32ObjectSymbolLookupResult> {
        if (symbol_index == 0 || symbol_index >= index.symbol_count) {
            return lookup_failure(
                Elf32SymbolLookupError::HashIndexOutOfRange);
        }

        Elf32Symbol symbol;
        if (!read_symbol(memory, *metadata.symbol_table, symbol_index,
                         symbol)) {
            return lookup_failure(Elf32SymbolLookupError::SymbolReadFailed);
        }

        const auto symbol_name = read_elf32_string_table_entry(
            memory, *metadata.string_table, symbol.name_offset,
            Elf32LinkerStringOptions{
                .max_string_bytes = options.max_name_bytes,
            });
        if (!symbol_name) {
            return lookup_failure(Elf32SymbolLookupError::StringReadFailed,
                                  symbol_name.error);
        }
        if (symbol_name.value != name) {
            return std::nullopt;
        }

        if (symbol.binding == kStbLocal ||
            symbol.section_index == kShnUndef ||
            symbol.visibility == kStvInternal ||
            symbol.visibility == kStvHidden) {
            return std::nullopt;
        }
        if (symbol.binding != kStbGlobal && symbol.binding != kStbWeak) {
            return lookup_failure(Elf32SymbolLookupError::UnsupportedBinding);
        }

        // Only the low two visibility bits are defined for this feature.
        std::array<std::uint8_t, kElf32SymbolEntrySize> raw{};
        std::uint32_t raw_address = 0;
        if (!checked_add(metadata.symbol_table->guest_address,
                         static_cast<std::uint64_t>(symbol_index) *
                             kElf32SymbolEntrySize,
                         raw_address) ||
            !memory.read(raw_address, raw)) {
            return lookup_failure(Elf32SymbolLookupError::SymbolReadFailed);
        }
        if ((raw[13] & 0xfcU) != 0 ||
            (symbol.visibility != kStvDefault &&
             symbol.visibility != kStvProtected)) {
            return lookup_failure(
                Elf32SymbolLookupError::UnsupportedVisibility);
        }

        if (symbol.section_index == kShnCommon ||
            symbol.section_index == kShnXindex ||
            (symbol.section_index >= kShnLoReserve &&
             symbol.section_index != kShnAbs)) {
            return lookup_failure(
                Elf32SymbolLookupError::UnsupportedSectionIndex);
        }

        if (symbol.type != kSttNotype &&
            symbol.type != kSttObject &&
            symbol.type != kSttFunc) {
            return lookup_failure(Elf32SymbolLookupError::UnsupportedType);
        }
        if (symbol.type == kSttGnuIfunc) {
            return lookup_failure(Elf32SymbolLookupError::UnsupportedType);
        }

        std::uint32_t guest_value = symbol.value;
        if (symbol.section_index != kShnAbs) {
            if (!checked_add(load_bias, symbol.value, guest_value)) {
                return lookup_failure(Elf32SymbolLookupError::ValueOverflow);
            }
        }

        Elf32ObjectSymbolLookupResult result;
        result.symbol.symbol_index = symbol_index;
        result.symbol.name = symbol_name.value;
        result.symbol.symbol = symbol;
        result.symbol.guest_value = guest_value;
        return result;
    };

    if (index.gnu_hash.has_value()) {
        const Elf32GnuHashIndex& hash_index = *index.gnu_hash;
        const std::uint32_t hash = gnu_hash(name);

        std::uint32_t bloom_word = 0;
        const std::uint32_t bloom_index =
            (hash / kElf32WordBits) &
            (hash_index.bloom_word_count - 1U);
        if (!read_indexed_word(memory, hash_index.bloom_guest_address,
                               bloom_index, bloom_word)) {
            return lookup_failure(Elf32SymbolLookupError::HashReadFailed);
        }
        const std::uint32_t first_bit =
            1U << (hash % kElf32WordBits);
        const std::uint32_t second_bit =
            1U << ((hash >> hash_index.bloom_shift) % kElf32WordBits);
        if ((bloom_word & first_bit) == 0 ||
            (bloom_word & second_bit) == 0) {
            return lookup_failure(Elf32SymbolLookupError::SymbolNotFound);
        }

        std::uint32_t symbol_index = 0;
        if (!read_indexed_word(
                memory, hash_index.buckets_guest_address,
                hash % hash_index.bucket_count, symbol_index)) {
            return lookup_failure(Elf32SymbolLookupError::HashReadFailed);
        }
        if (symbol_index == 0) {
            return lookup_failure(Elf32SymbolLookupError::SymbolNotFound);
        }
        if (symbol_index < hash_index.symbol_offset ||
            symbol_index >= index.symbol_count) {
            return lookup_failure(
                Elf32SymbolLookupError::HashIndexOutOfRange);
        }

        while (symbol_index < index.symbol_count) {
            const std::uint32_t chain_index =
                symbol_index - hash_index.symbol_offset;
            std::uint32_t chain_hash = 0;
            if (!read_indexed_word(memory, hash_index.chains_guest_address,
                                   chain_index, chain_hash)) {
                return lookup_failure(Elf32SymbolLookupError::HashReadFailed);
            }

            if ((chain_hash | 1U) == (hash | 1U)) {
                if (auto candidate = evaluate_candidate(symbol_index)) {
                    return *candidate;
                }
            }

            if ((chain_hash & 1U) != 0) {
                return lookup_failure(Elf32SymbolLookupError::SymbolNotFound);
            }
            ++symbol_index;
        }

        return lookup_failure(Elf32SymbolLookupError::InvalidHashChain);
    }

    const Elf32SysvHashIndex& hash_index = *index.sysv_hash;
    const std::uint32_t hash = sysv_hash(name);
    std::uint32_t symbol_index = 0;
    if (!read_indexed_word(memory, hash_index.buckets_guest_address,
                           hash % hash_index.bucket_count, symbol_index)) {
        return lookup_failure(Elf32SymbolLookupError::HashReadFailed);
    }
    if (symbol_index >= index.symbol_count && symbol_index != 0) {
        return lookup_failure(Elf32SymbolLookupError::HashIndexOutOfRange);
    }

    std::uint32_t hops = 0;
    while (symbol_index != 0 && hops < index.symbol_count) {
        if (auto candidate = evaluate_candidate(symbol_index)) {
            return *candidate;
        }

        std::uint32_t next = 0;
        if (!read_indexed_word(memory, hash_index.chains_guest_address,
                               symbol_index, next)) {
            return lookup_failure(Elf32SymbolLookupError::HashReadFailed);
        }
        if (next >= index.symbol_count && next != 0) {
            return lookup_failure(
                Elf32SymbolLookupError::HashIndexOutOfRange);
        }
        symbol_index = next;
        ++hops;
    }
    if (symbol_index != 0) {
        return lookup_failure(Elf32SymbolLookupError::InvalidHashChain);
    }
    return lookup_failure(Elf32SymbolLookupError::SymbolNotFound);
}

Elf32GraphSymbolLookupResult lookup_elf32_graph_symbol(
    const memory::GuestMemory& memory,
    const Elf32DependencyGraph& graph,
    std::size_t start_object,
    std::string_view name,
    const Elf32SymbolLookupOptions& options) {
    if (options.max_scope_objects == 0) {
        return graph_failure(Elf32GraphSymbolLookupError::InvalidOptions);
    }
    if (start_object >= graph.objects.size()) {
        return graph_failure(
            Elf32GraphSymbolLookupError::InvalidGraphStart, start_object);
    }

    std::vector<std::uint8_t> visited(graph.objects.size(), 0);
    std::deque<std::size_t> pending;
    pending.push_back(start_object);
    std::uint32_t scope_objects = 0;

    while (!pending.empty()) {
        const std::size_t object_index = pending.front();
        pending.pop_front();

        if (object_index >= graph.objects.size()) {
            return graph_failure(
                Elf32GraphSymbolLookupError::InvalidGraphEdge, object_index);
        }
        if (visited[object_index] != 0) continue;
        if (scope_objects >= options.max_scope_objects) {
            return graph_failure(
                Elf32GraphSymbolLookupError::ScopeLimitExceeded,
                object_index);
        }

        visited[object_index] = 1;
        ++scope_objects;
        const Elf32LoadedDependencyObject& object =
            graph.objects[object_index];

        if (object.linker_metadata.symbol_table.has_value()) {
            const Elf32SymbolIndexResult index = build_elf32_symbol_index(
                memory, object.linker_metadata, options);
            if (!index) {
                auto result = graph_failure(
                    Elf32GraphSymbolLookupError::IndexBuildFailed,
                    object_index);
                result.index_error = index.error;
                return result;
            }

            const Elf32ObjectSymbolLookupResult lookup =
                lookup_elf32_symbol(
                    memory, object.load.load_bias,
                    object.linker_metadata, index.index, name, options);
            if (lookup) {
                Elf32GraphSymbolLookupResult result;
                result.symbol.object_index = object_index;
                result.symbol.symbol = lookup.symbol;
                return result;
            }
            if (lookup.error != Elf32SymbolLookupError::SymbolNotFound) {
                auto result = graph_failure(
                    Elf32GraphSymbolLookupError::ObjectLookupFailed,
                    object_index);
                result.lookup_error = lookup.error;
                result.string_error = lookup.string_error;
                return result;
            }
        }

        for (const Elf32DependencyEdge& edge : object.dependencies) {
            if (edge.target_object >= graph.objects.size()) {
                return graph_failure(
                    Elf32GraphSymbolLookupError::InvalidGraphEdge,
                    object_index);
            }
            if (visited[edge.target_object] == 0) {
                pending.push_back(edge.target_object);
            }
        }
    }

    return graph_failure(Elf32GraphSymbolLookupError::SymbolNotFound);
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

const char* to_string(Elf32SymbolLookupError error) noexcept {
    switch (error) {
    case Elf32SymbolLookupError::None: return "none";
    case Elf32SymbolLookupError::InvalidOptions: return "invalid_options";
    case Elf32SymbolLookupError::InvalidMetadata: return "invalid_metadata";
    case Elf32SymbolLookupError::InvalidLookupName: return "invalid_lookup_name";
    case Elf32SymbolLookupError::UnsupportedVersioning: return "unsupported_versioning";
    case Elf32SymbolLookupError::HashReadFailed: return "hash_read_failed";
    case Elf32SymbolLookupError::HashIndexOutOfRange: return "hash_index_out_of_range";
    case Elf32SymbolLookupError::InvalidHashChain: return "invalid_hash_chain";
    case Elf32SymbolLookupError::SymbolReadFailed: return "symbol_read_failed";
    case Elf32SymbolLookupError::StringReadFailed: return "string_read_failed";
    case Elf32SymbolLookupError::SymbolNotFound: return "symbol_not_found";
    case Elf32SymbolLookupError::UnsupportedBinding: return "unsupported_binding";
    case Elf32SymbolLookupError::UnsupportedType: return "unsupported_type";
    case Elf32SymbolLookupError::UnsupportedVisibility: return "unsupported_visibility";
    case Elf32SymbolLookupError::UnsupportedSectionIndex: return "unsupported_section_index";
    case Elf32SymbolLookupError::ValueOverflow: return "value_overflow";
    }
    return "unknown";
}

const char* to_string(Elf32GraphSymbolLookupError error) noexcept {
    switch (error) {
    case Elf32GraphSymbolLookupError::None: return "none";
    case Elf32GraphSymbolLookupError::InvalidOptions: return "invalid_options";
    case Elf32GraphSymbolLookupError::InvalidGraphStart: return "invalid_graph_start";
    case Elf32GraphSymbolLookupError::InvalidGraphEdge: return "invalid_graph_edge";
    case Elf32GraphSymbolLookupError::ScopeLimitExceeded: return "scope_limit_exceeded";
    case Elf32GraphSymbolLookupError::IndexBuildFailed: return "index_build_failed";
    case Elf32GraphSymbolLookupError::ObjectLookupFailed: return "object_lookup_failed";
    case Elf32GraphSymbolLookupError::SymbolNotFound: return "symbol_not_found";
    }
    return "unknown";
}

}  // namespace liba32android::elf
