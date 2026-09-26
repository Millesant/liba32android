#include "compat/a32_android_log_shim.h"

.syntax unified
.arm

.text
.p2align 2
.global __android_log_write
.type __android_log_write, %function
__android_log_write:
    svc #LIBA32ANDROID_A32_ANDROID_LOG_WRITE_SHIM_SVC
    bx lr
.size __android_log_write, .-__android_log_write

.section .note.GNU-stack,"",%progbits
