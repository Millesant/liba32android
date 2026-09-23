cmake_minimum_required(VERSION 3.24)
project(liba32android LANGUAGES C CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

function(liba32android_enable_android_16k_elf_alignment target)
    if(ANDROID)
        target_link_options(${target}
            PRIVATE
                "-Wl,-z,max-page-size=16384"
                "-Wl,-z,common-page-size=16384"
        )
    endif()
endfunction()

option(LIBA32ANDROID_BUILD_TESTS "Build liba32android tests" ${BUILD_TESTING})
option(LIBA32ANDROID_BUILD_ANDROID_PROBES "Build Android-only diagnostic probes" OFF)
set(LIBA32ANDROID_ARM32_FIXTURE_PATH "" CACHE FILEPATH
    "Optional generated ARM32 Android ET_DYN fixture used by the real ELF32 loader integration test")

# Keep Dynarmic narrowly scoped to the A32 frontend needed by this runtime.
set(DYNARMIC_FRONTENDS "A32" CACHE STRING "" FORCE)
set(DYNARMIC_TESTS OFF CACHE BOOL "" FORCE)
set(DYNARMIC_TESTS_USE_UNICORN OFF CACHE BOOL "" FORCE)
set(DYNARMIC_USE_LLVM OFF CACHE BOOL "" FORCE)
set(DYNARMIC_USE_BUNDLED_EXTERNALS ON CACHE BOOL "" FORCE)
set(DYNARMIC_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

# Pinned current Azahar-maintained Dynarmic commit (2026-06-24).
FetchContent_Declare(
    dynarmic
    GIT_REPOSITORY https://github.com/azahar-emu/dynarmic.git
    GIT_TAG e77b1ba0b7da7cbe93021b01a663acfe7c4dd516
    GIT_SHALLOW FALSE
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(dynarmic)

add_library(liba32android SHARED
    src/cpu/dynarmic_cpu.cpp
    src/elf/elf32_dependency_loader.cpp
    src/elf/elf32_dependency_resolver.cpp
    src/elf/elf32_dynamic.cpp
    src/elf/elf32_dynamic_placement.cpp
    src/elf/elf32_linker_metadata.cpp
    src/elf/elf32_linker_strings.cpp
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

if(LIBA32ANDROID_BUILD_ANDROID_PROBES)
    if(NOT ANDROID)
        message(FATAL_ERROR "LIBA32ANDROID_BUILD_ANDROID_PROBES requires an Android toolchain")
    endif()
    if(NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a" AND
       NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        message(FATAL_ERROR "Android address-space probe requires arm64-v8a or x86_64")
    endif()

    add_executable(android_address_space_probe tools/android_address_space_probe.cpp)
    target_compile_features(android_address_space_probe PRIVATE cxx_std_20)
    liba32android_enable_android_16k_elf_alignment(android_address_space_probe)

    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        add_executable(android_runtime_smoke tools/android_runtime_smoke.cpp)
        target_include_directories(android_runtime_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        target_link_libraries(android_runtime_smoke PRIVATE liba32android)
        target_compile_features(android_runtime_smoke PRIVATE cxx_std_20)
        liba32android_enable_android_16k_elf_alignment(android_runtime_smoke)
    endif()
endif()

if(LIBA32ANDROID_BUILD_TESTS)
    add_executable(cpu_smoke tests/cpu_smoke.cpp)
    target_include_directories(cpu_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(cpu_smoke PRIVATE liba32android)

    add_executable(cpu_execution tests/cpu_execution.cpp)
    target_include_directories(cpu_execution PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(cpu_execution PRIVATE liba32android)

    add_executable(guest_memory_test tests/guest_memory.cpp)
    target_include_directories(guest_memory_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(guest_memory_test PRIVATE liba32android)

    add_executable(mapped_guest_memory_test tests/mapped_guest_memory.cpp)
    target_include_directories(mapped_guest_memory_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(mapped_guest_memory_test PRIVATE liba32android)

    add_executable(guest_va_allocator_test tests/guest_va_allocator.cpp)
    target_include_directories(guest_va_allocator_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(guest_va_allocator_test PRIVATE liba32android)

    add_executable(cpu_fastmem_test tests/cpu_fastmem.cpp)
    target_include_directories(cpu_fastmem_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(cpu_fastmem_test PRIVATE liba32android)

    add_executable(elf32_loader_test tests/elf32_loader.cpp)
    target_include_directories(elf32_loader_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_loader_test PRIVATE liba32android)

    add_executable(elf32_load_plan_test tests/elf32_load_plan.cpp)
    target_include_directories(elf32_load_plan_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_load_plan_test PRIVATE liba32android)

    add_executable(elf32_dynamic_test tests/elf32_dynamic.cpp)
    target_include_directories(elf32_dynamic_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dynamic_test PRIVATE liba32android)

    add_executable(elf32_dynamic_placement_test tests/elf32_dynamic_placement.cpp)
    target_include_directories(elf32_dynamic_placement_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dynamic_placement_test PRIVATE liba32android)

    add_executable(elf32_linker_metadata_test tests/elf32_linker_metadata.cpp)
    target_include_directories(elf32_linker_metadata_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_linker_metadata_test PRIVATE liba32android)

    add_executable(elf32_linker_strings_test tests/elf32_linker_strings.cpp)
    target_include_directories(elf32_linker_strings_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_linker_strings_test PRIVATE liba32android)

    add_executable(elf32_symbol_lookup_test tests/elf32_symbol_lookup.cpp)
    target_include_directories(elf32_symbol_lookup_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_symbol_lookup_test PRIVATE liba32android)

    add_executable(elf32_dependency_resolver_test tests/elf32_dependency_resolver.cpp)
    target_include_directories(elf32_dependency_resolver_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dependency_resolver_test PRIVATE liba32android)

    add_executable(elf32_dependency_loader_test tests/elf32_dependency_loader.cpp)
    target_include_directories(elf32_dependency_loader_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dependency_loader_test PRIVATE liba32android)

    add_executable(elf32_real_fixture_test tests/elf32_real_fixture.cpp)
    target_include_directories(elf32_real_fixture_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_dynamic_real_fixture_test tests/elf32_dynamic_real_fixture.cpp)
    target_include_directories(elf32_dynamic_real_fixture_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dynamic_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_linker_metadata_real_fixture_test
                   tests/elf32_linker_metadata_real_fixture.cpp)
    target_include_directories(elf32_linker_metadata_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_linker_metadata_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_linker_strings_real_fixture_test
                   tests/elf32_linker_strings_real_fixture.cpp)
    target_include_directories(elf32_linker_strings_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_linker_strings_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_dependency_resolver_real_fixture_test
                   tests/elf32_dependency_resolver_real_fixture.cpp)
    target_include_directories(elf32_dependency_resolver_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dependency_resolver_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_dynamic_placement_real_fixture_test
                   tests/elf32_dynamic_placement_real_fixture.cpp)
    target_include_directories(elf32_dynamic_placement_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dynamic_placement_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_dependency_loader_real_fixture_test
                   tests/elf32_dependency_loader_real_fixture.cpp)
    target_include_directories(elf32_dependency_loader_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_dependency_loader_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_symbol_lookup_real_fixture_test
                   tests/elf32_symbol_lookup_real_fixture.cpp)
    target_include_directories(elf32_symbol_lookup_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_symbol_lookup_real_fixture_test PRIVATE liba32android)

    add_test(NAME guest_arm_return_42 COMMAND cpu_smoke arm)
    add_test(NAME guest_thumb_return_42 COMMAND cpu_smoke thumb)
    add_test(NAME guest_register_state COMMAND cpu_execution registers)
    add_test(NAME guest_branch COMMAND cpu_execution branch)
    add_test(NAME guest_call COMMAND cpu_execution call)
    add_test(NAME guest_memory_load_store COMMAND cpu_execution memory)
    add_test(NAME guest_stack COMMAND cpu_execution stack)
    add_test(NAME guest_thumb_branch COMMAND cpu_execution thumb_branch)
    add_test(NAME guest_thumb_call COMMAND cpu_execution thumb_call)
    add_test(NAME guest_thumb_memory_load_store COMMAND cpu_execution thumb_memory)
    add_test(NAME guest_thumb_stack COMMAND cpu_execution thumb_stack)
    add_test(NAME guest_thumb_svc_exception COMMAND cpu_execution thumb_svc)
    add_test(NAME guest_instruction_fetch_fault COMMAND cpu_execution fetch_fault)
    add_test(NAME guest_thumb_data_fault COMMAND cpu_execution thumb_data_fault)
    add_test(NAME guest_memory_bounds COMMAND guest_memory_test)
    add_test(NAME mapped_guest_memory_lifecycle COMMAND mapped_guest_memory_test)
    add_test(NAME guest_va_free_range_search COMMAND guest_va_allocator_test)
    add_test(NAME dynarmic_fastmem_and_fallback COMMAND cpu_fastmem_test)
    add_test(NAME elf32_valid_dynamic_load COMMAND elf32_loader_test valid_dynamic)
    add_test(NAME elf32_valid_exec_load COMMAND elf32_loader_test valid_exec)
    add_test(NAME elf32_dynamic_metadata COMMAND elf32_loader_test dynamic_metadata)
    add_test(NAME elf32_header_validation COMMAND elf32_loader_test headers)
    add_test(NAME elf32_program_header_bounds COMMAND elf32_loader_test ph_bounds)
    add_test(NAME elf32_segment_validation COMMAND elf32_loader_test segments)
    add_test(NAME elf32_address_conflict COMMAND elf32_loader_test address_conflict)
    add_test(NAME elf32_load_plan COMMAND elf32_load_plan_test)
    add_test(NAME elf32_dynamic_array COMMAND elf32_dynamic_test)
    add_test(NAME elf32_dynamic_placement COMMAND elf32_dynamic_placement_test)
    add_test(NAME elf32_linker_metadata_collection COMMAND elf32_linker_metadata_test)
    add_test(NAME elf32_linker_string_entry COMMAND elf32_linker_strings_test)
    add_test(NAME elf32_symbol_index COMMAND elf32_symbol_lookup_test)
    add_test(NAME elf32_dependency_resolution COMMAND elf32_dependency_resolver_test)
    add_test(NAME elf32_dependency_loading COMMAND elf32_dependency_loader_test)

    if(LIBA32ANDROID_ARM32_FIXTURE_PATH)
        if(NOT EXISTS "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
            message(FATAL_ERROR
                "LIBA32ANDROID_ARM32_FIXTURE_PATH does not exist: ${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        endif()
        add_test(NAME elf32_real_arm32_fixture
                 COMMAND elf32_real_fixture_test "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_dynamic_array
                 COMMAND elf32_dynamic_real_fixture_test "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_linker_metadata
                 COMMAND elf32_linker_metadata_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_linker_strings
                 COMMAND elf32_linker_strings_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_dependency_resolution
                 COMMAND elf32_dependency_resolver_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_dynamic_placement
                 COMMAND elf32_dynamic_placement_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_dependency_loading
                 COMMAND elf32_dependency_loader_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_symbol_lookup
                 COMMAND elf32_symbol_lookup_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
    endif()
endif()
