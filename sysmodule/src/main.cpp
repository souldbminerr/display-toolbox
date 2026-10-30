#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

/* Useful helpers for logging */
#include "file_utils.hpp"
#include "errors.hpp"

#define INNER_HEAP_SIZE 0x20000

#ifdef __cplusplus
extern "C" {
#endif

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 1;

// Ty to MasaGratoR for this!
// This is done to save some space as they have no practical use in our case
void *__real___cxa_throw(void *thrown_exception, void *pvar, void (*dest)(void *));
void *__real__Unwind_Resume();
void *__real___gxx_personality_v0();

void __wrap___cxa_throw(void *thrown_exception, void *pvar, void (*dest)(void *)) {
    abort();
}

void __wrap__Unwind_Resume() {
    return;
}

void __wrap___gxx_personality_v0() {
    return;
}

void __libnx_initheap(void) {
    static u8 inner_heap[INNER_HEAP_SIZE];
    extern void* fake_heap_start;
    extern void* fake_heap_end;

    // Configure the newlib heap.
    fake_heap_start = inner_heap;
    fake_heap_end   = inner_heap + sizeof(inner_heap);
}

// Service initialization.
void __appInit(void) {
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    rc = fsInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    fsdevMountSdmc();
}

// Service deinitialization.
void __appExit(void) {
    fsdevUnmountAll();
    fsExit();
    smExit();
}

#ifdef __cplusplus
}
#endif

// Main program entrypoint
int main(int argc, char* argv[]) {
    return 0;
}