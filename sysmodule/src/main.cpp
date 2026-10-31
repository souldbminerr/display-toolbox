#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <switch.hpp>
#include "di.h"

using namespace nx;

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

/* Virtual MMIO */

constexpr u64 kDsiPa = 0x54300000;
constexpr u64 kCarPa = 0x60006000;

constexpr u64 kDsiSz = 0x40000;
constexpr u64 kCarSz = 0x1000;

u64 *DSI, *CAR;
u64 DSI_SZ, CAR_SZ;

// Main program entrypoint
int main(int argc, char* argv[]) {
    /* Initialize logging */
    fileUtils::Initialize();

    /* Map all virtual MMIO */
    u64 dsiOut;
    Result rc = svc::QueryMemoryMapping(&dsiOut, &DSI_SZ, kDsiPa, kDsiSz);
    ASSERT_RESULT_OK(rc, "svc::QueryMemoryMapping");
    DSI = (u64*)dsiOut;

    u64 carOut;
    rc = svc::QueryMemoryMapping(&carOut, &CAR_SZ, kCarPa, kCarSz);
    ASSERT_RESULT_OK(rc, "svc::QueryMemoryMapping");
    CAR = (u64*)carOut;


    /* Main loop */
    for(;;) {
        /* Disable video mode */
        DSI[DSI_VIDEO_MODE_CONTROL] = true;
        svcSleepThread(20000000);

        /* Unlock Level 2 register */
        /* By convention, DSI sequence must be sent like this */
        /* Send 4x5A to Register E2 */
        DSI[DSI_WR_DATA] = 0x5A5A5AE2;
        DSI[DSI_WR_DATA] = 0x5A;

        /* Set DSI trigger to VIDEO*/
        DSI[DSI_TRIGGER] = DSI_TRIGGER_VIDEO;

        DSI[DSI_WR_DATA] = ((MIPI_DCS_PRIV_SM_SET_COLOR_MODE | (0x65 << 8)) << 8) | MIPI_DSI_DCS_SHORT_WRITE_PARAM; // 0x65A015
        DSI[DSI_TRIGGER] = DSI_TRIGGER_VIDEO;
        
        DSI[DSI_VIDEO_MODE_CONTROL] = false;
        svcSleepThread(200'000'00);

    }

    /* NOTE: There is no need to re-lock DSI registers. */
    fileUtils::Exit();
    return 0;
}