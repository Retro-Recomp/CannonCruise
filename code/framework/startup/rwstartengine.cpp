// ============================================================================
// CannonCruise - RwStartEngine (rwstartengine.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\startup\rwstartengine.cpp
// Reverse-engineered and rewritten by analyzing with CC_ida.asm.
//
// Binary Function:
//   FUN_00418ac0 — RwStartEngine (startup init chain, "Step 7")
//
// Note: The file's context is only known from the call XREF in startup.cpp;
//      there is no assert path string inside the function.
// ============================================================================

#include "rwcore.h"
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>        // GetCurrentDirectoryA / GetDriveTypeA
#include <direct.h>         // _getdrive / _getcwd / _chdir
#else
#include <unistd.h>         // getcwd / chdir
#include <sys/stat.h>       // stat — search path validation
#endif

namespace
{
    // sub_4CB8D0 — search path entry (binary: arg_0 = 0x14 = 20 byte structure;
    // arg_1 = root directory ("X:" or cwd), arg_2 = name, single letter/"."/
    // off_612FE0 default). The structure is owned by the FS system in the binary;
    // it will be connected to the RtFS/CustomFS restoration in Phase 4.4b.
    // NOTE: the name "SearchPath" clashes with the Windows API (processenv.h) —
    // renamed to "RwSearchPath".
    struct RwSearchPath
    {
        char        path[0x104];
        char        name[8];
        bool        active;
        RwSearchPath* pNext;
    };

    RwSearchPath* s_pSearchPaths = 0;    // registered search paths (linked list)
    RwSearchPath* s_pActivePath = 0;     // last entry activated with sub_4CAFC0

    // sub_4CB8D0(0x14, path, name) — create a new search path entry
    RwSearchPath* CreateSearchPath(const char* pPath, const char* pName)
    {
        RwSearchPath* p = static_cast<RwSearchPath*>(calloc(1, sizeof(RwSearchPath)));
        if (!p)
            return 0;
        strncpy(p->path, pPath, sizeof(p->path) - 1);
        strncpy(p->name, pName, sizeof(p->name) - 1);
        p->pNext = s_pSearchPaths;
        s_pSearchPaths = p;
        return p;
    }

    // sub_4CAD20(pPath) — validate that the entry exists (current directory)
    RwBool ValidateSearchPath(const RwSearchPath* pPath)
    {
        if (!pPath || pPath->path[0] == 0)
            return FALSE;
#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(pPath->path);
        return (attrs != INVALID_FILE_ATTRIBUTES &&
                (attrs & FILE_ATTRIBUTE_DIRECTORY)) ? TRUE : FALSE;
#else
        struct stat st;
        return (stat(pPath->path, &st) == 0 && S_ISDIR(st.st_mode)) ? TRUE : FALSE;
#endif
    }

    // sub_4CAFC0(pPath) — make the entry the active search path
    void ActivateSearchPath(RwSearchPath* pPath)
    {
        if (pPath)
        {
            pPath->active = true;
            s_pActivePath = pPath;
        }
    }
}

// ============================================================================
// RwStartEngine (FUN_00418AC0)
// Binary Address: 0x00418ac0
// XREF: sub_41A680 (startup init): 0041aa66 — startup.cpp "Step 7"
// Return: TRUE = search path ready / FALSE = search path could not be set up
//
// Binary body (MSVC CRT + KERNEL32):
//   1. drive = __getdrive(); cwd = __getcwd(Path, 0x104)
//   2. drive < 1 (no valid drive):
//        n = GetCurrentDirectoryA(Buffer, 0x100)
//        n == 0 or n > 0x100 → FALSE
//        engine dword_65D7A4 vtable[+0x100h](Buffer, unk_612E84)
//          — report the working directory to the engine
//        esi = sub_4CB8D0(0x14, Buffer, off_612FE0) — create search path
//        esi null or sub_4CAD20(esi) fails → FALSE
//        sub_4CAFC0(esi) — make it the active search path → TRUE
//   3. drive >= 1: scan B: .. Z: (ebx = 2 .. 0x1A)
//        GetDriveTypeA("X:\"); set X:; type 0/1 (UNKNOWN/NO_ROOT_DIR) → skip
//        sub_4CB8D0(0x14, "X:", "X") — name = drive letter
//        creation/validation fails → FALSE (loop breaks)
//        if drive == current drive, activate it with sub_4CAFC0
//        (the loop continues even after activation)
//   4. __chdir(Path) — return to the old working directory → TRUE
//
// Phase 4.4b TODO: engine vtable[+0x100h] call (dword_65D7A4 engine object
// will exist after RwEngineOpen).
// ============================================================================
RwBool RwStartEngine(void)
{
#ifdef _WIN32
    int  drive = _getdrive();
    char savedCwd[0x104];
    savedCwd[0] = 0;
    _getcwd(savedCwd, 0x104);

    // --- Branch 2: no valid drive ---
    if (drive < 1)
    {
        char cwd[0x100];
        DWORD n = GetCurrentDirectoryA(sizeof(cwd), cwd);
        if (n == 0 || n > sizeof(cwd))
            return FALSE;

        // TODO(Phase 4.4b): engine dword_65D7A4 vtable[+0x100h](cwd, unk_612E84)
        RwSearchPath* pPath = CreateSearchPath(cwd, ".");   // off_612FE0 (default name)
        if (!pPath)
            return FALSE;
        if (!ValidateSearchPath(pPath))
            return FALSE;
        ActivateSearchPath(pPath);
        return TRUE;
    }

    // --- Branch 3: B: .. Z: drive scan ---
    for (int d = 2; d <= 26; ++d)
    {
        char root[4];
        root[0] = static_cast<char>('A' + d - 1);
        root[1] = ':';
        root[2] = '\\';
        root[3] = 0;
        UINT driveType = GetDriveTypeA(root);

        root[2] = 0;                                // "X:" (binary: var_20A = 0)
        if (driveType == DRIVE_UNKNOWN || driveType == DRIVE_NO_ROOT_DIR)
            continue;

        char name[2];
        name[0] = root[0];                          // name = drive letter
        name[1] = 0;

        RwSearchPath* pPath = CreateSearchPath(root, name);
        if (!pPath)
            return FALSE;                           // binary: loc_418B5E → FALSE
        if (!ValidateSearchPath(pPath))
            return FALSE;

        if (d == drive)                             // binary: cmp ebp, ebx
            ActivateSearchPath(pPath);              // activate only on the current drive
    }

    if (savedCwd[0])
        _chdir(savedCwd);                           // __chdir(Path)
    return TRUE;

#else
    // POSIX equivalent: single search path = working directory (branch 2 logic)
    char cwd[0x104];
    if (!getcwd(cwd, sizeof(cwd)))
        return FALSE;

    RwSearchPath* pPath = CreateSearchPath(cwd, ".");
    if (!pPath)
        return FALSE;
    if (!ValidateSearchPath(pPath))
        return FALSE;
    ActivateSearchPath(pPath);
    return TRUE;
#endif
}
