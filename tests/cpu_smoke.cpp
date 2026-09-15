#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

#include "cpu/dynarmic_cpu.h"

namespace {

int run_arm() {
    // ARM little-endian: mov r0, #42  (E3A0002A)
    constexpr std::array<std::uint8_t, 4> code{0x2A, 0x00, 0xA0, 0xE3};
    const auto result = liba32android::cpu::execute_one(code, liba32android::cpu::InstructionSet::Arm);

    if (result.exception_raised || result.regs[0] != 42) {
        std::cerr << "ARM smoke failed: r0=" << result.regs[0]
                  << " exception=" << result.exception_raised << '\n';
        return 1;
    }

    std::cout << "ARM guest returned 42\n";
    return 0;
}

int run_thumb() {
    // Thumb little-endian: movs r0, #42 (202A). The extra halfword only pads
    // the 32-bit instruction fetch used by the translator.
    constexpr std::array<std::uint8_t, 4> code{0x2A, 0x20, 0x00, 0x00};
    const auto result = liba32android::cpu::execute_one(code, liba32android::cpu::InstructionSet::Thumb);

    if (result.exception_raised || result.regs[0] != 42) {
        std::cerr << "Thumb smoke failed: r0=" << result.regs[0]
                  << " exception=" << result.exception_raised << '\n';
        return 1;
    }

    std::cout << "Thumb guest returned 42\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: cpu_smoke <arm|thumb>\n";
        return 2;
    }

    const std::string_view mode{argv[1]};
    if (mode == "arm") {
        return run_arm();
    }
    if (mode == "thumb") {
        return run_thumb();
    }

    std::cerr << "unknown mode: " << mode << '\n';
    return 2;
}
