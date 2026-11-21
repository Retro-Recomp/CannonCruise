#pragma once

// ============================================================================
// CannonCruise - Logic OR (CLogic_OR.h)
// Original path: D:\Projects\CannonCruisePC\code\Logic_OR.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: ctor FUN_0042C7B0 - FUN_0042C930
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"

namespace RWS
{

/**
 * @class CLogic_OR
 * @brief OR-combination logic node.
 *
 * Evaluates a list of sub-conditions; returns TRUE if any sub-condition
 * is true. Used for compound trigger logic (e.g. "trigger when tower1
 * OR tower2 is destroyed").
 *
 * **Binary references:**
 * - Ctor: `FUN_0042C7B0`
 * - Real ctor: `FUN_0042C930`
 * - Event eval: `FUN_0042CA00`
 * - Singleton global: `dword_63567C`
 */
class CLogic_OR : public CAttributeHandler, public CEventHandler
{
public:
    RWS_MAKENEWCLASS(CLogic_OR);
    RWS_DECLARE_CLASS(CLogic_OR);

    /**
     * @brief Constructor — initializes OR node with condition list.
     * @param numConditions Number of sub-conditions to track.
     */
    CLogic_OR(int numConditions = 2);

    /// Packet constructor required by RWS_MAKENEWCLASS (attributes not parsed yet).
    explicit CLogic_OR(const CAttributePacket&) : CLogic_OR(2) {}

    /**
     * @brief Destructor — clears condition registrations.
     */
    virtual ~CLogic_OR();

    /**
     * @brief Add a sub-condition to the OR list.
     * @param conditionIdx Index identifying the condition.
     * @param state TRUE when the condition is met.
     */
    void AddCondition(int conditionIdx, RwBool state);

    /**
     * @brief Evaluate the OR logic.
     * @return TRUE if any sub-condition is true.
     */
    RwBool Evaluate() const;

    /**
     * @brief Reset all conditions to FALSE.
     */
    void Reset();

private:
    int    m_numConditions;    ///< Number of conditions tracked
    RwBool m_conditions[8];    ///< Condition states (max 8 sub-conditions)
};

} // namespace RWS