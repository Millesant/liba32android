add_library(liba32android SHARED
    src/cpu/dynarmic_cpu.cpp
    src/elf/elf32_dependency_loader.cpp
    src/elf/elf32_dependency_resolver.cpp
    src/elf/elf32_dynamic.cpp
    src/elf/elf32_dynamic_placement.cpp
    src/elf/elf32_linker_metadata.cpp
    src/elf/elf32_linker_strings.cpp
    src/elf/elf32_relocation.cpp
    src/elf/elf32_relro.cpp
    src/elf/elf32_symbol_lookup.cpp
    src/elf/elf32_load_plan.cpp
    src/elf/elf32_loader.cpp
    src/memory/guest_memory.cpp
    src/memory/guest_va_allocator.cpp
)

# CMake automatically prefixes shared libraries with "lib" on Android/Linux.
# Keep the internal target name stable while producing exactly liba32android.so.
set_target_properties(liba32android PROPERTIES OUTPUT_NAME a32android)

target_include_directories(liba32android
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(liba32android
    PRIVATE
        dynarmic
)

target_compile_features(liba32android PRIVATE cxx_std_20)
liba32android_enable_android_16k_elf_alignment(liba32android)
