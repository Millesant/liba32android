liba32android_add_test_executable(
    compat_android_log_write_test
    tests/compat/a32_android_log_write.cpp
)

add_test(
    NAME a32_android_log_write_service
    COMMAND compat_android_log_write_test
)
