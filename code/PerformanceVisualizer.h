#ifndef PERFORMANCEVISUALIZER_H
#define PERFORMANCEVISUALIZER_H

// ============================================================================
// CannonCruise - Performance Visualizer (PerformanceVisualizer.h)
// Original path: D:\Projects\CannonCruisePC\code\PerformanceVisualizer.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// Binary ref: FUN_00498020 - FUN_004981e0 (CPerformanceVisualizer)
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include <rwcore.h>

/**
 * @class CPerformanceVisualizer
 * @brief Debug tool that displays real-time performance info on screen.
 *
 * Integrated with CRealtimeProfiler and CProfilerBehaviour, it draws metrics such as
 * FPS, frame time and system load to the screen through the Im2D (RenderWare 2D primitive)
 * API. Active only in development/debug builds.
 *
 * **Render behavior:**
 * - If `m_bEnabled` is FALSE, `Render()` returns early and draws nothing.
 * - `m_pCharset` (RtCharset) is used for text drawing.
 * - Profile bars are drawn with Im2D primitives.
 *
 * **Binary references:**
 * - Ctor: `FUN_00498020`
 * - Render: `FUN_004980a0` (stub -- body not yet filled in)
 * - HandleEvents: `FUN_00498180`
 */
class CPerformanceVisualizer : public RWS::CEventHandler
{
public:
    /**
     * @brief Default constructor — initializes the charset resource, registers for events.
     */
    CPerformanceVisualizer();

    /**
     * @brief Destructor — releases the charset resource, unregisters event handling.
     */
    virtual ~CPerformanceVisualizer();

    /**
     * @brief Processes incoming performance profile events.
     * @param msg The CMsg event to process.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Draws performance metrics to the screen.
     *
     * Draws nothing if `m_bEnabled` is FALSE.
     * FPS and profile bars are placed at the top of the screen using
     * Im2D primitives and RtCharset.
     *
     * @note Binary address `FUN_004980a0` — body not yet fully filled in (stub).
     */
    void Render();

    /**
     * @brief Enables or disables the visualizer.
     * @param bEnable If TRUE, subsequent Render() calls are active.
     */
    void SetEnabled(RwBool bEnable) { m_bEnabled = bEnable; }

private:
    RwBool m_bEnabled;   ///< Is the visualizer enabled?
    void* m_pCharset;    ///< RtCharset pointer for text drawing
};

#endif // PERFORMANCEVISUALIZER_H
