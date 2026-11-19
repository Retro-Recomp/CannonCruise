// ============================================================================
// CannonCruise - Waypoint (CWaypoint.cpp)
// Original path: D:\Projects\CannonCruisePC\code\Waypoint.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_004BFE60 (ctor), FUN_004BFF30 (0x80 alloc),
//            FUN_004BBFFF (loc_4BFFF0 setup), FUN_004053B0 (strlen),
//            FUN_004053F0 (malloc/new), FUN_0040B300 (strcpy),
//            FUN_0040B300 (float store), FUN_004D0C90, FUN_004CF020
// ============================================================================

#include "CWaypoint.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "RwsUtils.h"

namespace RWS
{

CWaypoint::CWaypoint(RwInt8 id, const RwV3d& pos)
    : m_id(id), m_pos(pos), m_active(TRUE)
{
    // Initialise this block (0x10 bytes)
    // Stored at the first allocation (sub_4053F0 returns a 0x10 block)
    m_links[0] = -1;
    m_links[1] = -1;
    m_links[2] = -1;
    m_links[3] = -1;
    m_links[4] = -1;
    m_links[5] = -1;
    m_links[6] = -1;
    m_links[7] = -1;

    // Second allocation (0x80 bytes at loc_4BFF30)
    // This is where loc_4BFFF0 sets up the full waypoint state
    // Based on binary analysis, the second block has these fields:
    // +00: vtable pointer (off_5EF3F0)
    // +04: 0
    // +08: 0
    // +0Ch: string offset from strlen, stored here
    // +10h: 0
    // +14h: 0
    // +18h: { off_5EF3E4, 1 }
    // +20h: string "0" via strcpy
    // +28h: ...
    // +30h: float (~1.0)
    // +34h: sub_4D0C90 result
    // +38h: sub_4CF020 result
    // +3Ch: 1.0f (3F800000h)
    // +40h: ~0.8f (3DCCCCCDh)
    // +44h: ~0.8f (3DCCCCCDh)
    // +48h: 1
    // +4Ch: 5
    // +50h: 0.5f (3F000000h)
    // +54h: 0.5f (3F000000h)
    // +58h: 0.5f (3F000000h)
    // +64h: 0
    // +70h: 0
    // +78h: 0

    // For now, zero the second block (the full 0x80-byte struct will
    // be initialized by the game's behavior system when the waypoint
    // is linked into a level .RWS).
    // TODO: wire up loc_4BFFF0 exact field initialization if needed.
    for (int i = 0; i < 8; i++)
        m_links[i] = -1;
}

CWaypoint::~CWaypoint()
{
}

void CWaypoint::SetLink(int idx, CWaypoint* waypoint)
{
    if (idx >= 0 && idx < 8)
        m_links[idx] = (waypoint ? reinterpret_cast<int>(waypoint) : -1);
}

CWaypoint* CWaypoint::GetLink(int idx) const
{
    if (idx >= 0 && idx < 8 && m_links[idx] >= 0)
        return reinterpret_cast<CWaypoint*>(m_links[idx]);
    return nullptr;
}

void CWaypoint::TriggerEvent()
{
    // Fire EVENT_INQUIRE_WAYPOINT event
    // Based on binary: loc_4BFFF0 calls loc_40BF90 with
    // "EVENT_INQUIRE_WAYPOINT" string and the waypoint data block
    // TODO: integrate with the game's event system when available
}

} // namespace RWS