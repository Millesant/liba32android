#include <sys/mman.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(__ANDROID__) || !defined(__aarch64__)
#error "android_address_space_probe must be built for Android arm64-v8a"
#endif

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

namespace {

constexpr std::uint64_t kFourGiB = std::uint64_t{1} << 32;
constexpr std::size_t kMaxPrintedLowMappings = 64;

int anonymous_flags() {
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
    flags |= MAP_NORESERVE;
#endif
    return flags;
}

void print_errno(const char* key, int error) {
    std::printf("%s.errno=%d\n", key, error);
    std::printf("%s.error=%s\n", key, std::strerror(error));
}

void print_environment(long page_size) {
#ifdef __ANDROID_API__
    std::printf("android.api=%d\n", __ANDROID_API__);
#else
    std::printf("android.api=unknown\n");
#endif
    std::printf("arch=aarch64\n");
    std::printf("page_size=%ld\n", page_size);

    utsname info{};
    if (uname(&info) == 0) {
        std::printf("kernel.sysname=%s\n", info.sysname);
        std::printf("kernel.release=%s\n", info.release);
        std::printf("kernel.version=%s\n", info.version);
        std::printf("kernel.machine=%s\n", info.machine);
    } else {
        const int error = errno;
        std::printf("kernel.status=unavailable\n");
        print_errno("kernel", error);
    }
}

void print_mmap_min_addr() {
    errno = 0;
    FILE* file = std::fopen("/proc/sys/vm/mmap_min_addr", "r");
    if (file == nullptr) {
        const int error = errno;
        std::printf("mmap_min_addr.status=unavailable\n");
        print_errno("mmap_min_addr", error);
        return;
    }

    unsigned long long value = 0;
    if (std::fscanf(file, "%llu", &value) == 1) {
        std::printf("mmap_min_addr.status=ok\n");
        std::printf("mmap_min_addr.value=%llu\n", value);
    } else {
        std::printf("mmap_min_addr.status=parse_error\n");
    }
    std::fclose(file);
}

void print_low_mappings() {
    errno = 0;
    FILE* file = std::fopen("/proc/self/maps", "r");
    if (file == nullptr) {
        const int error = errno;
        std::printf("low_maps.status=unavailable\n");
        print_errno("low_maps", error);
        return;
    }

    char line[768]{};
    std::size_t count = 0;
    std::size_t printed = 0;
    while (std::fgets(line, sizeof(line), file) != nullptr) {
        unsigned long long start = 0;
        unsigned long long end = 0;
        char permissions[5]{};
        if (std::sscanf(line, "%llx-%llx %4s", &start, &end, permissions) != 3) {
            continue;
        }
        if (start >= kFourGiB) {
            continue;
        }

        ++count;
        if (printed < kMaxPrintedLowMappings) {
            const auto clipped_end = end > kFourGiB ? kFourGiB : end;
            std::printf("low_map.%zu=0x%llx-0x%llx,%s\n", printed, start, clipped_end, permissions);
            ++printed;
        }
    }
    std::fclose(file);

    std::printf("low_maps.status=ok\n");
    std::printf("low_maps.count=%zu\n", count);
    std::printf("low_maps.printed=%zu\n", printed);
    std::printf("low_maps.truncated=%s\n", count > printed ? "true" : "false");
}

void probe_fastmem_reservation(std::size_t page_size) {
    const std::size_t length = static_cast<std::size_t>(kFourGiB);
    errno = 0;
    void* mapping = mmap(nullptr, length, PROT_NONE, anonymous_flags(), -1, 0);
    if (mapping == MAP_FAILED) {
        const int error = errno;
        std::printf("fastmem_4g.status=failed\n");
        print_errno("fastmem_4g", error);
        return;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(mapping);
    std::printf("fastmem_4g.status=reserved\n");
    std::printf("fastmem_4g.base=0x%" PRIxPTR "\n", base);
    std::printf("fastmem_4g.end=0x%" PRIx64 "\n", static_cast<std::uint64_t>(base) + kFourGiB);
    std::printf("fastmem_4g.base_below_4g=%s\n", static_cast<std::uint64_t>(base) < kFourGiB ? "true" : "false");

    auto* bytes = static_cast<unsigned char*>(mapping);
    void* sample = bytes + 16 * page_size;
    errno = 0;
    if (mprotect(sample, page_size, PROT_READ | PROT_WRITE) == 0) {
        *static_cast<volatile unsigned char*>(sample) = 0x5a;
        const bool value_ok = *static_cast<volatile unsigned char*>(sample) == 0x5a;
        std::printf("fastmem_4g.commit_page=%s\n", value_ok ? "success" : "value_mismatch");
        if (mprotect(sample, page_size, PROT_NONE) != 0) {
            print_errno("fastmem_4g.reset_page", errno);
        }
    } else {
        const int error = errno;
        std::printf("fastmem_4g.commit_page=failed\n");
        print_errno("fastmem_4g.commit_page", error);
    }

    if (munmap(mapping, length) == 0) {
        std::printf("fastmem_4g.unmap=success\n");
    } else {
        const int error = errno;
        std::printf("fastmem_4g.unmap=failed\n");
        print_errno("fastmem_4g.unmap", error);
    }
}

void probe_fixed_noreplace(std::size_t page_size) {
    constexpr std::array<std::uintptr_t, 6> candidates{
        0x00010000u,
        0x00100000u,
        0x01000000u,
        0x10000000u,
        0x40000000u,
        0x80000000u,
    };

    const int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        void* requested = reinterpret_cast<void*>(candidates[index]);
        errno = 0;
        void* mapping = mmap(requested, page_size, PROT_NONE, flags, -1, 0);
        if (mapping == MAP_FAILED) {
            const int error = errno;
            std::printf("fixed_noreplace.%zu.request=0x%" PRIxPTR "\n", index, candidates[index]);
            std::printf("fixed_noreplace.%zu.status=failed\n", index);
            std::printf("fixed_noreplace.%zu.errno=%d\n", index, error);
            std::printf("fixed_noreplace.%zu.error=%s\n", index, std::strerror(error));
            continue;
        }

        const auto actual = reinterpret_cast<std::uintptr_t>(mapping);
        std::printf("fixed_noreplace.%zu.request=0x%" PRIxPTR "\n", index, candidates[index]);
        std::printf("fixed_noreplace.%zu.actual=0x%" PRIxPTR "\n", index, actual);
        if (mapping != requested) {
            std::printf("fixed_noreplace.%zu.status=relocated_flag_not_enforced\n", index);
            munmap(mapping, page_size);
            continue;
        }

        std::printf("fixed_noreplace.%zu.status=exact\n", index);
        errno = 0;
        void* collision = mmap(requested, page_size, PROT_NONE, flags, -1, 0);
        if (collision == MAP_FAILED) {
            const int collision_error = errno;
            std::printf("fixed_noreplace.%zu.collision_errno=%d\n", index, collision_error);
            std::printf("fixed_noreplace.%zu.collision=%s\n", index,
                        collision_error == EEXIST ? "EEXIST" : "failed_other");
        } else {
            const auto collision_actual = reinterpret_cast<std::uintptr_t>(collision);
            std::printf("fixed_noreplace.%zu.collision=unexpected_mapping\n", index);
            std::printf("fixed_noreplace.%zu.collision_actual=0x%" PRIxPTR "\n", index, collision_actual);
            munmap(collision, page_size);
        }
        munmap(mapping, page_size);
    }
}

bool probe_jit_wx(std::size_t page_size, bool execute_generated_code) {
    errno = 0;
    void* mapping = mmap(nullptr, page_size, PROT_READ | PROT_WRITE, anonymous_flags(), -1, 0);
    if (mapping == MAP_FAILED) {
        const int error = errno;
        std::printf("jit_wx.map=failed\n");
        print_errno("jit_wx.map", error);
        std::printf("jit_wx.execute=NOT_RUN\n");
        return false;
    }

    // AArch64: mov w0, #42; ret
    constexpr std::array<std::uint32_t, 2> code{0x52800540u, 0xd65f03c0u};
    std::memcpy(mapping, code.data(), sizeof(code));
    auto* begin = static_cast<char*>(mapping);
    __builtin___clear_cache(begin, begin + sizeof(code));

    errno = 0;
    if (mprotect(mapping, page_size, PROT_READ | PROT_EXEC) != 0) {
        const int error = errno;
        std::printf("jit_wx.mprotect_rw_to_rx=failed\n");
        print_errno("jit_wx.mprotect_rw_to_rx", error);
        std::printf("jit_wx.execute=NOT_RUN\n");
        munmap(mapping, page_size);
        return false;
    }
    std::printf("jit_wx.mprotect_rw_to_rx=success\n");

    bool execution_ok = true;
    if (execute_generated_code) {
        std::fflush(stdout);
        using GeneratedFunction = int (*)();
        const auto function = reinterpret_cast<GeneratedFunction>(mapping);
        const int result = function();
        std::printf("jit_wx.execute=RUN\n");
        std::printf("jit_wx.result=%d\n", result);
        execution_ok = result == 42;
        std::printf("jit_wx.result_check=%s\n", execution_ok ? "PASS" : "FAIL");
    } else {
        std::printf("jit_wx.execute=NOT_RUN\n");
    }

    if (munmap(mapping, page_size) != 0) {
        print_errno("jit_wx.unmap", errno);
    }
    return execution_ok;
}

}  // namespace

int main(int argc, char** argv) {
    bool execute_generated_code = false;
    if (argc == 2 && std::strcmp(argv[1], "--execute-generated-code") == 0) {
        execute_generated_code = true;
    } else if (argc != 1) {
        std::fprintf(stderr, "usage: android_address_space_probe [--execute-generated-code]\n");
        return 2;
    }

    static_assert(sizeof(std::size_t) >= 8, "4 GiB probe requires a 64-bit process");

    const long page_size_raw = sysconf(_SC_PAGESIZE);
    if (page_size_raw <= 0) {
        std::fprintf(stderr, "sysconf(_SC_PAGESIZE) failed\n");
        return 2;
    }
    const auto page_size = static_cast<std::size_t>(page_size_raw);

    std::printf("probe.version=1\n");
    print_environment(page_size_raw);
    print_mmap_min_addr();
    print_low_mappings();
    probe_fastmem_reservation(page_size);
    probe_fixed_noreplace(page_size);
    const bool execution_ok = probe_jit_wx(page_size, execute_generated_code);
    std::printf("probe.complete=true\n");

    return execute_generated_code && !execution_ok ? 1 : 0;
}
