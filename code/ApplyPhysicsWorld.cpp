// ============================================================================
// CannonCruise - Physics World Apply (ApplyPhysicsWorld.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyPhysicsWorld.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_0043BB10 (HandleEvents),
//            FUN_00555EF0 (physics world step with 0x3D088889 ~ 1/30s),
//            FUN_0043A1C0 (world update wrapper)
// ============================================================================

#include "ApplyPhysicsWorld.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include "PhysicsBackend.h"
#include "PhysicsWorld.h"

namespace RWS
{

ApplyPhysicsWorld::ApplyPhysicsWorld(const CAttributePacket& packet)
    : m_gravityEnabled(TRUE),
      m_fGravity(9.8f),
      m_fFriction(0.8f),
      m_fRestitution(0.5f),
      m_solverIterations(8),
      m_bBoundsSet(FALSE)
{
    // Binary constructor (FUN_0043BB10) does:
    // 1. Zero-out the object
    // 2. Read physics attributes from packet
    // 3. Store gravity, friction, restitution, solver iterations
    // 4. Initialize bounds if present

    m_gravityEnabled = TRUE;

    // Read physics attributes from packet
    // The packet contains commands for: gravity, friction, restitution, solver iterations
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyPhysicsWorld));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Gravity enable flag
            m_gravityEnabled = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        case 1:
            // Gravity value
            m_fGravity = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 2:
            // Friction value
            m_fFriction = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 3:
            // Restitution value
            m_fRestitution = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 4:
            // Solver iterations
            m_solverIterations = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 5:
            // Bounds enable flag
            m_bBoundsSet = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyPhysicsWorld::~ApplyPhysicsWorld()
{
}

void ApplyPhysicsWorld::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_0043BB10) compares msg.Id with dword_635C68
    // and updates the physics world only on a match.
    // For now, just forward to the physics world update.
    // TODO: full event-integrated physics when event system is wired.
    (void)msg;
}

RwBool ApplyPhysicsWorld::Apply(const CAttributePacket& packet)
{
    // Binary (sub_43BB10 / loc_43BB60) applies physics world attributes:
    // - Reads gravity, friction, restitution, solver iterations from packet
    // - Initializes or updates the Bullet physics world settings
    // - Applies bounds clamp if present

    // Apply the read attributes (already done in constructor for simplicity)
    // In a full implementation, this would update the live physics world.

    // TODO: forward these settings to the live world once the Apply path
    // has access to its PhysicsWorldHandle (PhysicsWorld_SetGravity etc.).
    // PhysicsBackend exposes only per-world C functions, no global setters.
    (void)packet;
    return TRUE;
}

} // namespace RWS