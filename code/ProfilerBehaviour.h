#ifndef PROFILERBEHAVIOUR_H
#define PROFILERBEHAVIOUR_H

// ============================================================================
// CannonCruise - Profiler Behavior (ProfilerBehaviour.h)
// Original path: D:\Projects\CannonCruisePC\code\ProfilerBehaviour.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include <rwcore.h>

class CProfilerBehaviour : public RWS::CEventHandler
{
public:
    CProfilerBehaviour();
    virtual ~CProfilerBehaviour();

    virtual void HandleEvents(const RWS::CMsg& msg);

    void EnableItem(RwUInt32 index);
    void DisableItem(RwUInt32 index);

private:
    RwUInt32 m_uProfileFlags;
};

#endif // PROFILERBEHAVIOUR_H
