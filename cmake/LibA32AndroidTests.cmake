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

    add_executable(elf32_relocation_test tests/elf32_relocation.cpp)
    target_include_directories(elf32_relocation_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_relocation_test PRIVATE liba32android)

    add_executable(elf32_relocation_apply_test tests/elf32_relocation_apply.cpp)
    target_include_directories(elf32_relocation_apply_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_relocation_apply_test PRIVATE liba32android)

    add_executable(elf32_relro_test tests/elf32_relro.cpp)
    target_include_directories(elf32_relro_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_relro_test PRIVATE liba32android)

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

    add_executable(elf32_relocation_real_fixture_test
                   tests/elf32_relocation_real_fixture.cpp)
    target_include_directories(elf32_relocation_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_relocation_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_relocation_apply_real_fixture_test
                   tests/elf32_relocation_apply_real_fixture.cpp)
    target_include_directories(elf32_relocation_apply_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_relocation_apply_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_relro_real_fixture_test
                   tests/elf32_relro_real_fixture.cpp)
    target_include_directories(elf32_relro_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_relro_real_fixture_test PRIVATE liba32android)

    add_executable(elf32_jump_slot_real_fixture_test
                   tests/elf32_jump_slot_real_fixture.cpp)
    target_include_directories(elf32_jump_slot_real_fixture_test
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(elf32_jump_slot_real_fixture_test PRIVATE liba32android)

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
    add_test(NAME elf32_relro_metadata COMMAND elf32_loader_test relro_metadata)
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
    add_test(NAME elf32_relocation_plan COMMAND elf32_relocation_test)
    add_test(NAME elf32_relocation_apply COMMAND elf32_relocation_apply_test)
    add_test(NAME elf32_relro_seal COMMAND elf32_relro_test)
    add_test(NAME elf32_dependency_resolution COMMAND elf32_dependency_resolver_test)
    add_test(NAME elf32_dependency_loading COMMAND elf32_dependency_loader_test)

    if((LIBA32ANDROID_ARM32_JUMP_SLOT_CONSUMER_PATH AND
        NOT LIBA32ANDROID_ARM32_JUMP_SLOT_PROVIDER_PATH) OR
       (LIBA32ANDROID_ARM32_JUMP_SLOT_PROVIDER_PATH AND
        NOT LIBA32ANDROID_ARM32_JUMP_SLOT_CONSUMER_PATH))
        message(FATAL_ERROR
            "Both LIBA32ANDROID_ARM32_JUMP_SLOT_CONSUMER_PATH and LIBA32ANDROID_ARM32_JUMP_SLOT_PROVIDER_PATH must be supplied together")
    endif()

    if(LIBA32ANDROID_ARM32_JUMP_SLOT_CONSUMER_PATH AND
       LIBA32ANDROID_ARM32_JUMP_SLOT_PROVIDER_PATH)
        if(NOT EXISTS "${LIBA32ANDROID_ARM32_JUMP_SLOT_CONSUMER_PATH}")
            message(FATAL_ERROR
                "JUMP_SLOT consumer fixture does not exist: ${LIBA32ANDROID_ARM32_JUMP_SLOT_CONSUMER_PATH}")
        endif()
        if(NOT EXISTS "${LIBA32ANDROID_ARM32_JUMP_SLOT_PROVIDER_PATH}")
            message(FATAL_ERROR
                "JUMP_SLOT provider fixture does not exist: ${LIBA32ANDROID_ARM32_JUMP_SLOT_PROVIDER_PATH}")
        endif()
        add_test(NAME elf32_real_jump_slot_apply
                 COMMAND elf32_jump_slot_real_fixture_test
                         "${LIBA32ANDROID_ARM32_JUMP_SLOT_CONSUMER_PATH}"
                         "${LIBA32ANDROID_ARM32_JUMP_SLOT_PROVIDER_PATH}")
    endif()

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
        add_test(NAME elf32_real_relocation_plan
                 COMMAND elf32_relocation_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_relocation_apply
                 COMMAND elf32_relocation_apply_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
        add_test(NAME elf32_real_relro_seal
                 COMMAND elf32_relro_real_fixture_test
                         "${LIBA32ANDROID_ARM32_FIXTURE_PATH}")
    endif()
endif()
