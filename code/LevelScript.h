#ifndef CANNONCRUISE_LEVELSCRIPT_H
#define CANNONCRUISE_LEVELSCRIPT_H

// ============================================================================
// CannonCruise - Level script (LevelScript.h)
//
// The levels drive their tutorials and missions with named events sent
// between RenderWare Studio behaviours. This module collects those records
// from the level's 0x0704 object records and runs the event graph:
//
//   CShowText          id6 start event -> show text id0 for id1 seconds
//                      (0 = until NEXT is pressed) -> send id7
//   CMissionIndicator  goal marker at its position between id1 and id2
//   CCollisionTrigger  armed between CTrigger id0 and id2; entering sends
//                      CCollisionTrigger id1 + CTrigger id4, leaving id3
//   CEventTrigger      armed between CTrigger id0 and id2; after id0
//                      CEventTrigger id1 events, sends CTrigger id4
//   CFade              id2 -> (fade) -> id3
//   CLoadLevel         id0 -> load level id1 (iMsgPlayerDead = restart)
// ============================================================================

#include "rwcore.h"

struct RwStudioObject;
namespace RWS {
class CAttributePacket;
}

/// Clears all collected script records and runtime state.
void LevelScript_Reset(void);

/// Collects the record if it is one of the script behaviours above.
void LevelScript_CollectObject(const RwStudioObject *pObject,
                               const RWS::CAttributePacket &packet);

/// Prints the collision trigger volumes (diagnostics).
void LevelScript_DumpTriggers(void);

/// Number of collected script records (0: the level has no script).
RwUInt32 LevelScript_RecordCount(void);

/// TRUE if a CLoadLevel moves on to another level (the level's exit).
RwBool LevelScript_HasLevelExit(void);

/// Marks an event as produced by game code (fish pickup, ship death ...).
/// A goal marker whose end event nothing produces is completed by sailing
/// up to it (mine crates and treasure chests are not simulated yet).
void LevelScript_RegisterProducer(const char *pEvent);

/// Sends "iMsgPostSceneCreate" (call once the level is set up).
void LevelScript_Start(void);

/// Queues a named event (processed by LevelScript_Update).
void LevelScript_Send(const char *pEvent);

/// TRUE once the event has been sent in this level.
RwBool LevelScript_WasSent(const char *pEvent);

/**
 * @brief Advances texts, fades and triggers and processes queued events.
 * @param dt          Frame time in seconds.
 * @param shipX,shipZ Player ship position (collision triggers).
 * @param nextPressed NEXT key went down this frame (confirms a text).
 */
void LevelScript_Update(RwReal dt, RwReal shipX, RwReal shipZ,
                        RwBool nextPressed);

/// Draws the current text box (own overlay pass; call between Begin/EndFrame).
void LevelScript_Render(void);

/// Current goal marker (latest active CMissionIndicator), if any.
RwBool LevelScript_GetGoal(RwReal *pX, RwReal *pZ);

/// TRUE once a CLoadLevel fired for a level other than a restart; writes
/// the target name (e.g. "Intro2").
RwBool LevelScript_LevelExitReached(char *pNext, RwUInt32 nextSize);

#endif // CANNONCRUISE_LEVELSCRIPT_H
