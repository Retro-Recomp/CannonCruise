#pragma once

// ============================================================================
// CannonCruise - Waypoint (CWaypoint.h)
// Original path: D:\Projects\CannonCruisePC\code\Waypoint.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: ctor FUN_004BFE60, event FUN_004BF580, render FUN_004BBF90
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class CWaypoint
 * @brief Waypoint entity for navigation and event triggering.
 *
 * Represents a waypoint in the world with position, links to other waypoints,
 * and event flags. Binary-driven: constructor allocates two blocks (0x10 + 0x80)
 * and sets up EVENT_INQUIRE_WAYPOINT event. Render strings
 * MSG_THUNDER0_RENDER/MSG_THUNDER1_RENDER are configured via loc_4BFFF0.
 *
 * **Binary references:**
 * - Ctor: `FUN_004BFE60`
 * - Second alloc: `FUN_004BFF30` (0x80 bytes)
 * - Setup func: `FUN_004BBFFF` (loc_4BFFF0)
 * - Event: `EVENT_INQUIRE_WAYPOINT`
 * - Render strings: `MSG_THUNDER0_RENDER`, `MSG_THUNDER1_RENDER`
 */
class CWaypoint : public CAttributeHandler, public CEventHandler
{
public:
    RWS_MAKENEWCLASS(CWaypoint);
    RWS_DECLARE_CLASS(CWaypoint);

    /**
     * @brief Constructor — initializes waypoint from attribute packet.
     * @param id Waypoint identifier byte.
     * @param pos World position.
     */
    CWaypoint(RwInt8 id, const RwV3d& pos);

    /// Packet constructor required by RWS_MAKENEWCLASS (attributes not parsed yet).
    explicit CWaypoint(const CAttributePacket&) : CWaypoint(0, RwV3d{ 0.0f, 0.0f, 0.0f }) {}

    /**
     * @brief Destructor — clears event registrations and frees blocks.
     */
    virtual ~CWaypoint();

    /**
     * @brief Get the waypoint ID.
     * @return Identifier byte.
     */
    RwInt8 GetId() const { return m_id; }

    /**
     * @brief Get the world position.
     * @return Position vector.
     */
    const RwV3d& GetPos() const { return m_pos; }

    /**
     * @brief Set a linked waypoint.
     * @param idx Link index (0-based).
     * @param waypoint Pointer to linked waypoint.
     */
    void SetLink(int idx, CWaypoint* waypoint);

    /**
     * @brief Get a linked waypoint.
     * @param idx Link index (0-based).
     * @return Pointer to linked waypoint, or nullptr.
     */
    CWaypoint* GetLink(int idx) const;

    /**
     * @brief Trigger the waypoint event.
     * Fires EVENT_INQUIRE_WAYPOINT.
     */
    void TriggerEvent();

private:
    RwInt8    m_id;                ///< Waypoint identifier byte
    RwV3d     m_pos;               ///< World position
    RwInt32   m_links[8];          ///< Link indices to other waypoints (unused = -1)
    RwBool    m_active;            ///< Waypoint active flag
    // Padded/additional fields from binary:
    // +18h: control/dword with [0] = off_5EF3E4, [4] = 1
    // +20h: string buffer (up to 32 chars)
    // +30h: float field (value ~1.0)
    // +34h: dword from sub_4D0C90
    // +38h: dword from sub_4CF020
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
};

} // namespace RWS