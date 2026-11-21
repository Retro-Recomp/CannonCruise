#ifndef RWS_CSYSTEMCOMMANDS_H
#define RWS_CSYSTEMCOMMANDS_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (csystemcommands.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\attributehandler\csystemcommands.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00403400 - FUN_004035a0 (RWS::CSystemCommands)
// ============================================================================

#include <rwcore.h>
#include <rpworld.h>

namespace RWS
{
    /**
     * @enum SystemCommandId
     * @brief System-level attribute command ids.
     *
     * System commands produced by `CAttributeHandler::ProcessPacket()`
     * are identified by these ids before being routed to `CSystemCommands` handlers.
     */
    enum SystemCommandId
    {
        CMD_LoadMatrix   = 0x01, ///< Load the RwFrame transform matrix
        CMD_SetParent    = 0x02, ///< Attach the child frame to the parent frame
        CMD_SetFlags     = 0x03, ///< Set the RpAtomic flags
        CMD_AttachAtomic = 0x04  ///< Attach the atomic to the given frame
    };

    /**
     * @struct CAttributeCommand
     * @brief Structure representing an attribute packet command.
     *
     * It is the command record passed to `CSystemCommands` handlers.
     * Contains a command id, a data size, and a data pointer.
     */
    struct CAttributeCommand
    {
        RwUInt32    m_CommandId; ///< Command id (SystemCommandId value)
        RwUInt32    m_DataSize;  ///< Command data size (bytes)
        const void* m_pData;    ///< Command data pointer

        /// @return Command id
        RwUInt32    GetCommandId() const { return m_CommandId; }
        /// @return Command data pointer
        const void* GetData()      const { return m_pData; }
        /// @return Command data size
        RwUInt32    GetDataSize()  const { return m_DataSize; }
    };

    /**
     * @class CSystemCommands
     * @brief System-level attribute command handlers for core RenderWare objects.
     *
     * `CAttributeHandler::ProcessPacket()` a `CMD_LoadMatrix`, `CMD_SetParent` etc.
     * receives a system command, this class's static methods are called directly.
     * Commands specific to game logic are instead passed through `CAttributeHandler::HandleAttributes()`
     * virtual function to the relevant class.
     *
     * **Binary references:**
     * - HandleLoadMatrix: `FUN_00403400`
     * - HandleSetParent: `FUN_00403460`
     * - HandleAttachAtomic: `FUN_004034c0`
     * - HandleSetFlags: `FUN_00403520`
     */
    class CSystemCommands
    {
    public:
        /**
         * @brief Applies the transform matrix to the RwFrame.
         * @param pFrame   Target frame.
         * @param attrCmd  Command containing the matrix data.
         */
        static void HandleLoadMatrix(RwFrame* pFrame, const CAttributeCommand& attrCmd);

        /**
         * @brief Attaches the child frame to the parent frame.
         * @param pChildFrame  Child frame to attach.
         * @param pParentFrame Parent frame.
         */
        static void HandleSetParent(RwFrame* pChildFrame, RwFrame* pParentFrame);

        /**
         * @brief Attaches the atomic to the given frame.
         * @param pAtomic RpAtomic object to add.
         * @param pFrame  Target RwFrame.
         */
        static void HandleAttachAtomic(RpAtomic* pAtomic, RwFrame* pFrame);

        /**
         * @brief Sets the RpAtomic flags.
         * @param pAtomic Target RpAtomic object.
         * @param flags   Flag value to set.
         */
        static void HandleSetFlags(RpAtomic* pAtomic, RwUInt32 flags);

        /**
         * @brief Extracts the RpClump pointer from the attribute packet.
         * @param packet Attribute packet to parse.
         * @return const void* — Clump data (stub in CannonCruise, always nullptr).
         */
        /**
         * @brief First model (embedded clump resource) referenced by the
         *        packet's CSystemCommands command 0 GUIDs.
         * @return const RwsResource* (see RwsResources.h) or NULL when the
         *         object has no model registered for the current level.
         */
        static const void* ExtractClump(const class CAttributePacket& packet);
        /**
         * @brief Entity transform (CSystemCommands command 1: RwMatrix,
         *        right/up/at/pos as 4x4 floats).
         * @return TRUE if the packet carries one.
         */
        static RwBool ExtractMatrix(const class CAttributePacket& packet,
                                    RwReal outMatrix[16]);

        /**
         * @brief Extracts the RpClump pointer from the message.
         * @param msg Message to parse.
         * @return const void* — Clump data (stub in CannonCruise, always nullptr).
         */
        static const void* ExtractClump(const struct CMsg& msg) { return 0; }
    };
}


#endif // RWS_CSYSTEMCOMMANDS_H
