#ifndef RT2DMAESTRO_H
#define RT2DMAESTRO_H

// ============================================================================
// CannonCruise - Rt2dMaestro real implementation (Rt2dMaestro.h)
//
// Menu scene manager created from the Menu.anm stream: parsed node
// table + selection state + Im2D placeholder render (frame + selection bar).
// Feeds CMenuBehavior's m_pMaestro (FUN_00443800/FUN_00443c20).
// ============================================================================

#include "Rt2dMaestroStream.h"
#include <rt2danim.h>

struct Rt2dMaestro
{
    Rt2dMaestroStreamInfo info;   ///< parsed stream
    RwReal  time;                 ///< seconds -- accumulated by Update
    RwInt32 selection;            ///< info.nodes[] index (active command)
    RwInt32 commandCount;         ///< type=6 command count (menu items)
    RwInt32 commands[rwrt2dMAX_NODES]; ///< command node indices
};

#endif // RT2DMAESTRO_H
