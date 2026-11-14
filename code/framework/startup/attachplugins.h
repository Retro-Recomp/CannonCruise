#ifndef RWS_ATTACHPLUGINS_H
#define RWS_ATTACHPLUGINS_H

// ============================================================================
// CannonCruise - RenderWare Plugin Attachment Header (attachplugins.h)
// Original file: D:\Projects\CannonCruisePC\code\framework\startup\attachplugins.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// ============================================================================

#include <rwcore.h>

/**
 * @brief Initializes all RenderWare plugins and attaches them to the engine.
 *
 * Must run BEFORE `RwEngineInit()` is called. Ensures that HAnim, Skin, 
 * Collision and other platform-specific (e.g. D3D9) plugins
 * are registered with the system.
 *
 * **Binary ref:** `FUN_00419e40`
 *
 * @return TRUE if the plugins attached successfully. On failure 
 *         the application usually halts via assert.
 */
RwBool AttachPlugins(void);

#endif // RWS_ATTACHPLUGINS_H

