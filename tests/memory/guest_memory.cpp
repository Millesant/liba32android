#include <array>
#include <cstdint>
#include <iostream>
#include <span>

#include "memory/guest_memory.h"

int main() {
    liba32android::memory::LinearGuestMemory memory{16, 0x1000};

    constexpr std::array<std::uint8_t, 4> value{0x78, 0x56, 0x34, 0x12};
    if (!memory.write(0x100C, value)) {
        std::cerr << "expected in-range write to succeed\n";
        return 1;
    }

    std::array<std::uint8_t, 4> readback{};
    if (!memory.read(0x100C, readback) || readback != value) {
        std::cerr << "in-range readback mismatch\n";
        return 1;
    }

    if (memory.write(0x100E, value)) {
        std::cerr << "cross-boundary write unexpectedly succeeded\n";
        return 1;
    }

    if (memory.read(0x0FFF, readback)) {
        std::cerr << "read before guest base unexpectedly succeeded\n";
        return 1;
    }

    if (memory.base() != 0x1000 || memory.size() != 16) {
        std::cerr << "guest memory metadata mismatch\n";
        return 1;
    }

    return 0;
}
