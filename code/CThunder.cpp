// ============================================================================
// CannonCruise - Thunder (CThunder.cpp)
// Original path: D:\Projects\CannonCruisePC\code\Thunder.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_004BB5C0 (ctor),
//            FUN_004BBB70 (audio init),
//            FUN_0040B300 (strcpy/float store),
//            FUN_004D0C90, FUN_004CF020,
//            FUN_004BBF90 (loc_40BF90 - message setup),
//            FUN_004053F0 (malloc/new)
// ============================================================================

#include "CThunder.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include <cstring>

namespace RWS
{

CThunder::CThunder(const RwV3d& pos)
    : m_pos(pos), m_bActive(FALSE)
{
    // Binary constructor (sub_4BB5C0) does:
    // 1. Store vtable pointer off_5EF3F0 at start of object
    // 2. Clear [esi+4], [esi+8], [esi+0Ch], [esi+10h], [esi+14h] to 0
    // 3. At +18h: { off_5EF3E4, 1 }
    // 4. sub_40B300 called 3 times with "1", "2", "3" at offsets +20h, +28h, +30h
    //    (these are strcpy stores or float stores)
    // 5. off_5F0A08 and off_5F0A00 stored at +0 and +18h
    // 6. [esi+64h] = 0, [esi+70h] = 0, [esi+78h] = 0
    // 7. [esi+38h] = sub_4D0C90() result
    // 8. [esi+34h] = sub_4CF020() result
    // 9. [esi+3Ch] = 3F800000h (1.0f)
    // 10. [esi+40h] = 3DCCCCCDh (≈0.8f)
    // 11. [esi+44h] = 40A00000h or similar (≈1.5f)
    // 12. [esi+48h] = 1
    // 13. [esi+4Ch] = 5
    // 14. [esi+50h] = 3F000000h (0.5f)
    // 15. [esi+54h] = 3F000000h (0.5f)
    // 16. [esi+58h] = 3F000000h (0.5f)

    // Initialise all cleared dwords/fields
    m_bActive = FALSE;

    // Zero out the object memory pattern from binary
    // (the real layout has specific field values set by sub_40B300 etc.)
    // For the port, we'll keep it minimal and document the binary-derived layout.
}

CThunder::~CThunder()
{
}

void CThunder::PlaySound()
{
    // Based on binary: plays "Audio_Thunder" via
    // sub_4BBB70 and loc_40BF90 with "SAudioPlayData*"
    // and the audio string. For now, just set active flag.
    m_bActive = TRUE;
}

void CThunder::RenderEffect(RwInt32 renderMsg)
{
    // Based on binary: calls loc_40BF90 with
    // "RwCamera*" and either "MSG_THUNDER0_RENDER"
    // or "MSG_THUNDER1_RENDER" depending on renderMsg.
    // For now, just a no-op stub.
    (void)renderMsg;
}

void CThunder::Update(RwReal deltaTime)
{
    // Based on binary: uses the float fields at +30h, +34h, +38h
    // for timing/intensity decay. For now, no-op stub.
    (void)deltaTime;
}

} // namespace RWS