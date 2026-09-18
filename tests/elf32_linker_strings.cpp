#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "elf/elf32_linker_strings.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32LinkerStringError;
using liba32android::elf::Elf32LinkerStringOptions;
using liba32android::elf::Elf32StringTableMetadata;
using liba32android::elf::read_elf32_string_table_entry;
using liba32android::memory::LinearGuestMemory;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

bool write_bytes(LinearGuestMemory& memory,
                 std::uint32_t address,
                 std::span<const std::uint8_t> bytes) {
    return memory.write(address, bytes);
}

int test_valid_and_empty_strings() {
    LinearGuestMemory memory(0x100, 0x1000);
    const std::array<std::uint8_t, 8> bytes{
        'f', 'o', 'o', 0, 0, 'x', 'y', 0,
    };
    if (!write_bytes(memory, 0x1010, bytes)) return fail("could not stage string table");

    const Elf32StringTableMetadata table{.guest_address = 0x1010, .size = 8};

    const auto ordinary = read_elf32_string_table_entry(
        memory, table, 0, Elf32LinkerStringOptions{.max_string_bytes = 3});
    if (!ordinary || ordinary.value != "foo") {
        return fail("valid string at exact payload limit was not materialized");
    }

    const auto empty = read_elf32_string_table_entry(
        memory, table, 4, Elf32LinkerStringOptions{.max_string_bytes = 0});
    if (!empty || !empty.value.empty()) {
        return fail("empty string was not accepted");
    }

    return 0;
}

int test_non_utf8_bytes_are_preserved() {
    LinearGuestMemory memory(0x20, 0x2000);
    const std::array<std::uint8_t, 3> bytes{0xffU, 0x80U, 0};
    if (!write_bytes(memory, 0x2004, bytes)) return fail("could not stage byte string");

    const Elf32StringTableMetadata table{.guest_address = 0x2004, .size = 3};
    const auto result = read_elf32_string_table_entry(
        memory, table, 0, Elf32LinkerStringOptions{.max_string_bytes = 2});
    if (!result || result.value.size() != 2 ||
        static_cast<std::uint8_t>(result.value[0]) != 0xffU ||
        static_cast<std::uint8_t>(result.value[1]) != 0x80U) {
        return fail("non-UTF-8 bytes were not preserved exactly");
    }
    return 0;
}

int test_offset_and_address_failures() {
    LinearGuestMemory memory(0x20, 0);

    const Elf32StringTableMetadata small_table{.guest_address = 0, .size = 4};
    if (read_elf32_string_table_entry(
            memory, small_table, 4,
            Elf32LinkerStringOptions{.max_string_bytes = 4}).error !=
        Elf32LinkerStringError::StringOffsetOutOfRange) {
        return fail("offset equal to STRSZ was not rejected");
    }

    const Elf32StringTableMetadata overflow_table{
        .guest_address = 0xffffffffU,
        .size = 2,
    };
    if (read_elf32_string_table_entry(
            memory, overflow_table, 0,
            Elf32LinkerStringOptions{.max_string_bytes = 1}).error !=
        Elf32LinkerStringError::AddressOverflow) {
        return fail("guest string range overflow was not rejected");
    }
    return 0;
}

int test_read_failure() {
    LinearGuestMemory memory(0x20, 0x1000);
    const Elf32StringTableMetadata table{.guest_address = 0x9000, .size = 4};
    if (read_elf32_string_table_entry(
            memory, table, 0,
            Elf32LinkerStringOptions{.max_string_bytes = 3}).error !=
        Elf32LinkerStringError::ReadFailed) {
        return fail("unreadable guest string was not rejected");
    }
    return 0;
}

int test_unterminated_and_limit_policy() {
    {
        LinearGuestMemory memory(0x20, 0x3000);
        const std::array<std::uint8_t, 3> bytes{'a', 'b', 'c'};
        if (!write_bytes(memory, 0x3000, bytes)) return fail("could not stage unterminated bytes");
        const Elf32StringTableMetadata table{.guest_address = 0x3000, .size = 3};
        if (read_elf32_string_table_entry(
                memory, table, 0,
                Elf32LinkerStringOptions{.max_string_bytes = 5}).error !=
            Elf32LinkerStringError::UnterminatedString) {
            return fail("table-end without NUL was not classified as unterminated");
        }
    }

    {
        LinearGuestMemory memory(0x20, 0x4000);
        const std::array<std::uint8_t, 4> bytes{'a', 'b', 'c', 0};
        if (!write_bytes(memory, 0x4000, bytes)) return fail("could not stage exact-limit bytes");
        const Elf32StringTableMetadata table{.guest_address = 0x4000, .size = 4};
        const auto result = read_elf32_string_table_entry(
            memory, table, 0,
            Elf32LinkerStringOptions{.max_string_bytes = 3});
        if (!result || result.value != "abc") {
            return fail("payload exactly at maximum followed by NUL did not succeed");
        }
    }

    {
        LinearGuestMemory memory(0x20, 0x5000);
        const std::array<std::uint8_t, 5> bytes{'a', 'b', 'c', 'd', 0};
        if (!write_bytes(memory, 0x5000, bytes)) return fail("could not stage over-limit bytes");
        const Elf32StringTableMetadata table{.guest_address = 0x5000, .size = 5};
        if (read_elf32_string_table_entry(
                memory, table, 0,
                Elf32LinkerStringOptions{.max_string_bytes = 3}).error !=
            Elf32LinkerStringError::StringTooLong) {
            return fail("first extra non-NUL payload byte was not rejected as too long");
        }
    }

    return 0;
}

int test_failure_does_not_mutate_guest_memory() {
    LinearGuestMemory memory(0x20, 0x6000);
    const std::array<std::uint8_t, 5> sentinel{'a', 'b', 'c', 'd', 0};
    if (!write_bytes(memory, 0x6000, sentinel)) return fail("could not stage mutation sentinel");

    const Elf32StringTableMetadata table{.guest_address = 0x6000, .size = 5};
    if (read_elf32_string_table_entry(
            memory, table, 0,
            Elf32LinkerStringOptions{.max_string_bytes = 2}).error !=
        Elf32LinkerStringError::StringTooLong) {
        return fail("mutation guard setup did not fail as expected");
    }

    std::array<std::uint8_t, 5> after{};
    if (!memory.read(0x6000, after) || after != sentinel) {
        return fail("failed string consumption mutated guest bytes");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_valid_and_empty_strings(); status != 0) return status;
    if (const int status = test_non_utf8_bytes_are_preserved(); status != 0) return status;
    if (const int status = test_offset_and_address_failures(); status != 0) return status;
    if (const int status = test_read_failure(); status != 0) return status;
    if (const int status = test_unterminated_and_limit_policy(); status != 0) return status;
    if (const int status = test_failure_does_not_mutate_guest_memory(); status != 0) return status;
    return 0;
}
