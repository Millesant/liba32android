#include <bit>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "elf/elf32_dynamic.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32DynamicError;
using liba32android::elf::Elf32DynamicSegment;
using liba32android::elf::parse_elf32_dynamic;
using liba32android::memory::LinearGuestMemory;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
}

void append_entry(std::vector<std::uint8_t>& bytes, std::uint32_t raw_tag, std::uint32_t value) {
    append_u32(bytes, raw_tag);
    append_u32(bytes, value);
}

int test_valid_and_unknown_tags() {
    LinearGuestMemory memory(0x100, 0x1000);
    std::vector<std::uint8_t> bytes;
    append_entry(bytes, 0x70000001U, 0x12345678U);
    append_entry(bytes, 0xffffffffU, 0x89abcdefU);
    append_entry(bytes, 0, 0);
    // Bytes after the first DT_NULL are padding, not logical entries.
    append_entry(bytes, 5, 0xfeedfaceU);
    if (!memory.write(0x1020, bytes)) {
        return fail("could not stage valid dynamic array");
    }

    const Elf32DynamicSegment segment{
        .guest_address = 0x1020,
        .file_size = static_cast<std::uint32_t>(bytes.size()),
        .memory_size = static_cast<std::uint32_t>(bytes.size()),
    };
    const auto result = parse_elf32_dynamic(memory, segment);
    if (!result || result.entries.size() != 3) {
        return fail("valid dynamic array was not parsed through the first DT_NULL");
    }
    if (result.entries[0].tag != static_cast<std::int32_t>(0x70000001U) ||
        result.entries[0].value != 0x12345678U) {
        return fail("ordinary unknown dynamic tag/value was not preserved");
    }
    if (result.entries[1].tag != std::bit_cast<std::int32_t>(0xffffffffU) ||
        result.entries[1].value != 0x89abcdefU) {
        return fail("signed unknown dynamic tag/value was not preserved");
    }
    if (result.entries[2].tag != 0 || result.entries[2].value != 0) {
        return fail("DT_NULL terminator was not preserved as the final logical entry");
    }
    return 0;
}

int test_truncated_entry() {
    LinearGuestMemory memory(0x40, 0x2000);
    const Elf32DynamicSegment segment{
        .guest_address = 0x2000,
        .file_size = 10,
        .memory_size = 16,
    };
    if (parse_elf32_dynamic(memory, segment).error != Elf32DynamicError::TruncatedEntry) {
        return fail("non-8-byte-aligned PT_DYNAMIC file range was not rejected");
    }
    return 0;
}

int test_unterminated_array() {
    LinearGuestMemory memory(0x40, 0x3000);
    std::vector<std::uint8_t> bytes;
    append_entry(bytes, 5, 0x11111111U);
    append_entry(bytes, 6, 0x22222222U);
    if (!memory.write(0x3000, bytes)) {
        return fail("could not stage unterminated dynamic array");
    }

    const Elf32DynamicSegment segment{
        .guest_address = 0x3000,
        .file_size = static_cast<std::uint32_t>(bytes.size()),
        .memory_size = static_cast<std::uint32_t>(bytes.size()),
    };
    if (parse_elf32_dynamic(memory, segment).error != Elf32DynamicError::Unterminated) {
        return fail("dynamic array without DT_NULL was not rejected");
    }
    return 0;
}

int test_invalid_range_and_read_failure() {
    LinearGuestMemory memory(0x20, 0x4000);
    const Elf32DynamicSegment invalid{
        .guest_address = 0x4000,
        .file_size = 16,
        .memory_size = 8,
    };
    if (parse_elf32_dynamic(memory, invalid).error != Elf32DynamicError::InvalidRange) {
        return fail("p_filesz > p_memsz dynamic range was not rejected defensively");
    }

    const Elf32DynamicSegment unreadable{
        .guest_address = 0x5000,
        .file_size = 8,
        .memory_size = 8,
    };
    if (parse_elf32_dynamic(memory, unreadable).error != Elf32DynamicError::ReadFailed) {
        return fail("unreadable dynamic guest bytes were not reported");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_valid_and_unknown_tags(); status != 0) return status;
    if (const int status = test_truncated_entry(); status != 0) return status;
    if (const int status = test_unterminated_array(); status != 0) return status;
    if (const int status = test_invalid_range_and_read_failure(); status != 0) return status;
    return 0;
}
