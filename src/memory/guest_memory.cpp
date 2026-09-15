#include "memory/guest_memory.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace liba32android::memory {
namespace {

constexpr std::uint64_t kGuestAddressSpaceSize = std::uint64_t{1} << 32;

}  // namespace

LinearGuestMemory::LinearGuestMemory(std::size_t size, std::uint32_t base)
        : base_{base}, bytes_(size) {
    if (static_cast<std::uint64_t>(size) > kGuestAddressSpaceSize - static_cast<std::uint64_t>(base_)) {
        throw std::invalid_argument("linear guest memory exceeds the AArch32 address space");
    }
}

bool LinearGuestMemory::read(std::uint32_t address, std::span<std::uint8_t> output) const {
    std::size_t offset = 0;
    if (!translate(address, output.size(), offset)) {
        return false;
    }

    std::copy(bytes_.begin() + static_cast<std::ptrdiff_t>(offset),
              bytes_.begin() + static_cast<std::ptrdiff_t>(offset + output.size()), output.begin());
    return true;
}

bool LinearGuestMemory::write(std::uint32_t address, std::span<const std::uint8_t> input) {
    std::size_t offset = 0;
    if (!translate(address, input.size(), offset)) {
        return false;
    }

    std::copy(input.begin(), input.end(), bytes_.begin() + static_cast<std::ptrdiff_t>(offset));
    return true;
}

std::uint32_t LinearGuestMemory::base() const noexcept {
    return base_;
}

std::size_t LinearGuestMemory::size() const noexcept {
    return bytes_.size();
}

bool LinearGuestMemory::translate(std::uint32_t address, std::size_t length, std::size_t& offset) const noexcept {
    if (address < base_) {
        return false;
    }

    const std::uint64_t translated = static_cast<std::uint64_t>(address) - static_cast<std::uint64_t>(base_);
    if (translated > static_cast<std::uint64_t>(bytes_.size())) {
        return false;
    }

    const auto translated_size = static_cast<std::size_t>(translated);
    if (length > bytes_.size() - translated_size) {
        return false;
    }

    offset = translated_size;
    return true;
}

}  // namespace liba32android::memory
