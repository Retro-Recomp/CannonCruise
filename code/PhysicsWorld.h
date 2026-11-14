#ifndef PHYSICSWORLD_H
#define PHYSICSWORLD_H

// ============================================================================
// CannonCruise - Havok Physics World (PhysicsWorld.h)
// Original path: D:\Projects\CannonCruisePC\code\PhysicsWorld.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// ============================================================================

#include "Singleton.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include <rwcore.h>
#include <vector>

/**
 * @class CPhysicsWorld
 * @brief Singleton wrapper around the physics backend world.
 *
 * The current backend is Bullet; body/phantom pointers are delegated through
 * the backend API. RigidBodyExistsInWorld checks local membership, not SDK
 * object validity. Accessed via CSingleton<CPhysicsWorld>.
 */
class CPhysicsWorld : public CSingleton<CPhysicsWorld>
{
public:
    /** @brief Constructor – creates the configured physics backend world. */
    CPhysicsWorld();

    /**
     * @brief Destructor – asserts that all rigid bodies have been removed.
     *
     * Clearing the vectors here is a safety measure; leaking bodies would
     * indicate a teardown ordering bug.
     */
    virtual ~CPhysicsWorld();

    /**
     * @brief Startup chain allocator — new if no singleton, existing one otherwise.
     *
     * Binary: sub_41A680 (startup init) — after HavokInit it allocates the
     * singleton itself (alloc 0x440 → ctor → written to dword_635AF4);
     * the CPhysicsWorldBehaviour ctor on the other hand USES the singleton. This helper
     * covers that step in startup.cpp; during level load the behaviour calls it
     * as a fallback.
     */
    static CPhysicsWorld* Create();

    /**
     * @brief Advances the physics simulation by deltaTime seconds.
     * @param deltaTime  Time step supplied to the backend.
     *
     * The port currently supplies 1/60 s from its runtime loop. The original
     * world's update wrapper passes 1/30 to an internal routine; parity is
     * unresolved.
     */
    void Update(RwReal deltaTime);

    /**
     * @brief Registers a rigid body with the physics world.
     * @param pBody  Opaque body pointer created by the active backend.
     */
    void AddRigidBody(void* pBody);

    /**
     * @brief Unregisters and removes a rigid body from the simulation.
     * @param pBody  Pointer to the body to remove.
     *
     * Performs a linear search; safe to call even if the body was already
     * removed (the search simply finds nothing).
     */
    void RemoveRigidBody(void* pBody);

    /**
     * @brief Checks whether a rigid body is currently registered.
     * @param pBody  Pointer to the body to look up.
     * @return TRUE if the body is found, FALSE otherwise.
     */
    RwBool RigidBodyExistsInWorld(void* pBody) const;

    /**
     * @brief Registers a phantom (trigger volume) with the physics world.
     * @param pPhantom  Opaque trigger pointer created by the active backend.
     */
    void AddPhantom(void* pPhantom);

    /**
     * @brief Unregisters and removes a phantom from the simulation.
     * @param pPhantom  Pointer to the phantom to remove.
     */
    void RemovePhantom(void* pPhantom);

private:
    std::vector<void*> m_avpRigidBodyVector; ///< All active rigid bodies.
    std::vector<void*> m_avpPhantomVector;   ///< All active phantom volumes.
    void* m_pHavokWorld;                     ///< Legacy name; opaque backend world.
};

/**
 * @class CPhysicsWorldBehaviour
 * @brief RWS behaviour that owns and drives the CPhysicsWorld singleton.
 *
 * Registered with the RenderWare Studio class factory so the engine can
 * instantiate it from level data. HandleEvents currently steps on every
 * delivered message without checking its ID; the original message filter is
 * not yet restored.
 */
class CPhysicsWorldBehaviour : public RWS::CEventHandler
{
public:
    /** @brief Constructor – uses the startup-created CPhysicsWorld singleton
     * (binary sub_41A680); creates one only if absent (port fallback). */
    CPhysicsWorldBehaviour();

    /** @brief Destructor – destroys the world only if this behaviour created it. */
    virtual ~CPhysicsWorldBehaviour();

    /**
     * @brief Steps the physics simulation when an event is delivered.
     * @param msg  Engine message (currently not filtered by ID).
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

private:
    CPhysicsWorld* m_pPhysicsWorld; ///< singleton (ownership: m_bOwnsWorld)
    RwBool m_bOwnsWorld;            ///< if TRUE, deleted in the dtor
};

#endif // PHYSICSWORLD_H
