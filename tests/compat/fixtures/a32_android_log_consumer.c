// Freestanding ARMv7/Android consumer for the feature-027 liblog shim.
// Linking against the generated liblog.so must create one DT_NEEDED edge and
// an R_ARM_JUMP_SLOT reference for __android_log_write.

__attribute__((visibility("default")))
int __android_log_write(int priority, const char* tag, const char* text);

__attribute__((visibility("default"), noinline))
int fixture_android_log_write(const char* tag, const char* text) {
    return __android_log_write(4, tag, text);
}
