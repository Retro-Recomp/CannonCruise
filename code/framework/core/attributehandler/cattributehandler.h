#ifndef RWS_CATTRIBUTEHANDLER_H
#define RWS_CATTRIBUTEHANDLER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (cattributehandler.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\attributehandler\cattributehandler.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00402000 - FUN_00402400 (RWS::CAttributeHandler)
// ============================================================================

#include <rwcore.h>
#include <map>
#include <string>
#include <cassert>

#ifndef RWS_ASSERT
/// @brief Assert macro for the RWS framework
#define RWS_ASSERT(cond, msg) assert((cond) && (msg))
#endif

namespace RWS
{
    /**
     * @class CAttributePacket
     * @brief Raw byte packet carrying an object's configuration data.
     *
     * In RenderWare Studio's binary entity system, objects are initialized
     * through this packet structure. `CAttributeHandler::HandleAttributes()`
     * consumes them and parses them with the help of `CAttributeCommandIterator`.
     */
    class CAttributePacket
    {
    public:
        /// @brief Empty packet constructor
        CAttributePacket() : m_pData(0), m_DataSize(0) {}

        /**
         * @brief Creates a packet from raw data.
         * @param pData Raw packet data.
         * @param size  Data size (bytes).
         */
        CAttributePacket(const void* pData, RwUInt32 size) : m_pData(pData), m_DataSize(size) {}

        /// @return Pointer to the raw packet data
        const void* GetData() const { return m_pData; }
        /// @return Packet size (bytes)
        RwUInt32    GetSize() const { return m_DataSize; }

    private:
        const void* m_pData;    ///< Pointer to the raw data
        RwUInt32    m_DataSize; ///< Data size
    };

    /**
     * @struct CAttributeCommandEntry
     * @brief Structure holding an attribute command and its data.
     *
     * Produced by `CAttributeCommandIterator`; each command
     * carries a command id (m_CommandId) and a pointer to that command's data.
     */
    struct CAttributeCommandEntry
    {
        RwUInt32    m_CommandId;     ///< Command id
        const void* m_pCommandData;  ///< Command data pointer

        /// @return Command id
        RwUInt32    GetCommandId()   const { return m_CommandId; }
        /// @return Command data pointer
        const void* GetCommandData() const { return m_pCommandData; }
    };

    /**
     * @class CAttributeCommandIterator
     * @brief Iterator over the commands inside a `CAttributePacket`.
     *
     * Used inside `CAttributeHandler::HandleAttributes()` to walk the packet commands
     * for parsing:
     * ```cpp
     * for (CAttributeCommandIterator it(packet); !it.IsFinished(); ++it)
     * {
     *     if (it->GetCommandId() == CMD_SET_HEALTH)
     *         m_Health = *static_cast<const RwReal*>(it->GetCommandData());
     * }
     * ```
     */
    class CAttributeCommandIterator
    {
    public:
        /**
         * @brief Initializes the iterator.
         * @param packet     Packet to parse.
         * @param classIndex Index of the target class within the packet.
         */
        CAttributeCommandIterator(const CAttributePacket& packet, RwUInt32 classIndex = 0)
            : m_bFinished(true)
        {
            m_Current.m_CommandId = 0;
            m_Current.m_pCommandData = 0;
        }

        /// @return TRUE if all commands have been processed
        RwBool IsFinished() const { return m_bFinished; }
        /// @brief Advances to the next command
        void operator++() { m_bFinished = true; }
        /// @return Pointer to the current command entry
        const CAttributeCommandEntry* operator->() const { return &m_Current; }

    private:
        RwBool               m_bFinished; ///< Iterator finished flag
        CAttributeCommandEntry m_Current; ///< Current command entry
    };

    /**
     * @class CAttributeHandler
     * @brief Base class that processes object configuration attributes.
     *
     * All configurable game objects in CannonCruise (CShip, CCannon,
     * CSpeakManager etc.) derive from this class to become compatible with RenderWare Studio's
     * binary entity system.
     *
     * **Usage flow:**
     * 1. `CAttributeHandler::Initialize()` is called at application startup.
     * 2. Entity packets are processed via `ProcessPacket()`; new object instances
     *    are created through `CClassFactory` and registered via `RegisterInstance()`.
     * 3. Each object receives its own attributes by overriding `HandleAttributes()`.
     *
     * **Binary references:**
     * - Instance: `FUN_00402000`
     * - ProcessPacket: `FUN_00402100`
     * - FindInstance: `FUN_004021c0`
     * - RegisterInstance: `FUN_00402240`
     */
    class CAttributeHandler
    {
    public:
        /**
         * @struct EntityInstance
         * @brief Structure describing a registered object instance.
         */
        struct EntityInstance
        {
            RwUInt32 m_InstanceId; ///< Unique instance id
            void*    m_pObject;    ///< Object pointer
            RwUInt32 m_Flags;      ///< Instance flags
        };

        /// @brief Default constructor
        CAttributeHandler();
        /**
         * @brief Constructor taking an attribute packet (called by CClassFactory).
         * @param packet Initial attribute packet.
         */
        CAttributeHandler(const CAttributePacket& packet);
        /// @brief Virtual destructor
        virtual ~CAttributeHandler();

        /**
         * @brief Virtual function that processes the attribute packet.
         * @param packet Attribute packet to process.
         *
         * Derived classes override this function and
         * `CAttributeCommandIterator` to parse the packet commands.
         */
        virtual void HandleAttributes(const CAttributePacket& packet) {}

        /// @return Pointer to the singleton CAttributeHandler instance
        static CAttributeHandler* Instance();
        /// @brief Starts the attribute system
        static void Initialize();
        /// @brief Shuts down the attribute system
        static void Shutdown();

        /**
         * @brief Processes raw packet data and creates/configures objects.
         * @param pPacketData Raw packet data.
         * @param dataSize    Data size (bytes).
         */
        static void ProcessPacket(const RwUInt8* pPacketData, RwUInt32 dataSize);

        /**
         * @brief Looks up a registered object instance by id.
         * @param instanceId Instance id to look up.
         * @return void* — Object pointer or nullptr.
         */
        static void* FindInstance(RwUInt32 instanceId);

        /**
         * @brief Registers an object instance with the system.
         * @param instanceId Instance id.
         * @param pObject    Object pointer.
         */
        static void RegisterInstance(RwUInt32 instanceId, void* pObject);

        /**
         * @brief Removes an object instance from the system.
         * @param instanceId Instance id to remove.
         */
        static void UnregisterInstance(RwUInt32 instanceId);

        /**
         * @brief Checks whether the packet is an object creation packet.
         * @param pPacket Raw packet data.
         * @return TRUE if the packet creates a new object.
         */
        static RwBool IsInstanceCreationPacket(const RwUInt8* pPacket);
        /**
         * @brief Checks whether the packet is an end chunk.
         * @param pPacket Raw packet data.
         * @return TRUE if no new chunk can start after this packet.
         */
        static RwBool IsEndChunk(const RwUInt8* pPacket);
        /**
         * @brief Checks whether all packets have been processed.
         * @param pPacket Raw packet data.
         * @return TRUE if processing is complete.
         */
        static RwBool IsFinished(const RwUInt8* pPacket);

    private:
        static CAttributeHandler* sm_pInstance; ///< Singleton instance pointer

        /// @brief Instance id → EntityInstance mapping table (g_ptheMap / DAT_00635650)
        std::map<RwUInt32, EntityInstance> m_InstanceMap;
    };
}

#include "cclassfactory.h"

#endif // RWS_CATTRIBUTEHANDLER_H


