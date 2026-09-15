cmake_minimum_required(VERSION 3.24)
project(liba32android LANGUAGES C CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(LIBA32ANDROID_BUILD_TESTS "Build liba32android tests" ${BUILD_TESTING})

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
    src/memory/guest_memory.cpp
)

target_include_directories(liba32android
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(liba32android
    PRIVATE
        dynarmic
)

target_compile_features(liba32android PRIVATE cxx_std_20)

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

    add_test(NAME guest_arm_return_42 COMMAND cpu_smoke arm)
    add_test(NAME guest_thumb_return_42 COMMAND cpu_smoke thumb)
    add_test(NAME guest_register_state COMMAND cpu_execution registers)
    add_test(NAME guest_branch COMMAND cpu_execution branch)
    add_test(NAME guest_call COMMAND cpu_execution call)
    add_test(NAME guest_memory_load_store COMMAND cpu_execution memory)
    add_test(NAME guest_stack COMMAND cpu_execution stack)
    add_test(NAME guest_memory_bounds COMMAND guest_memory_test)
endif()
