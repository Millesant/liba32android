#include "cpu/dynarmic_cpu.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A32/arch_version.h"
#include "dynarmic/interface/A32/config.h"

namespace liba32android::cpu {
namespace {

constexpr std::size_t kMemorySize = 4096;

class Environment final : public Dynarmic::A32::UserCallbacks {
public:
    explicit Environment(std::span<const std::uint8_t> code) {
        if (code.size() > memory_.size()) {
            throw std::invalid_argument("guest code does not fit M0 scratch memory");
        }
        std::copy(code.begin(), code.end(), memory_.begin());
    }

    std::uint8_t MemoryRead8(std::uint32_t vaddr) override {
        if (vaddr >= memory_.size()) {
            return 0;
        }
        return memory_[vaddr];
    }

    std::uint16_t MemoryRead16(std::uint32_t vaddr) override {
        return static_cast<std::uint16_t>(MemoryRead8(vaddr)) |
               static_cast<std::uint16_t>(MemoryRead8(vaddr + 1)) << 8;
    }

    std::uint32_t MemoryRead32(std::uint32_t vaddr) override {
        return static_cast<std::uint32_t>(MemoryRead16(vaddr)) |
               static_cast<std::uint32_t>(MemoryRead16(vaddr + 2)) << 16;
    }

    std::uint64_t MemoryRead64(std::uint32_t vaddr) override {
        return static_cast<std::uint64_t>(MemoryRead32(vaddr)) |
               static_cast<std::uint64_t>(MemoryRead32(vaddr + 4)) << 32;
    }

    void MemoryWrite8(std::uint32_t vaddr, std::uint8_t value) override {
        if (vaddr < memory_.size()) {
            memory_[vaddr] = value;
        }
    }

    void MemoryWrite16(std::uint32_t vaddr, std::uint16_t value) override {
        MemoryWrite8(vaddr, static_cast<std::uint8_t>(value));
        MemoryWrite8(vaddr + 1, static_cast<std::uint8_t>(value >> 8));
    }

    void MemoryWrite32(std::uint32_t vaddr, std::uint32_t value) override {
        MemoryWrite16(vaddr, static_cast<std::uint16_t>(value));
        MemoryWrite16(vaddr + 2, static_cast<std::uint16_t>(value >> 16));
    }

    void MemoryWrite64(std::uint32_t vaddr, std::uint64_t value) override {
        MemoryWrite32(vaddr, static_cast<std::uint32_t>(value));
        MemoryWrite32(vaddr + 4, static_cast<std::uint32_t>(value >> 32));
    }

    void InterpreterFallback(std::uint32_t, std::size_t) override {
        exception_raised_ = true;
    }

    void CallSVC(std::uint32_t) override {
        exception_raised_ = true;
    }

    void ExceptionRaised(std::uint32_t, Dynarmic::A32::Exception) override {
        exception_raised_ = true;
    }

    void AddTicks(std::uint64_t ticks) override {
        ticks_left_ = ticks >= ticks_left_ ? 0 : ticks_left_ - ticks;
    }

    std::uint64_t GetTicksRemaining() override {
        return ticks_left_;
    }

    [[nodiscard]] bool exception_raised() const noexcept {
        return exception_raised_;
    }

private:
    std::array<std::uint8_t, kMemorySize> memory_{};
    std::uint64_t ticks_left_ = 1;
    bool exception_raised_ = false;
};

}  // namespace

ExecutionResult execute_one(std::span<const std::uint8_t> code, InstructionSet instruction_set) {
    Environment environment{code};

    Dynarmic::A32::UserConfig config{};
    config.callbacks = &environment;
    config.arch_version = Dynarmic::A32::ArchVersion::v7;
    config.code_cache_size = 8 * 1024 * 1024;
    config.always_little_endian = true;

    Dynarmic::A32::Jit jit{config};
    jit.Regs().fill(0);
    jit.ExtRegs().fill(0);
    jit.Regs()[15] = 0;

    // User mode (0x10), plus Thumb state when requested.
    const std::uint32_t cpsr = instruction_set == InstructionSet::Thumb ? 0x30u : 0x10u;
    jit.SetCpsr(cpsr);
    static_cast<void>(jit.Step());

    return ExecutionResult{
        .regs = jit.Regs(),
        .cpsr = jit.Cpsr(),
        .exception_raised = environment.exception_raised(),
    };
}

}  // namespace liba32android::cpu
