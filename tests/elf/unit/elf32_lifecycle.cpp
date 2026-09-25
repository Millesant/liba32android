#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "elf/elf32_lifecycle.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32FunctionArrayDecodeError;
using liba32android::elf::Elf32FunctionArrayDecodeOptions;
using liba32android::elf::Elf32FunctionArrayMetadata;
using liba32android::elf::decode_elf32_function_array;
using liba32android::memory::LinearGuestMemory;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
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

int test_exact_decode_and_raw_sentinels() {
    LinearGuestMemory memory(0x100, 0x1000);
    constexpr std::uint32_t address = 0x1010;
    if (!write_u32(memory, address, 0U) ||
        !write_u32(memory, address + 4U, 0xffffffffU) ||
        !write_u32(memory, address + 8U, 0x12345679U)) {
        return fail("could not stage lifecycle array");
    }

    std::array<std::uint8_t, 12> before{};
    if (!memory.read(address, before)) {
        return fail("could not snapshot lifecycle array");
    }

    const auto result = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{.guest_address = address, .size = 12},
        Elf32FunctionArrayDecodeOptions{.max_entries = 3});
    if (!result ||
        result.entries !=
            std::vector<std::uint32_t>{0U, 0xffffffffU, 0x12345679U}) {
        return fail("lifecycle array decode changed order or sentinel values");
    }

    std::array<std::uint8_t, 12> after{};
    if (!memory.read(address, after) || after != before) {
        return fail("lifecycle array decode mutated guest memory");
    }
    return 0;
}

int test_entry_ceiling_preflights_before_read() {
    LinearGuestMemory memory(0x100, 0x1000);
    const auto result = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{
            .guest_address = 0x90000000U,
            .size = 12,
        },
        Elf32FunctionArrayDecodeOptions{.max_entries = 2});
    if (result.error != Elf32FunctionArrayDecodeError::TooManyEntries ||
        !result.entries.empty()) {
        return fail("lifecycle entry ceiling did not fail before reading");
    }
    return 0;
}

int test_invalid_size_and_range() {
    LinearGuestMemory memory(0x100, 0x1000);

    const auto bad_size = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{.guest_address = 0x1000, .size = 6},
        Elf32FunctionArrayDecodeOptions{.max_entries = 2});
    if (bad_size.error != Elf32FunctionArrayDecodeError::InvalidArraySize) {
        return fail("non-integral lifecycle array size was not rejected");
    }

    const auto overflow = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{
            .guest_address = 0xfffffffcU,
            .size = 8,
        },
        Elf32FunctionArrayDecodeOptions{.max_entries = 2});
    if (overflow.error != Elf32FunctionArrayDecodeError::RangeOverflow) {
        return fail("lifecycle decoder range overflow was not rejected");
    }
    return 0;
}

int test_unreadable_and_empty_arrays() {
    LinearGuestMemory memory(0x100, 0x1000);

    const auto unreadable = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{.guest_address = 0x2000, .size = 4},
        Elf32FunctionArrayDecodeOptions{.max_entries = 1});
    if (unreadable.error != Elf32FunctionArrayDecodeError::ReadFailed ||
        !unreadable.entries.empty()) {
        return fail("unreadable lifecycle array was not rejected cleanly");
    }

    const auto empty = decode_elf32_function_array(
        memory,
        Elf32FunctionArrayMetadata{
            .guest_address = 0xffffffffU,
            .size = 0,
        },
        Elf32FunctionArrayDecodeOptions{.max_entries = 0});
    if (!empty || !empty.entries.empty()) {
        return fail("zero-length lifecycle array did not decode as empty");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_exact_decode_and_raw_sentinels(); status != 0) {
        return status;
    }
    if (const int status = test_entry_ceiling_preflights_before_read();
        status != 0) {
        return status;
    }
    if (const int status = test_invalid_size_and_range(); status != 0) {
        return status;
    }
    if (const int status = test_unreadable_and_empty_arrays(); status != 0) {
        return status;
    }
    return 0;
}
