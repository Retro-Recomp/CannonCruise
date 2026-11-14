// ============================================================================
// CannonCruise - RenderWare Studio Framework (streamhandler.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\streamhandler\streamhandler.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
//
// Binary Functions:
//   FUN_00408ad0 — CStreamHandler::RegisterChunkHandler
//   FUN_00408e00 — CStreamHandler::ProcessStream
// ============================================================================

#include "streamhandler.h"
#include <cassert>

namespace RWS
{
    CStreamHandler* CStreamHandler::sm_pInstance = 0;

    // ========================================================================
    // CStreamHandler Constructor & Destructor
    // ========================================================================
    CStreamHandler::CStreamHandler()
    {
    }

    CStreamHandler::~CStreamHandler()
    {
        m_ChunkHandlers.clear();
    }

    // ========================================================================
    // CStreamHandler::Instance
    // ========================================================================
    CStreamHandler* CStreamHandler::Instance()
    {
        if (sm_pInstance == 0)
        {
            sm_pInstance = new CStreamHandler();
        }
        return sm_pInstance;
    }

    // ========================================================================
    // CStreamHandler::Initialize / Shutdown
    // ========================================================================
    void CStreamHandler::Initialize()
    {
        Instance();
    }

    void CStreamHandler::Shutdown()
    {
        if (sm_pInstance != 0)
        {
            delete sm_pInstance;
            sm_pInstance = 0;
        }
    }

    // ========================================================================
    // CStreamHandler::RegisterChunkHandler (FUN_00408ad0)
    // ASM: 00408ad0 — Register a handler for a specific chunk ID
    // ========================================================================
    RwBool CStreamHandler::RegisterChunkHandler(RwUInt32 chunkId, ChunkHandlerFunc pHandler)
    {
        CStreamHandler* pStreamHandler = Instance();
        
        std::map<RwUInt32, ChunkHandlerFunc>::iterator it = pStreamHandler->m_ChunkHandlers.find(chunkId);
        assert(it == pStreamHandler->m_ChunkHandlers.end() &&
               "Unable to register chunk handler, a chunk handler for the specified chunk id has already been registered.");

        pStreamHandler->m_ChunkHandlers[chunkId] = pHandler;
        return TRUE;
    }

    // ========================================================================
    // CStreamHandler::UnRegisterChunkHandler
    // ========================================================================
    void CStreamHandler::UnRegisterChunkHandler(RwUInt32 chunkId)
    {
        Instance()->m_ChunkHandlers.erase(chunkId);
    }

    // ========================================================================
    // CStreamHandler::ProcessStream (FUN_00408e00)
    // ASM: 00408e00 — Parse the RenderWare Studio data stream
    // ========================================================================
    RwBool CStreamHandler::ProcessStream(RwStream* pStream)
    {
        assert(pStream != 0 && "Failed PRE-condition: pStream");

        RwChunkHeaderInfo chunkHeader;
        while (RwStreamReadChunkHeaderInfo(pStream, &chunkHeader))
        {
            std::map<RwUInt32, ChunkHandlerFunc>::iterator it = 
                Instance()->m_ChunkHandlers.find(chunkHeader.type);

            if (it != Instance()->m_ChunkHandlers.end() && it->second != 0)
            {
                if (!it->second(pStream, chunkHeader.type, chunkHeader.length))
                {
                    return FALSE;
                }
            }
            else
            {
                // Skip unknown chunks or chunks without a handler
                RwStreamSkip(pStream, chunkHeader.length);
            }
        }

        return TRUE;
    }

    // ========================================================================
    // CStreamHandler::ProcessFile
    // ========================================================================
    RwBool CStreamHandler::ProcessFile(const char* pFilename)
    {
        if (!pFilename) return FALSE;

        RwStream* pStream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, pFilename);
        if (pStream)
        {
            RwBool result = ProcessStream(pStream);
            RwStreamClose(pStream, 0);
            return result;
        }

        return FALSE;
    }
}
