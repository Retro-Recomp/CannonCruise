// ============================================================================
// CannonCruise - GameManager bootstrap (gamemanager.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\startup\gamemanager.cpp
// Reverse-engineered and rewritten by analyzing with CC_ida.asm.
//
// Binary Functions:
//   FUN_0044aaf0 — GameManagerInit   (startup init chain, "Step 15")
//   FUN_0044aba0 — GameManagerShutdown (startup shutdown chain, +24)
//   sub_449d30   — GameManager entry table (free list) setup
//
// Note: The file's context is only known from the call in startup.cpp;
//      the binary has no assert path string for these functions.
// ============================================================================

#include "rwcore.h"
#include <cstdlib>

namespace
{
    // GameManager entry record — 12 (0x0C) bytes (binary: sub_4053F0(0Ch, 2))
    //   [+0x00] free list initial cursor (written by sub_449D30)
    //   [+0x04] entry count (binary: 0x2710 = 10000)
    //   [+0x08] entry table (binary: 0x1D4C0 = 10000 * 12 bytes)
    struct GameManagerEntry
    {
        void*    pCursor;   // +0x00 — free list cursor (points to the table)
        RwUInt32 entryCount;// +0x04
        void*    pEntries;  // +0x08
    };

    // dword_636878 — GameManager singleton instance
    GameManagerEntry* s_pGameManager = 0;
}

// ---------------------------------------------------------------------------
// Sets up the free list for the GameManager entry table (binary: sub_449D30)
// thiscall: ECX = this. The table elements form a 12-byte chain:
//   entry[i].+0x04 = &entry[i+1]  (next)
//   entry[i].+0x08 = &entry[i-1]  (previous)
//   entry[count-1].+0x08 = &entry[count-2]; this.+0x00 = start of the table
// ---------------------------------------------------------------------------
static void InitGameManagerEntries(GameManagerEntry* pThis)
{
    RwUInt32 count = pThis->entryCount;
    char*    pTable = static_cast<char*>(pThis->pEntries);

    if (count <= 1)
    {
        // loc_449DAD: single entry — chain head 0, cursor = table
        if (pTable)
        {
            *reinterpret_cast<void**>(pTable + 4) = 0;
        }
        pThis->pCursor = pTable;
        return;
    }

    // The first entry has no back link (+0x08 = 0)
    *reinterpret_cast<void**>(pTable + 8) = 0;

    // entry[i].+0x04 = &entry[i+1]  (i = 0 .. count-2)
    // entry[i].+0x08 = &entry[i-1]  (i = 1 .. count-1)
    for (RwUInt32 i = 0; i + 1 < count; ++i)
    {
        char* pEntry = pTable + i * 12;
        *reinterpret_cast<void**>(pEntry + 4) = pEntry + 12;
        if (i > 0)
        {
            *reinterpret_cast<void**>(pEntry + 8) = pEntry - 12;
        }
    }

    // The last entry's back link = second-to-last (binary tail: [edx+eax*4-4])
    char* pLast = pTable + (count - 1) * 12;
    *reinterpret_cast<void**>(pLast + 8) = pTable + (count - 2) * 12;

    // this.+0x00 = start of the table (binary: mov [ecx], edx)
    pThis->pCursor = pTable;
}

// ============================================================================
// GameManagerInit (FUN_0044AAF0)
// Binary Address: 0x0044aaf0
// XREF: sub_41A680 (startup init): 0041a6..+4E5 — startup.cpp "Step 15"
//
// If dword_636878 is null, creates a new GameManager:
//   1. 12-byte manager object (sub_4053F0(0Ch, 2))
//   2. entryCount = 0x2710 (10000); pEntries = new(0x1D4C0 = 10000 * 12)
//   3. The free list chain is set up via sub_449D30
// In the binary, if the object already exists (dword_636878 != 0), returns without touching anything.
// (SEH scope guard: sub_404E70(unk_657BA8) / sub_404E70(0) — in the comment.)
// ============================================================================
void GameManagerInit(void)
{
    if (s_pGameManager)
        return;

    GameManagerEntry* p = static_cast<GameManagerEntry*>(calloc(1, sizeof(GameManagerEntry)));
    if (!p)
        return;

    p->entryCount = 0x2710;                         // mov [esi+4], 2710h
    p->pEntries = calloc(p->entryCount, 12);        // sub_4053F0(1D4C0h, 2)
    if (!p->pEntries)
    {
        free(p);
        return;
    }

    InitGameManagerEntries(p);                      // sub_449D30

    s_pGameManager = p;                             // mov dword_636878, esi
}

// ============================================================================
// GameManagerShutdown (FUN_0044ABA0)
// Binary Address: 0x0044aba0
// XREF: sub_41AB90 (startup shutdown): +24 — startup.cpp shutdown chain
//
// If dword_636878 is null, it skips. Otherwise:
//   1. pEntries (entry table) is freed (sub_4053B0 = operator delete)
//   2. the manager object is freed
//   3. dword_636878 = 0
// ============================================================================
void GameManagerShutdown(void)
{
    GameManagerEntry* p = s_pGameManager;
    if (!p)
        return;

    free(p->pEntries);      // sub_4053B0(p->pEntries)
    free(p);                // sub_4053B0(p)
    s_pGameManager = 0;     // mov dword_636878, 0
}
