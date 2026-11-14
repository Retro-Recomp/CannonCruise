#ifndef LOADLEVEL_H
#define LOADLEVEL_H

// ============================================================================
// CannonCruise - Level Loader (LoadLevel.h)
// Original path: D:\Projects\CannonCruisePC\code\LoadLevel.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include <rwcore.h>
#include <string>

class CLoadLevel : public RWS::CEventHandler
{
public:
    CLoadLevel();
    virtual ~CLoadLevel();

    virtual void HandleEvents(const RWS::CMsg& msg);

    // Level load trigger
    void RequestLevel(const char* pLevelName);

private:
    char m_szLevelName[64];
};

#endif // LOADLEVEL_H
