// ============================================================================
// CannonCruise - Menu Get Time Delegate (MenuGetTimeDelegate.cpp)
// Original path: D:\Projects\CannonCruisePC\code\MenuGetTimeDelegate.cpp
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
//
// Binary functions:
//   FUN_004455e0 — CMenuGetTimeDelegate constructor
//   FUN_00445750 — CMenuGetTimeDelegate::Execute
// ============================================================================

#include "MenuGetTimeDelegate.h"
#include <cassert>

/**
 * @brief Constructs a new CMenuGetTimeDelegate instance.
 *
 * @param pMenu Optional owning menu behavior.
 * @see FUN_004455e0
 */
CMenuGetTimeDelegate::CMenuGetTimeDelegate(CMenuBehavior* pMenu)
    : CMenuDelegate(pMenu),
      m_pAnim(0)
{
}

/**
 * @brief Destructor.
 */
CMenuGetTimeDelegate::~CMenuGetTimeDelegate()
{
}

/**
 * @brief Executes animation track position query.
 * @see FUN_00445750
 */
void CMenuGetTimeDelegate::Execute()
{
}

/**
 * @brief Returns the extracted animation playback timestamp.
 * @return Playback time in seconds.
 */
RwReal CMenuGetTimeDelegate::GetTime() const
{
    return 0.0f;
}

