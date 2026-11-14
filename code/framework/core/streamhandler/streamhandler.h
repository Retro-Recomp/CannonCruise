#ifndef RWS_STREAMHANDLER_H
#define RWS_STREAMHANDLER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (streamhandler.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\streamhandler\streamhandler.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00404000 - FUN_004042e0 (RWS::CStreamHandler)
// ============================================================================

#include <rwcore.h>
#include <map>

namespace RWS
{
    /**
     * @typedef ChunkHandlerFunc
     * @brief Callback function pointer that processes a RenderWare stream chunk.
     *
     * Bound to a specific chunk id via `CStreamHandler::RegisterChunkHandler()`.
     * Called when the relevant chunk is encountered while the stream is processed.
     *
     * @param pStream   RenderWare stream to process.
     * @param chunkId   Chunk id.
     * @param chunkSize Chunk size (bytes).
     * @return TRUE if the chunk was processed successfully.
     */
    typedef RwBool (*ChunkHandlerFunc)(RwStream* pStream, RwUInt32 chunkId, RwUInt32 chunkSize);

    /**
     * @class CStreamHandler
     * @brief Chunk manager for the RenderWare stream system.
     *
     * RenderWare binary files (`.dff`, `.bsp`, `.txd` etc.) have a chunk-based
     * format. `CStreamHandler` parses the files by calling a registered
     * handler for each chunk type.
     *
     * **Usage:**
     * ```cpp
     * CStreamHandler::RegisterChunkHandler(rwID_CLUMP, &OnClumpChunk);
     * CStreamHandler::ProcessFile("levels/level01.bsp");
     * ```
     *
     * **Binary references:**
     * - Instance: `FUN_00404000`
     * - ProcessStream: `FUN_00404080`
     * - ProcessFile: `FUN_004040f0`
     * - RegisterChunkHandler: `FUN_00404180`
     */
    class CStreamHandler
    {
    public:
        /// @return Pointer to the singleton CStreamHandler instance
        static CStreamHandler* Instance();
        /// @brief Initializes the stream system
        static void Initialize();
        /// @brief Shuts down the stream system, removing all chunk handlers
        static void Shutdown();

        /**
         * @brief Registers a handler for the given chunk id.
         * @param chunkId  Chunk id (e.g. `rwID_CLUMP`, `rwID_WORLD`).
         * @param pHandler Handler function pointer.
         * @return TRUE if registration succeeded.
         */
        static RwBool RegisterChunkHandler(RwUInt32 chunkId, ChunkHandlerFunc pHandler);

        /**
         * @brief Removes the handler for the given chunk id.
         * @param chunkId Chunk id whose handler should be removed.
         */
        static void UnRegisterChunkHandler(RwUInt32 chunkId);

        /**
         * @brief Processes an open RenderWare stream.
         * @param pStream Stream to process.
         * @return TRUE if the stream was processed successfully.
         */
        static RwBool ProcessStream(RwStream* pStream);

        /**
         * @brief Reads a file from disk and processes its chunks.
         * @param pFilename Path of the file to read.
         * @return TRUE if the file was processed successfully.
         */
        static RwBool ProcessFile(const char* pFilename);

    private:
        CStreamHandler();
        ~CStreamHandler();

        static CStreamHandler* sm_pInstance; ///< Singleton instance pointer

        std::map<RwUInt32, ChunkHandlerFunc> m_ChunkHandlers; ///< ChunkId → handler mapping table
    };
}

#endif // RWS_STREAMHANDLER_H
