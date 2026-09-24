#include "elf/elf32_dynamic.h"

#include "elf/internal/elf32_bytes.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace liba32android::elf {
namespace {

constexpr std::size_t kElf32DynamicEntrySize = 8;
constexpr std::int32_t kDynamicTagNull = 0;
constexpr std::uint64_t kGuestAddressSpaceSize = std::uint64_t{1} << 32;

[[nodiscard]] Elf32DynamicResult failure(Elf32DynamicError error) {
    Elf32DynamicResult result;
    result.error = error;
    return result;
}

}  // namespace

Elf32DynamicResult parse_elf32_dynamic(const memory::GuestMemory& memory,
                                       const Elf32DynamicSegment& segment) {
    if (segment.file_size > segment.memory_size ||
        static_cast<std::uint64_t>(segment.guest_address) + segment.memory_size >
            kGuestAddressSpaceSize) {
        return failure(Elf32DynamicError::InvalidRange);
    }
    if ((segment.file_size % kElf32DynamicEntrySize) != 0) {
        return failure(Elf32DynamicError::TruncatedEntry);
    }

    Elf32DynamicResult result;
    std::array<std::uint8_t, kElf32DynamicEntrySize> bytes{};
    for (std::uint64_t offset = 0; offset < segment.file_size;
         offset += kElf32DynamicEntrySize) {
        const std::uint64_t guest_address =
            static_cast<std::uint64_t>(segment.guest_address) + offset;
        if (!memory.read(static_cast<std::uint32_t>(guest_address), bytes)) {
            return failure(Elf32DynamicError::ReadFailed);
        }

        const std::uint32_t raw_tag = detail::decode_u32_le(bytes, 0);
        const Elf32DynamicEntry entry{
            .tag = std::bit_cast<std::int32_t>(raw_tag),
            .value = detail::decode_u32_le(bytes, 4),
        };
        result.entries.push_back(entry);
        if (entry.tag == kDynamicTagNull) {
            return result;
        }
    }

    return failure(Elf32DynamicError::Unterminated);
}

const char* to_string(Elf32DynamicError error) noexcept {
    switch (error) {
    case Elf32DynamicError::None: return "none";
    case Elf32DynamicError::InvalidRange: return "invalid_range";
    case Elf32DynamicError::TruncatedEntry: return "truncated_entry";
    case Elf32DynamicError::ReadFailed: return "read_failed";
    case Elf32DynamicError::Unterminated: return "unterminated";
    }
    return "unknown";
}

}  // namespace liba32android::elf
