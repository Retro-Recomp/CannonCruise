#pragma once

// ============================================================================
// CannonCruise - Physics World Apply (ApplyPhysicsWorld.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyPhysicsWorld.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_0043BB10 - compares message ID with dword_635C68
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyPhysicsWorld
 * @brief Applies physics world settings from attribute packet.
 *
 * Handles physics world initialization parameters such as gravity,
 * friction, restitution, solver iterations and bounds.
 *
 * **Binary references:**
 * - HandleEvents: `FUN_0043BB10`
 * - Compares incoming message ID with `dword_635C68`
 */
class ApplyPhysicsWorld : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyPhysicsWorld);
    RWS_DECLARE_CLASS(ApplyPhysicsWorld);

    /**
     * @brief Constructor — initializes physics world apply handler.
     * @param packet Attribute packet with physics settings.
     */
    ApplyPhysicsWorld(const CAttributePacket& packet);

    /**
     * @brief Applies physics world attributes.
     * @param msg Event message container.
     * @return TRUE if applied successfully.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure physics world.
     * @param packet Packet containing physics configuration.
     * @return TRUE if physics world attributes were applied.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwBool m_gravityEnabled;
    RwReal m_fGravity;
    RwReal m_fFriction;
    RwReal m_fRestitution;
    RwInt32 m_solverIterations;
    RwBool m_bBoundsSet;
};

} // namespace RWS