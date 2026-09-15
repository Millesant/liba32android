#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>

#include "cpu/a32_cpu.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::cpu::ExecutionRequest;
using liba32android::cpu::ExecutionResult;
using liba32android::memory::LinearGuestMemory;

bool load_code(LinearGuestMemory& memory, std::span<const std::uint8_t> code) {
    return memory.write(0, code);
}

bool execution_ok(const ExecutionResult& result, std::size_t expected_instructions) {
    return !result.exception_raised && !result.memory_fault &&
           result.instructions_executed == expected_instructions;
}

bool read_u32(const LinearGuestMemory& memory, std::uint32_t address, std::uint32_t& value) {
    std::array<std::uint8_t, 4> bytes{};
    if (!memory.read(address, bytes)) {
        return false;
    }

    value = static_cast<std::uint32_t>(bytes[0]) |
            static_cast<std::uint32_t>(bytes[1]) << 8 |
            static_cast<std::uint32_t>(bytes[2]) << 16 |
            static_cast<std::uint32_t>(bytes[3]) << 24;
    return true;
}

int run_register_state() {
    // add r2, r0, r1 (E0802001)
    constexpr std::array<std::uint8_t, 4> code{0x01, 0x20, 0x80, 0xE0};
    LinearGuestMemory memory{4096};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.regs[0] = 20;
    request.regs[1] = 22;
    const auto result = liba32android::cpu::execute(memory, request);
    if (!execution_ok(result, 1) || result.regs[2] != 42) {
        std::cerr << "register-state test failed: r2=" << result.regs[2] << '\n';
        return 1;
    }
    return 0;
}

int run_branch() {
    // b +0 -> target at 0x8; mov r0,#1 is skipped; target mov r0,#42.
    constexpr std::array<std::uint8_t, 12> code{
        0x00, 0x00, 0x00, 0xEA,
        0x01, 0x00, 0xA0, 0xE3,
        0x2A, 0x00, 0xA0, 0xE3,
    };
    LinearGuestMemory memory{4096};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);
    if (!execution_ok(result, 2) || result.regs[0] != 42) {
        std::cerr << "branch test failed: r0=" << result.regs[0] << '\n';
        return 1;
    }
    return 0;
}

int run_call() {
    // bl +0 -> target at 0x8; LR must receive the return address 0x4.
    constexpr std::array<std::uint8_t, 12> code{
        0x00, 0x00, 0x00, 0xEB,
        0x01, 0x00, 0xA0, 0xE3,
        0x2A, 0x00, 0xA0, 0xE3,
    };
    LinearGuestMemory memory{4096};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);
    if (!execution_ok(result, 2) || result.regs[0] != 42 || result.regs[14] != 4) {
        std::cerr << "call test failed: r0=" << result.regs[0] << " lr=" << result.regs[14] << '\n';
        return 1;
    }
    return 0;
}

int run_memory_load_store() {
    // str r0,[r1] (E5810000); ldr r2,[r1] (E5912000).
    constexpr std::array<std::uint8_t, 8> code{
        0x00, 0x00, 0x81, 0xE5,
        0x00, 0x20, 0x91, 0xE5,
    };
    LinearGuestMemory memory{4096};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.regs[0] = 0x12345678;
    request.regs[1] = 0x300;
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);

    std::uint32_t stored = 0;
    if (!read_u32(memory, 0x300, stored) || !execution_ok(result, 2) ||
        result.regs[2] != 0x12345678 || stored != 0x12345678) {
        std::cerr << "memory test failed: r2=0x" << std::hex << result.regs[2]
                  << " stored=0x" << stored << std::dec << '\n';
        return 1;
    }
    return 0;
}

int run_stack() {
    // str r0,[sp,#-4]! (E52D0004); ldr r1,[sp] (E59D1000).
    constexpr std::array<std::uint8_t, 8> code{
        0x04, 0x00, 0x2D, 0xE5,
        0x00, 0x10, 0x9D, 0xE5,
    };
    LinearGuestMemory memory{4096};
    if (!load_code(memory, code)) {
        return 1;
    }

    ExecutionRequest request{};
    request.regs[0] = 0xCAFEBABE;
    request.regs[13] = 0x800;
    request.instruction_count = 2;
    const auto result = liba32android::cpu::execute(memory, request);

    std::uint32_t stored = 0;
    if (!read_u32(memory, 0x7FC, stored) || !execution_ok(result, 2) ||
        result.regs[13] != 0x7FC || result.regs[1] != 0xCAFEBABE || stored != 0xCAFEBABE) {
        std::cerr << "stack test failed: sp=0x" << std::hex << result.regs[13]
                  << " r1=0x" << result.regs[1] << " stored=0x" << stored << std::dec << '\n';
        return 1;
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: cpu_execution <registers|branch|call|memory|stack>\n";
        return 2;
    }

    const std::string_view mode{argv[1]};
    if (mode == "registers") {
        return run_register_state();
    }
    if (mode == "branch") {
        return run_branch();
    }
    if (mode == "call") {
        return run_call();
    }
    if (mode == "memory") {
        return run_memory_load_store();
    }
    if (mode == "stack") {
        return run_stack();
    }

    std::cerr << "unknown mode: " << mode << '\n';
    return 2;
}
