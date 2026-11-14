// ============================================================================
// CannonCruise - Render Pipeline bootstrap (renderpipeline.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\startup\renderpipeline.cpp
// Reverse-engineered and rewritten by analyzing with CC_ida.asm.
//
// Binary Functions:
//   FUN_0041c190 — RenderPipelineInit    (startup init chain, "Step 14")
//   FUN_0041c270 — RenderPipelineShutdown (startup shutdown chain, +6)
//   FUN_0041a640 — SetLightAmbient       (startup init chain, "Step 13")
//
// Note: The file's context is only known from the call in startup.cpp.
//      The full sub-system call chain in the init body is listed below
//      with addresses; missing targets await restoration in Phase 4.4b
//      (because the relevant sub-systems are not in the codebase).
// ============================================================================

#include "rwcore.h"
#include "eventhandler/ceventhandler.h"
#include "attributehandler/Allocation Policies/clinearmemorymanager.h"
#include "FontSystem.h"
#include "RenderPipeline.h"

namespace
{
    // dword_63600C — "iMsgStartSystem" registered event id (registered by RenderPipelineInit,
    //                  deleted by RenderPipelineShutdown — binary: loc_40BF90 / loc_40CC60)
    RWS::CEventId s_iMsgStartSystem = 0;

    // dword_636004 — "iMsgStopSystem" registered event id
    RWS::CEventId s_iMsgStopSystem = 0;

    // dword_635608 — "iMsgDeleteEventHandler" (sub_40BF10 kaydeder,
    //                  deletes it via sub_40BF60)
    RWS::CEventId s_iMsgDeleteEventHandler = 0;

    // unk_635600 — "iMsgDeleteEntity"
    RWS::CEventId s_iMsgDeleteEntity = 0;

    // dword_6355F8 — "iMsgOnDeleteEntity" (format: "CEventHandler*")
    RWS::CEventId s_iMsgOnDeleteEntity = 0;

    // dword_635644 — pipeline notification/timer callback
    // Binary init sonunda: mov dword_635644, offset sub_41BFA0
    // In binary shutdown: mov dword_635644, 0
    // Because the body of FUN_0041BFA0 has not been restored, a placeholder is assigned (Phase 4.4b).
    typedef void (*PipelineCallback)();
    void PipelineCallbackStub()
    {
        // TODO(Phase 4.4b): FUN_0041BFA0 — pipeline callback body
    }
    PipelineCallback s_pPipelineCallback = 0;
}

// ============================================================================
// RenderPipelineInit (FUN_0041C190)
// Binary Address: 0x0041c190
// XREF: sub_41A680 (startup init): 0041ab5d — startup.cpp "Step 14"
// Arguments: arg_0 = width, arg_4 = height, arg_8 = flags (startup extern)
//
// Binary call chain (all in the original order):
//   1. loc_429360(width, height, flags) → sub_41CE00(result)
//      — render target / framebuffer structure
//   2. sub_41CDF0() → sub_4CC140(x) → sub_4D6220()
//   3. sub_40BF10(0, 0) — framework message registrations:
//        "iMsgDeleteEventHandler" (unk_635608)
//        "iMsgDeleteEntity"       (unk_635600)
//        "iMsgOnDeleteEntity"     (dword_6355F8, format "CEventHandler*")
//   4. sub_41C650, sub_40F5C0, sub_414E60 (= CLinearMemoryManager::Delete)
//   5. dword_635644 = sub_41BFA0 (tick callback)
//   6. sub_40F880, sub_408360→sub_40A070, sub_407DD0, sub_4082D0,
//      sub_429C50, sub_41BA60, sub_41BB90, sub_41B770, sub_41CE10,
//      nullsub_11, sub_4D9E40, sub_41CEB0 ×2, nullsub_11, sub_41C0E0
//   7. loc_40BF90(dword_63600C, "iMsgStartSystem", 0)
//      loc_40BF90(dword_636004, "iMsgStopSystem",  0)
//
// Restoration: steps that can be modeled with the current API (3, 4, 5, 7) are executed;
// missing sub-system targets are kept as TODO(Phase 4.4b).
// ============================================================================
void RenderPipelineInit(RwUInt32 width, RwUInt32 height, RwUInt32 flags)
{
    (void)width; (void)height; (void)flags;

    // Step 2 — sub_41CDF0() → sub_4CC140(x) → sub_4D6220():
    //   the sub_4CC140 chain in original order: sub_4D5450, sub_4D0E70,
    //   sub_4CEFD0 (=FontSystemAllocBuffers), sub_4CD660 (=FontSystemInit),
    //   sub_4D41C0, sub_4CC1F0, sub_4CC020.
    //   sub_4D5450/4D0E70/4D41C0 engine stream registrations (sub_50dbf0 +
    //   dword_65d7a4 vtable allocation) — pool allocation covers this in the port,
    //   registration is a no-op → TODO(Phase 4.4b engine). Portable state
    //   (sub_4CC1F0) was restored with RenderPipeline_Reset; sub_4CC020
    //   requires a camera object → the aspect ratio stays 1.0 in Reset (same as
    //   the binary without a camera).
    FontSystemAllocBuffers(); // sub_4CEFD0 — glyph vertex buffer (0x2400)
    FontSystemInit();         // sub_4CD660 — path + quad-table
    RenderPipeline_Reset();   // sub_4CC1F0 — matrix stack + Im2D state

    // Step 3 — framework messages (binary: sub_40BF10(0, 0)):
    //   "iMsgDeleteEventHandler" / "iMsgDeleteEntity" / "iMsgOnDeleteEntity"
    //   (the last one with format "CEventHandler*"). The sub_40E1E0(0) and loc_40E9E0(0)
    //   pre-calls require a sub-system → TODO.
    s_iMsgDeleteEventHandler =
        RWS::CEventHandler::RegisterMsg("iMsgDeleteEventHandler");
    s_iMsgDeleteEntity =
        RWS::CEventHandler::RegisterMsg("iMsgDeleteEntity");
    s_iMsgOnDeleteEntity =
        RWS::CEventHandler::RegisterMsg("iMsgOnDeleteEntity", "CEventHandler*");

    // Step 4 — CLinearMemoryManager::Delete (binary: sub_414E60)
    // Resets the linear memory pools at pipeline startup.
    RWS::CLinearMemoryManager::Delete();

    // Step 5 — tick callback (binary: dword_635644 = sub_41BFA0)
    s_pPipelineCallback = &PipelineCallbackStub;

    // Step 7 — pipeline messages (binary: loc_40BF90)
    s_iMsgStartSystem = RWS::CEventHandler::RegisterMsg("iMsgStartSystem");
    s_iMsgStopSystem  = RWS::CEventHandler::RegisterMsg("iMsgStopSystem");

    // TODO(Phase 4.4b) — steps 1, 2, 6 sub-system chain:
    //   loc_429360 / sub_41CE00 / sub_41CDF0 / sub_4D6220
    //   motor members of sub_4CC140: sub_4D5450 / sub_4D0E70 / sub_4D41C0
    //   (stream registrations + freelist); camera context ported via the portable path
    //   (RenderPipeline_SetViewWindowFromCamera / BuildViewMatrix) —
    //   vtable hook (sub_4CC110) stays on the engine side —
    //   (sub_4CC1F0 portable reset = RenderPipeline_Reset above)
    //   (pre-calls of sub_40BF10: sub_40E1E0 / loc_40E9E0)
    //   sub_41C650 / sub_40F5C0 / sub_40F880 / sub_408360→sub_40A070 /
    //   sub_407DD0 / sub_4082D0 / sub_429C50 / sub_41BA60 / sub_41BB90 /
    //   sub_41B770 / sub_41CE10 / sub_4D9E40 / sub_41CEB0 / sub_41C0E0
}

// ============================================================================
// RenderPipelineShutdown (FUN_0041C270)
// Binary Address: 0x0041c270
// XREF: sub_41AB90 (startup shutdown): 0041ab06 — startup.cpp shutdown chain
//
// Binary call chain:
//   1. loc_40CC60(dword_636004) → delete the iMsgStopSystem registration
//      loc_40CC60(dword_63600C) → delete the iMsgStartSystem registration
//   2. sub_41C140, nullsub_11, sub_40D840(unk_635608)
//   3. sub_411CE0, sub_408360→sub_40A080, sub_4D9DF0, sub_41BBC0,
//      sub_41BB10, nullsub_11, sub_41CE70, sub_429C70, sub_40F910, sub_411C10
//   4. dword_635644 = 0 (remove the tick callback)
//   5. sub_407320, sub_408320, sub_407DE0, sub_40F5E0
//   6. CLinearMemoryManager pool loop: Instance() →
//      sub_415200 (DestroyPool) for each pool — modeled in the codebase with Delete()
//   7. sub_40BF60 — delete framework message registrations
//   8. sub_4229A0, sub_4D6330, __ioterm, sub_41CDF0 → sub_429070,
//      sub_41CE00(0) — release the render target
// ============================================================================
void RenderPipelineShutdown(void)
{
    // Step 1 — delete pipeline message registrations (binary: loc_40CC60 → UnRegisterMsg)
    if (s_iMsgStopSystem)
    {
        RWS::CEventHandler::UnRegisterMsg(s_iMsgStopSystem);
        s_iMsgStopSystem = 0;
    }
    if (s_iMsgStartSystem)
    {
        RWS::CEventHandler::UnRegisterMsg(s_iMsgStartSystem);
        s_iMsgStartSystem = 0;
    }

    // Step 4 — remove the tick callback (binary: dword_635644 = 0)
    s_pPipelineCallback = 0;

    // Step 6 — release the linear memory pools.
    // The binary removes pools one by one with DestroyPool and keeps the manager alive;
    // the closest match in the codebase is Delete() (the manager is lazily re-created).
    RWS::CLinearMemoryManager::Delete();

    // Step 7 — delete framework message registrations (binary: sub_40BF60 →
    // loc_40CC60 ×3; followed by sub_40EC70 / loc_40E470 subscriber cleanup TODO)
    if (s_iMsgDeleteEventHandler)
    {
        RWS::CEventHandler::UnRegisterMsg(s_iMsgDeleteEventHandler);
        s_iMsgDeleteEventHandler = 0;
    }
    if (s_iMsgDeleteEntity)
    {
        RWS::CEventHandler::UnRegisterMsg(s_iMsgDeleteEntity);
        s_iMsgDeleteEntity = 0;
    }
    if (s_iMsgOnDeleteEntity)
    {
        RWS::CEventHandler::UnRegisterMsg(s_iMsgOnDeleteEntity);
        s_iMsgOnDeleteEntity = 0;
    }

    // Step 8 (partial) — font teardown: in the binary's __ioterm atexit chain
    //   +62 sub_4CD580 (FontSystemTeardown — font name list/pools/path)
    //   +67 sub_4CEF90 (FontSystemShutdown — vertex buffer)
    //   since __ioterm itself has not been restored, the calls are modeled here.
    FontSystemTeardown();
    FontSystemShutdown();

    // TODO(Phase 4.4b) — steps 2, 3, 5, 8 sub-system chain:
    //   sub_41C140 / sub_40D840 (unk_635608 subscriber cleanup) / sub_411CE0 /
    //   sub_408360→sub_40A080 / sub_4D9DF0 / sub_41BBC0 / sub_41BB10 /
    //   sub_41CE70 / sub_429C70 / sub_40F910 / sub_411C10 / sub_407320 /
    //   sub_408320 / sub_407DE0 / sub_40F5E0 / sub_40EC70+loc_40E470
    //   (sub_40BF60 continuation) / sub_4229A0 / sub_4D6330 / __ioterm /
    //   sub_41CDF0 → sub_429070 / sub_41CE00(0)
}

// ============================================================================
// SetLightAmbient (FUN_0041A640)
// Binary Adresi: 0x0041a640
// XREF: sub_41A680 (startup init): 0041a6..+4CA — startup.cpp "Step 13"
//
// Binary body:
//   1. sub_41A5A0() — produces the light context (sub_41A500 definition + sub_50D2A0(3, 1, &cmd)
//      + sub_50D1D0(cmd, 0)). If the context is null, it silently returns.
//   2. [context+0x18] &= ~0x00000002 — clear the current light/link flag
//      (binary: and 0FFFFFFFDh)
//   3. sub_50D2A0(3, 1, &cmd) + sub_50D1D0(cmd, 0) — ambient light command
//      (type = 3 → rpLIGHTAMBIENT family) is applied.
//
// sub_41A5A0 / sub_41A500 / sub_50D2A0 / sub_50D1D0 depend on the RW engine
// command infrastructure (engine command object, dword_65D7A4 vtable +0x144h allocation);
// there is no equivalent in the codebase → TODO(Phase 4.4b).
// ============================================================================
void SetLightAmbient(void)
{
    // TODO(Phase 4.4b) — produce the context (sub_41A5A0), clear the flag
    // ([context+0x18] &= ~2) and add the ambient light to the scene via
    // sub_50D2A0(3, 1)/sub_50D1D0. Deliberate no-op until the context is restored.
}
