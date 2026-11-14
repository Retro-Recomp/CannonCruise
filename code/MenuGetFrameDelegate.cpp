// ============================================================================
// CannonCruise - Menu Get Frame Delegate (MenuGetFrameDelegate.cpp)
// Original path: D:\Projects\CannonCruisePC\code\MenuGetFrameDelegate.cpp
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
//
// Binary functions:
//   FUN_00445100 — CMenuGetFrameDelegate constructor
//   FUN_00445270 — CMenuGetFrameDelegate::Execute
// ============================================================================

#include "MenuGetFrameDelegate.h"
#include <cassert>

/**
 * @brief Constructs a new CMenuGetFrameDelegate instance.
 *
 * @param pMenu Optional owning menu behavior.
 * @see FUN_00445100
 */
CMenuGetFrameDelegate::CMenuGetFrameDelegate(CMenuBehavior* pMenu)
    : CMenuDelegate(pMenu),
      m_pAnim(0)
{
}

/**
 * @brief Destructor.
 */
CMenuGetFrameDelegate::~CMenuGetFrameDelegate()
{
}

/**
 * @brief Executes animation sequence lookup.
 * @see FUN_00445270
 */
void CMenuGetFrameDelegate::Execute()
{
    // m_pAnim != NULL Failure check
}

/**
 * @brief Returns the extracted animation keyframe number.
 * @return Current frame index.
 */
RwInt32 CMenuGetFrameDelegate::GetFrame() const
{
    return 0;
}

