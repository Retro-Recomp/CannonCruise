// ============================================================================
// CannonCruise - System services bootstrap (systemservices.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\startup\systemservices.cpp
// Reverse-engineered and rewritten by analyzing with CC_ida.asm.
//
// Binary Functions:
//   FUN_005ca6c0 — FileSystemInit      (startup init chain, "Step 3a")
//   FUN_00428de0 — TextureCacheInit    (startup init chain, "Step 3b")
//   FUN_004cac40 — SubSystemShutdown   (startup shutdown chain, +1A)
//   FUN_0040fd60 — ResourceShutdown    (startup shutdown chain, +2E)
//   FUN_00405670 — GeneralShutdown     (startup shutdown chain, +33)
//
// Note: The file's context is only known from the call XREFs in startup.cpp;
//      except for ResourceShutdown (assert path cattributehandler.cpp), none of them
//      has a file proof string.
// ============================================================================

#include "rwcore.h"
#include "attributehandler/cattributehandler.h"
#include "eventhandler/ceventhandler.h"
#include <cassert>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>       // timeGetDevCaps / timeBeginPeriod / timeGetTime
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#endif

namespace
{
    // dword_658944 — head of the RW subsystem device list (the binary, at the start of shutdown,
    // tests this first; if null it exits immediately). The device list
    // is released through the engine object (dword_65D7A4) vtable (+0x138h);
    // until the RW engine infrastructure is restored, it stays null.
    void* s_pSubSystemList = 0;
}

// ============================================================================
// FileSystemInit (FUN_005CA6C0)
// Binary Address: 0x005ca6c0
// XREF: sub_41A680 (startup init): 0041a7f8 — startup.cpp "Step 3a"
//       (called again during shutdown: 0041abb9)
//
// Resolution: In IDA this address is labeled as the 1-byte `nullsub_11` (`retn`)
// (the align gap between sub_5CA6A0 and sub_5CA6D0). The
// FUN_005ca6c0 CALL target in Ghidra is the same single byte. So the file system
// initialization CALL in the binary is a COMPLETE NO-OP — real setup is done by
// static initializers or at some other stage. An empty body is written to stay true to the binary.
// ============================================================================
void FileSystemInit(void)
{
    // Binary: `retn` (nullsub_11) — deliberate no-op.
}

// ============================================================================
// TextureCacheInit (FUN_00428DE0)
// Binary Address: 0x00428de0
// XREF: sub_41A680 (startup init): 0041a7fd — startup.cpp "Step 3b"
//
// Binary body (WINMM):
//   1. timeGetDevCaps(&tc, sizeof(TIMECAPS))
//   2. timeBeginPeriod(tc.wPeriodMin)     — raise the timer resolution
//   3. ESI = timeGetTime()                — start timestamp (return value)
//   4. timeEndPeriod(tc.wPeriodMin)
// The caller (startup) ignores the return value; still, like the binary, a time
// stamp is produced and TRUE is returned if it is non-zero.
// On non-Windows platforms WINMM does not exist → TRUE without reading the clock.
// Note (Phase 4.4b): The Windows build needs a winmm.lib link;
// (the pragma in this file handles it on MSVC).
// ============================================================================
RwBool TextureCacheInit(void)
{
#ifdef _WIN32
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(tc)) != TIMERR_NOERROR)
        return TRUE;                    // the binary would also continue on error
    timeBeginPeriod(tc.wPeriodMin);
    DWORD start = timeGetTime();
    timeEndPeriod(tc.wPeriodMin);
    return TRUE;                        // return value is ignored by the caller
#else
    return TRUE;
#endif
}

// ============================================================================
// SubSystemShutdown (FUN_004CAC40)
// Binary Address: 0x004cac40
// XREF: sub_41AB90 (startup shutdown): 0041ab..+1A — startup.cpp shutdown chain
//
// Binary body:
//   1. dword_658944 (device list head) is null → return immediately
//   2. if dword_65894C exists: copy the result of sub_50D4A0 into the object's +0x10h field
//      0x0B dwords, dword_65894C = 0; otherwise invoke the object [+0x3C]
//      callback (arg=3)
//   3. Across the linked list: each device's [+0x18h] callback, release of
//      the field through the engine vtable (+0x138h), release via
//      sub_4CBCE0(device, 2)
//   4. Finally release the list head object via the engine vtable (+0x138h)
// The device list is filled by the engine subsystem plugins (Phase 4.4b);
// until then the binary also silently returns with a null head.
// ============================================================================
void SubSystemShutdown(void)
{
    if (!s_pSubSystemList)
        return;

    // TODO(Phase 4.4b) — device list chain + engine vtable[+0x138h]
    // release operations: sub_50D4A0 copy, device [+0x18h] callback,
    // sub_4CBCE0(device, 2). This is walked here once the list is restored.
    s_pSubSystemList = 0;
}

// ============================================================================
// ResourceShutdown (FUN_0040FD60)
// Binary Address: 0x0040fd60
// XREF: sub_41AB90 (startup shutdown): 0041ab..+2E — startup.cpp shutdown chain
//
// The assert path string proves the file context:
//   "D:\Projects\CannonCruisePC\code\framework\core\attributehandler\c..."
//   "g_ptheMap->size() == 0 Failed" (cattributehandler.cpp, line 0x53)
//
// Binary body:
//   1. dword_635650 (g_ptheMap = CAttributeHandler instance) null → return
//   2. map size != 0 → assert "g_ptheMap->size() == 0 Failed"
//      (assert yoksa: "Assert failed:" + dword_63564C testi → _exit)
//   3. sub_411E20(instance) — dtor; sub_4053B0(instance) — delete
//   4. dword_635650 = 0
// The current counterpart of this contract is CAttributeHandler::Shutdown()
// (restored by reference in cattributehandler.cpp: FUN_0040fd60).
// ============================================================================
void ResourceShutdown(void)
{
    RWS::CAttributeHandler::Shutdown();
}

// ============================================================================
// GeneralShutdown (FUN_00405670)
// Binary Address: 0x00405670
// XREF: sub_41AB90 (startup shutdown): 0041ab..+33 — startup shutdown chain
//       (the LAST step of the startup shutdown)
//
// Binary body:
//   1. dword_635574 (general subscription/registration list — lazy-inited by sub_4055F0)
//      if not null: for each record, remove the subscription links via sub_4057A0
//      (unlink + release via sub_4053B0, counter-1)
//   2. sub_48B3E0(record) — bulk-remove all remaining links
//   3. sub_4053B0(record) — delete the object
//   4. dword_635574 = 0
//
// The closest existing counterpart: the event system's subscription records
// (s_RegisteredEvents + s_EventSubscribers) — CEventHandler::Shutdown()
// clears both. The full class identity of dword_635574 will be verified in Phase 4.4b.
// ============================================================================
void GeneralShutdown(void)
{
    RWS::CEventHandler::Shutdown();
}
