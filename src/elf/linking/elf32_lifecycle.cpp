#include "elf/elf32_lifecycle.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace liba32android::elf {
namespace {

[[nodiscard]] Elf32FunctionArrayDecodeResult failure(
    Elf32FunctionArrayDecodeError error) {
    Elf32FunctionArrayDecodeResult result;
    result.error = error;
    return result;
}

}  // namespace

Elf32FunctionArrayDecodeResult decode_elf32_function_array(
    const memory::GuestMemory& memory,
    const Elf32FunctionArrayMetadata& array,
    const Elf32FunctionArrayDecodeOptions& options) {
    constexpr std::uint32_t kEntrySize = 4;
    constexpr std::uint64_t kGuestAddressSpaceSize = std::uint64_t{1} << 32;

    if ((array.size % kEntrySize) != 0) {
        return failure(Elf32FunctionArrayDecodeError::InvalidArraySize);
    }

    const std::uint32_t entry_count = array.size / kEntrySize;
    if (entry_count > options.max_entries) {
        return failure(Elf32FunctionArrayDecodeError::TooManyEntries);
    }

    if (static_cast<std::uint64_t>(array.guest_address) +
            static_cast<std::uint64_t>(array.size) >
        kGuestAddressSpaceSize) {
        return failure(Elf32FunctionArrayDecodeError::RangeOverflow);
    }

    Elf32FunctionArrayDecodeResult result;
    result.entries.reserve(entry_count);
    for (std::uint32_t index = 0; index < entry_count; ++index) {
        const std::uint64_t address =
            static_cast<std::uint64_t>(array.guest_address) +
            static_cast<std::uint64_t>(index) * kEntrySize;
        if (address > std::numeric_limits<std::uint32_t>::max()) {
            return failure(Elf32FunctionArrayDecodeError::RangeOverflow);
        }

        std::array<std::uint8_t, kEntrySize> bytes{};
        if (!memory.read(static_cast<std::uint32_t>(address), bytes)) {
            return failure(Elf32FunctionArrayDecodeError::ReadFailed);
        }
        result.entries.push_back(
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8U) |
            (static_cast<std::uint32_t>(bytes[2]) << 16U) |
            (static_cast<std::uint32_t>(bytes[3]) << 24U));
    }
    return result;
}

const char* to_string(Elf32FunctionArrayDecodeError error) noexcept {
    switch (error) {
    case Elf32FunctionArrayDecodeError::None: return "none";
    case Elf32FunctionArrayDecodeError::InvalidArraySize:
        return "invalid_array_size";
    case Elf32FunctionArrayDecodeError::RangeOverflow:
        return "range_overflow";
    case Elf32FunctionArrayDecodeError::TooManyEntries:
        return "too_many_entries";
    case Elf32FunctionArrayDecodeError::ReadFailed:
        return "read_failed";
    }
    return "unknown";
}

}  // namespace liba32android::elf
