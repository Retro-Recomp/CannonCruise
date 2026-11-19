// ============================================================================
// CannonCruise - Logic OR (CLogic_OR.cpp)
// Original path: D:\Projects\CannonCruisePC\code\Logic_OR.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_0042C7B0 (ctor), FUN_0042C930 (real ctor),
//            FUN_0042CA00 (eval), FUN_004053B0 (strlen),
//            FUN_004053F0 (malloc/new)
// ============================================================================

#include "CLogic_OR.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"

namespace RWS
{

CLogic_OR::CLogic_OR(int numConditions)
    : m_numConditions(numConditions)
{
    // Initialise all conditions to FALSE
    for (int i = 0; i < 8 && i < m_numConditions; i++)
        m_conditions[i] = FALSE;

    // Zero-out any extra slots
    for (int i = m_numConditions; i < 8; i++)
        m_conditions[i] = FALSE;
}

CLogic_OR::~CLogic_OR()
{
}

void CLogic_OR::AddCondition(int conditionIdx, RwBool state)
{
    if (conditionIdx >= 0 && conditionIdx < m_numConditions)
        m_conditions[conditionIdx] = state;
}

RwBool CLogic_OR::Evaluate() const
{
    // OR: return TRUE if any sub-condition is TRUE
    for (int i = 0; i < m_numConditions; i++)
    {
        if (m_conditions[i] == TRUE)
            return TRUE;
    }
    return FALSE;
}

void CLogic_OR::Reset()
{
    for (int i = 0; i < m_numConditions; i++)
        m_conditions[i] = FALSE;
}

} // namespace RWS