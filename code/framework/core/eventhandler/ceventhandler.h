#ifndef RWS_CEVENTHANDLER_H
#define RWS_CEVENTHANDLER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (ceventhandler.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\eventhandler\ceventhandler.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00401000 - FUN_004013e0 (RWS::CEventHandler)
// ============================================================================

#include <rwcore.h>
#include <vector>
#include <string>

/**
 * @namespace RWS
 * @brief Namespace hosting all core classes of the RenderWare Studio framework.
 *
 * All game objects of CannonCruise derive from core classes in this namespace such as
 * `CEventHandler` and `CAttributeHandler`. Event-based messaging
 * (`SendMsg` / `HandleEvents`) is the main communication mechanism of the game logic.
 */
namespace RWS
{
    /// Default block size for registered event links
    #define CEVENT_HANDLER_REGISTERED_MSG_DEFAULT_BLOCK_SIZE 128
    /// Default block size for registered messages
    #define CREGISTERED_MSGS_DEFAULT_BLOCK_SIZE 128

    /**
     * @typedef CEventId
     * @brief Event id type — 32-bit unsigned integer.
     *
     * Unique event id returned by `RegisterMsg()`.
     * Fixed system events (iMsgRunningTick etc.) are defined directly;
     * game-specific events are obtained at runtime via `RegisterMsg()`.
     */
    typedef RwUInt32 CEventId;

    /// Tick event published every frame of the game loop
    const CEventId iMsgRunningTick    = 0x0001;
    /// Update event published for the Havok physics step
    const CEventId iMsgPhysicsUpdate  = 0x0002;

    /**
     * @struct CMsg
     * @brief Message structure passed through the event-based messaging system.
     *
     * Delivered to all registered `CEventHandler` instances by `SendMsg()`.
     * The `m_pData` / `pData` field is a generic (void*) pointer to the data
     * specific to the sent event; the receiver casts it to the appropriate type.
     *
     * **Usage example:**
     * ```cpp
     * CMsg msg(iMsgRunningTick, nullptr, 0);
     * CEventHandler::SendMsg(msg);
     * ```
     */
    struct CMsg
    {
        union
        {
            CEventId m_Id;  ///< Event id (C++ style access)
            CEventId Id;    ///< Event id (old style access — backward compatibility)
        };
        union
        {
            const void* m_pData; ///< Pointer to event-specific data (C++ style)
            const void* pData;   ///< Pointer to event-specific data (old style)
        };
        RwUInt32 m_Priority; ///< Message priority (higher priority is processed first)

        /**
         * @brief CMsg constructor.
         * @param id       Event id.
         * @param pData    Event-specific data (may be nullptr).
         * @param priority Message priority (default: 0).
         */
        CMsg(CEventId id = 0, const void* pData = 0, RwUInt32 priority = 0)
            : m_Id(id), m_pData(pData), m_Priority(priority)
        {
        }
    };

    /**
     * @class CEventHandler
     * @brief Base class for the RenderWare Studio event system.
     *
     * All game behavior classes in CannonCruise (CSpeakManager, CAudioManager,
     * CScene, etc.) derive directly or indirectly from `CEventHandler`.
     * This class provides subscribing to events (`LinkMsg`), unsubscribing (`UnLinkMsg`)
     * and handling events (`HandleEvents`).
     *
     * **Lifecycle:**
     * 1. The constructor is called.
     * 2. Subscriptions to the events of interest are made with `LinkMsg()`.
     * 3. Incoming events are handled by overriding `HandleEvents()`.
     * 4. When the destructor is called, `UnLinkAllProcess()` is triggered automatically.
     *
     * **Binary references:**
     * - Dtor: `FUN_00401000`
     * - HandleEvents: `FUN_00401060` (virtual, empty by default)
     * - LinkMsg: `FUN_004010c0`
     * - UnLinkMsg: `FUN_00401180`
     * - SendMsg: `FUN_004012a0` (statik)
     * - RegisterMsg: `FUN_00401300` (statik)
     */
    class CEventHandler
    {
    public:
        /**
         * @brief Default constructor.
         */
        CEventHandler();

        /**
         * @brief Priority-parametric constructor (backward compatibility).
         * @param priority Initial priority (may be unused).
         */
        CEventHandler(RwUInt32);

        /**
         * @brief Destructor — cancels all subscriptions by calling `UnLinkAllProcess()`.
         */
        virtual ~CEventHandler();

        /**
         * @brief Virtual function handling the incoming event.
         * @param msg Message to process.
         *
         * Derived classes override this function and implement their own event logic.
         * The base class implementation does nothing for unknown events.
         */
        virtual void HandleEvents(const CMsg& msg);

        /**
         * @brief HandleEvents overload taking a mutable message reference.
         * @param msg Message to process.
         */
        virtual void HandleEvents(CMsg& msg) { HandleEvents((const CMsg&)msg); }

        /**
         * @brief Subscribes to the given event.
         * @param eventId  Event id to subscribe to.
         * @param pFormat  Optional format string describing the data format.
         * @param priority Event priority (higher value = processed first).
         */
        void LinkMsg(CEventId eventId, const char* pFormat = 0, RwUInt32 priority = 0);

        /**
         * @brief Unsubscribes from the given event.
         * @param eventId Event id to unsubscribe from.
         */
        void UnLinkMsg(CEventId eventId);

        /**
         * @brief Cancels all event subscriptions.
         *
         * Called automatically by the destructor; may also be called manually.
         */
        void UnLinkAllProcess();

        /**
         * @brief Compatibility wrapper for `LinkMsg()`.
         * @param eventId Event id to subscribe to.
         */
        void RegisterForMessage(CEventId eventId) { LinkMsg(eventId); }

        /**
         * @brief Compatibility wrapper for `UnLinkMsg()`.
         * @param eventId Event id to unsubscribe from.
         */
        void UnregisterForMessage(CEventId eventId) { UnLinkMsg(eventId); }

        /**
         * @brief Sends a message to all subscribed `CEventHandler` instances.
         * @param msg Message to send.
         *
         * The message is delivered to subscribers sorted by their `m_Priority` value.
         */
        static void SendMsg(const CMsg& msg);

        /**
         * @brief Registers a new event name and returns a unique id.
         * @param pEventName Event name to register (e.g. "iMsgHit").
         * @param pFormat    Optional data format string.
         * @return CEventId — Unique id of the new event.
         */
        static CEventId RegisterMsg(const char* pEventName, const char* pFormat = 0);

        /**
         * @brief Removes a previously registered event from the system.
         * @param eventId Event id to remove.
         *
         * Binary reference: `loc_40CC60` — deletes the registered event record and its
         * subscription links (called by RenderPipelineShutdown).
         */
        static void UnRegisterMsg(CEventId eventId);

        /**
         * @brief Initializes the event system (called once at application startup).
         */
        static void Initialize();

        /**
         * @brief Shuts down the event system and clears all registered events.
         */
        static void Shutdown();

    protected:
        /**
         * @struct EventLink
         * @brief Internal structure describing an event this object subscribes to.
         */
        struct EventLink
        {
            CEventId   m_EventId;  ///< Subscribed event id
            RwUInt32   m_Priority; ///< Processing priority
            std::string m_Format;  ///< Data format string
        };

        std::vector<EventLink> m_LinkedEvents; ///< All event subscriptions of this object
    };
}

#endif // RWS_CEVENTHANDLER_H
