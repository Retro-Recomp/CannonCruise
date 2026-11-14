// ============================================================================
// CannonCruise - Havok system bootstrap (havokstart.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\startup\havokstart.cpp
// Reverse-engineered and rewritten by analyzing with CC_ida.asm.
//
// Binary Functions:
//   FUN_00434fb0 — HavokInit        (startup init chain, "Step 1")
//   FUN_004356f0 — TextureCacheShutdown (startup shutdown chain, +1F)
//
// Note: TextureCacheShutdown lives in this file because in the binary it shares the same
//      global lifecycle with HavokInit:
//        byte_660934  — system flag (set by HavokInit, cleared by shutdown)
//        dword_660920 — active memory context (the Havok singleton)
//        dword_660938 / dword_66093c / dword_660940 — created by HavokInit
//                          3 refcounted sub-system objects (released by shutdown)
//      File context: there is no assert path string inside the function (only
//      a call XREF from startup.cpp).
// ============================================================================

#include "rwcore.h"
#include <cstdlib>
#include <cstring>

namespace
{
    // -------------------------------------------------------------------------
    // byte_660934 — system lifecycle flag
    // -------------------------------------------------------------------------
    bool s_havokInited = false;

    // -------------------------------------------------------------------------
    // dword_660920 — active memory context (swapped via sub_52DCA0)
    // sub_435810 ctor contract:
    //   [+0x00] vtable (off_5EF898) — class methods, Phase 4.4b
    //   [+0x04] refcount = 1
    //   [+0x08, +0x0C, +0x10] = 0
    //   [+0x14] = -1
    //   [+0x20 .. +0x4028) = 0 (0x1002 dword arena table, binary: rep stosd)
    //   sub_52DC80(this, align16(+0x2F), 0x4000) — arena setup (TODO)
    //   Total allocation: 0x4030 bytes (sub_4053F0(4030h, 2))
    // -------------------------------------------------------------------------
    struct HavokContext
    {
        void*    vtable;          // +0x00
        RwUInt32 refCount;        // +0x04
        RwUInt32 field_08;        // +0x08
        RwUInt32 field_0C;        // +0x0C
        RwUInt32 field_10;        // +0x10
        RwInt32  field_14;        // +0x14 = -1
        RwUInt32 pad_18;          // +0x18
        RwUInt32 pad_1C;          // +0x1C
        RwUInt32 arena[0x1002];   // +0x20 .. +0x4028
        RwUInt32 padTail[2];      // +0x4028 .. +0x4030 (full allocation size)
    };

    HavokContext* s_pMemoryContext = 0;   // dword_660920

    // -------------------------------------------------------------------------
    // Refcounted sub-system objects (binary: vtable+0x10h allocation, 0x12 + type)
    //   dword_660940 — type = 0x08, vtable off_5EF848
    //   dword_66093C — type = 0x20, vtable off_5EF83C (after ctor sub_4357A0
    //                  it is swapped to off_5EF830; the +0x08/+0x14 chains
    //                  are built via sub_52DCE0 — TODO Phase 4.4b)
    //   dword_660938 — type = 0x08, vtable off_5EF808
    // In the binary the refcount lives at +0x06 (word); when it reaches zero, vtable[0](1) is called
    // to destroy the object.
    // -------------------------------------------------------------------------
    struct HavokRefObject
    {
        void*    vtable;      // +0x00
        RwUInt16 type;        // +0x04
        RwUInt16 refCount;    // +0x06
        void*    links[6];    // +0x08 .. +0x1C
    };

    HavokRefObject* s_pSystem38 = 0;   // dword_660938
    HavokRefObject* s_pSystem3C = 0;   // dword_66093C
    HavokRefObject* s_pSystem40 = 0;   // dword_660940

    // sub_4053F0(size, flags) — MSVC operator new equivalent (binary comment)
    void* SysAlloc(size_t size)
    {
        return calloc(1, size);
    }

    // Create sub-system object (binary: context vtable[+0x10h](0x12, type))
    HavokRefObject* CreateRefObject(RwUInt16 type, void* vtable)
    {
        HavokRefObject* p = static_cast<HavokRefObject*>(SysAlloc(sizeof(HavokRefObject)));
        if (!p)
            return 0;
        p->vtable = vtable;
        p->type = type;
        p->refCount = 1;
        return p;
    }

    // Binary refcount pattern: dec word [+0x06]; zero → vtable[0](1) / free
    void ReleaseRefObject(HavokRefObject*& pObject)
    {
        if (!pObject)
            return;
        if (pObject->refCount > 0)
        {
            --pObject->refCount;
            if (pObject->refCount == 0)
            {
                // vtable[0](1) — destroyer (TODO(Phase 4.4b): class dtors)
                free(pObject);
            }
        }
        pObject = 0;
    }

    // sub_52DCA0(pNew) — swaps the active memory context:
    //   if one exists, decrement its refcount; if it reaches 0, destroy it via vtable[+0x20h](1);
    //   then dword_660920 = pNew.
    void SetActiveMemoryContext(HavokContext* pNew)
    {
        if (s_pMemoryContext)
        {
            if (s_pMemoryContext->refCount > 0)
            {
                --s_pMemoryContext->refCount;
                if (s_pMemoryContext->refCount == 0)
                {
                    // vtable[+0x20h](1) — context destroyer (model: release)
                    free(s_pMemoryContext);
                }
            }
        }
        s_pMemoryContext = pNew;
    }

    // sub_435810 — HavokContext ctor (binary contract, see struct comment)
    HavokContext* CreateHavokContext()
    {
        HavokContext* p = static_cast<HavokContext*>(SysAlloc(0x4030));
        if (!p)
            return 0;
        p->vtable = 0;                       // off_5EF898 (TODO Phase 4.4b)
        p->refCount = 1;                     // [esi+4] = 1
        p->field_08 = 0;
        p->field_0C = 0;
        p->field_10 = 0;
        p->field_14 = -1;                    // [esi+14h] = -1
        memset(p->arena, 0, sizeof(p->arena));   // rep stosd (0x1002 dword)
        // TODO(Phase 4.4b): sub_52DC80(this, align16(+0x2F), 0x4000)
        return p;
    }
}

// ============================================================================
// HavokInit (FUN_00434FB0)
// Binary Address: 0x00434fb0
// XREF: sub_41A680 (startup init): 0041a6a5 — startup.cpp "Step 1"
//
// Binary body (SEH wrapper skipped):
//   1. byte_660934 != 0 → already initialized, direct exit
//   2. singleton = sub_4053F0(4030h, 2) + sub_435810 ctor
//   3. sub_52DCA0(singleton) — swap into the active memory context
//   4. dword_660940: type 8, vtable off_5EF848 (old ref is released)
//   5. dword_66093C: type 0x20, vtable off_5EF83C → ctor sub_4357A0,
//      sub_52DCE0 zincirleri (+0x08, +0x14), vtable → off_5EF830
//   6. dword_660938: type 8, vtable off_5EF808
//   7. sub_52E600() — next setup stage (inverse: sub_52E700)
//   8. byte_660934 = 1; call off_6229C4 at exit (atexit chain)
//
// Note: DAT_00635af4 (ms_pSingleton) is NOT written in this function — the startup
//      assert stays null on purpose (startup assert: "ms_pSingleton == NULL
//      Failed", .\Singleton.h line 0x33).
// ============================================================================
void HavokInit(void)
{
    if (s_havokInited)
        return;

    // Step 2-3 — singleton + active context swap
    HavokContext* pContext = CreateHavokContext();
    if (!pContext)
        return;
    SetActiveMemoryContext(pContext);

    // Step 4 — type 8 object (dword_660940)
    {
        HavokRefObject* pOld = s_pSystem40;
        s_pSystem40 = CreateRefObject(8, 0);          // vtable: off_5EF848
        ReleaseRefObject(pOld);
    }

    // Step 5 — type 0x20 object (dword_66093C)
    // TODO(Phase 4.4b): ctor sub_4357A0, sub_52DCE0 chains (+0x08/+0x14),
    // vtable swap off_5EF83C → off_5EF830, +0x10/+0x1C ref release
    {
        HavokRefObject* pOld = s_pSystem3C;
        s_pSystem3C = CreateRefObject(0x20, 0);       // vtable: off_5EF83C
        ReleaseRefObject(pOld);
    }

    // Step 6 — type 8 object (dword_660938)
    {
        HavokRefObject* pOld = s_pSystem38;
        s_pSystem38 = CreateRefObject(8, 0);          // vtable: off_5EF808
        ReleaseRefObject(pOld);
    }

    // Step 7 — TODO(Phase 4.4b): sub_52E600()
    s_havokInited = true;                             // byte_660934 = 1

    // Step 8 — TODO(Phase 4.4b): off_6229C4 (atexit/cleanup chain registration)
}

// ============================================================================
// TextureCacheShutdown (FUN_004356F0)
// Binary Address: 0x004356f0
// XREF: sub_41AB90 (startup shutdown): 0041ab..+1F — startup.cpp shutdown chain
//
// Binary body:
//   1. byte_660934 == 0 → not initialized; straight to the tail (off_6229C8)
//   2. sub_52E700() — HavokInit sub_52E600'nin tersi (TODO)
//   3. dword_660938 refcount-1; 0 → vtable[0](1); global = 0
//   4. same for dword_66093C
//   5. same for dword_660940
//   6. sub_52DCA0(0) — swap the active context to null (old context refcount
//      drops to 0 → gets destroyed → the singleton lives/dies here)
//   7. byte_660934 = 0; tail: jmp off_6229C8 (global cleanup chain, TODO)
// ============================================================================
void TextureCacheShutdown(void)
{
    if (!s_havokInited)
        return;

    // Step 2 — TODO(Phase 4.4b): sub_52E700()

    // Step 3-5 — release the three sub-system objects (binary order: 38, 3C, 40)
    ReleaseRefObject(s_pSystem38);
    ReleaseRefObject(s_pSystem3C);
    ReleaseRefObject(s_pSystem40);

    // Step 6 — swap the active context to null (binary: sub_52DCA0(0))
    SetActiveMemoryContext(0);

    s_havokInited = false;                            // byte_660934 = 0

    // Step 7 — TODO(Phase 4.4b): off_6229C8 (global cleanup chain)
}
