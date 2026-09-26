liba32android_add_test_executable(
    runtime_service_dispatch_test
    tests/runtime/a32_service_dispatch.cpp
)

add_test(
    NAME a32_host_service_dispatch
    COMMAND runtime_service_dispatch_test
)
