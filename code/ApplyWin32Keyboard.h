#pragma once

// ============================================================================
// CannonCruise - Win32 Keyboard Apply (ApplyWin32Keyboard.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyWin32Keyboard.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_00442F00 - win32 keyboard apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyWin32Keyboard
 * @brief Applies win32 keyboard input attributes and configures
 *        keyboard event handling for the game.
 *
 * Handles the Apply logic for win32 keyboard input, reading key
 * mappings, repeat rates, and modifier states from the attribute
 * packet.
 *
 * **Binary references:**
 * - CreateWin32Keyboard: `FUN_00442F00`
 * - ApplyWin32Keyboard: `FUN_00442F60`
 */
class ApplyWin32Keyboard : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyWin32Keyboard);
    RWS_DECLARE_CLASS(ApplyWin32Keyboard);

    /**
     * @brief Constructor — initializes win32 keyboard apply handler.
     * @param packet Attribute packet with keyboard settings.
     */
    ApplyWin32Keyboard(const CAttributePacket& packet);

    virtual ~ApplyWin32Keyboard();

    /**
     * @brief Applies win32 keyboard events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure win32 keyboard.
     * @param packet Packet containing keyboard settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwInt32 m_keyRepeatDelay;    ///< Key repeat delay in milliseconds
    RwInt32 m_keyRepeatRate;     ///< Key repeat rate in characters per second
    RwBool m_bKeyStates[256];   ///< Current state of each keyboard key
    RwInt32 m_modifierKeys;     ///< Active modifier keys (Shift, Ctrl, Alt)
};

} // namespace RWS