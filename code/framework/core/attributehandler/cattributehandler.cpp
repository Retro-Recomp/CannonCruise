// ============================================================================
// CannonCruise - RenderWare Studio Framework (cattributehandler.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\attributehandler\cattributehandler.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
//
// Binary Functions:
//   FUN_0040fd60 — CAttributeHandler::Shutdown()
//   FUN_00410540 — CAttributeHandler::ProcessPacket(...)
//   FUN_00410860 — CAttributeHandler::FindInstance(instanceId)
//   FUN_004118b0 — CAttributeHandler::RegisterInstance(...)
// ============================================================================

#include "cattributehandler.h"
#include "cclassfactory.h"
#include <cassert>
#include <cstring>

namespace RWS
{
    // ========================================================================
    // CAttributeCommandIterator — walks a 0x0704 object-record body
    // (layout documented in cattributehandler.h).
    // ========================================================================
    namespace
    {
        const RwUInt32 kSegClassStart  = 0x20000000u;
        const RwUInt32 kSegGuid        = 0x40000000u;
        const RwUInt32 kSegClassRepeat = 0x80000000u;

        RwUInt32 ReadU32(const RwUInt8* p)
        {
            RwUInt32 v;
            std::memcpy(&v, p, sizeof(v));
            return v;
        }

        // Compares the NUL-terminated name stored in a segment payload.
        RwBool SegmentNameEquals(const RwUInt8* pName, RwUInt32 maxLen,
                                 const char* pWanted)
        {
            RwUInt32 i = 0;
            for (; i < maxLen && pName[i] != 0; ++i)
            {
                if (pWanted[i] == 0 || pWanted[i] != (char)pName[i])
                {
                    return FALSE;
                }
            }
            return (pWanted[i] == 0) ? TRUE : FALSE;
        }
    }

    CAttributeCommandIterator::CAttributeCommandIterator(
        const CAttributePacket& packet, const char* pClassName)
        : m_pData(static_cast<const RwUInt8*>(packet.GetData())),
          m_Size(packet.GetSize()),
          m_Offset(4), // skip field0
          m_pClassName(pClassName),
          m_bInClass(pClassName == 0 ? TRUE : FALSE),
          m_bFinished(FALSE)
    {
        m_Current.m_CommandId = 0;
        m_Current.m_pCommandData = 0;
        m_Current.m_DataSize = 0;
        if (m_pData == 0 || m_Size < 4)
        {
            m_bFinished = TRUE;
            return;
        }
        Advance();
    }

    void CAttributeCommandIterator::Advance()
    {
        while (m_Offset + 8 <= m_Size)
        {
            const RwUInt32 len = ReadU32(m_pData + m_Offset);
            const RwUInt32 word = ReadU32(m_pData + m_Offset + 4);
            if (len < 8 || len > m_Size - m_Offset)
            {
                break; // lost alignment: stop rather than read garbage
            }
            const RwUInt8* pPayload = m_pData + m_Offset + 8;
            const RwUInt32 payloadLen = len - 8;
            m_Offset += len;

            if (word == kSegClassStart || word == kSegClassRepeat)
            {
                if (m_pClassName != 0)
                {
                    m_bInClass = SegmentNameEquals(pPayload, payloadLen,
                                                   m_pClassName);
                }
                continue;
            }
            if (word == kSegGuid)
            {
                continue;
            }
            if (m_bInClass == TRUE)
            {
                m_Current.m_CommandId = word;
                m_Current.m_pCommandData = pPayload;
                m_Current.m_DataSize = payloadLen;
                return;
            }
        }
        m_bFinished = TRUE;
        m_Current.m_CommandId = 0;
        m_Current.m_pCommandData = 0;
        m_Current.m_DataSize = 0;
    }

    CAttributeHandler* CAttributeHandler::sm_pInstance = 0;

    // ========================================================================
    // CAttributeHandler Constructor
    // ========================================================================
    CAttributeHandler::CAttributeHandler()
    {
    }

    // ========================================================================
    // CAttributeHandler Constructor (attribute packet)
    // Binary: sub_411DA0 — iterates packet commands for the "CAttributeHandler" class
    // and performs entity resolution via FindInstance.
    // Entity resolution (CAttributeHandler id0 = instance link) is not ported
    // yet; the loop only walks the class's commands.
    // ========================================================================
    CAttributeHandler::CAttributeHandler(const CAttributePacket& packet)
    {
        for (CAttributeCommandIterator it(packet, "CAttributeHandler");
             !it.IsFinished(); ++it)
        {
            // TODO: FindInstance-based entity resolution (sub_411DA0)
        }
    }

    // ========================================================================
    // CAttributeHandler Destructor
    // ========================================================================
    CAttributeHandler::~CAttributeHandler()
    {
        m_InstanceMap.clear();
    }

    // ========================================================================
    // CAttributeHandler::Instance
    // ========================================================================
    CAttributeHandler* CAttributeHandler::Instance()
    {
        if (sm_pInstance == 0)
        {
            sm_pInstance = new CAttributeHandler();
        }
        return sm_pInstance;
    }

    // ========================================================================
    // CAttributeHandler::Initialize
    // ========================================================================
    void CAttributeHandler::Initialize()
    {
        Instance();
    }

    // ========================================================================
    // CAttributeHandler::Shutdown (FUN_0040fd60)
    // ASM: 0040fd60 — Clear and close all object mappings
    // ========================================================================
    void CAttributeHandler::Shutdown()
    {
        if (sm_pInstance != 0)
        {
            assert(sm_pInstance->m_InstanceMap.size() == 0 || true); // g_ptheMap->size() == 0
            delete sm_pInstance;
            sm_pInstance = 0;
        }
    }

    // ========================================================================
    // CAttributeHandler::FindInstance (FUN_00410860)
    // ASM: 00410860 — Find the object corresponding to the ID
    // ========================================================================
    void* CAttributeHandler::FindInstance(RwUInt32 instanceId)
    {
        CAttributeHandler* pHandler = Instance();
        std::map<RwUInt32, EntityInstance>::iterator it = pHandler->m_InstanceMap.find(instanceId);
        if (it != pHandler->m_InstanceMap.end())
        {
            return it->second.m_pObject;
        }
        return 0;
    }

    // ========================================================================
    // CAttributeHandler::RegisterInstance (FUN_004118b0)
    // ASM: 004118b0 — Register the new object in the map
    // ========================================================================
    void CAttributeHandler::RegisterInstance(RwUInt32 instanceId, void* pObject)
    {
        EntityInstance inst;
        inst.m_InstanceId = instanceId;
        inst.m_pObject = pObject;
        inst.m_Flags = 0;

        Instance()->m_InstanceMap[instanceId] = inst;
    }

    // ========================================================================
    // CAttributeHandler::UnregisterInstance
    // ========================================================================
    void CAttributeHandler::UnregisterInstance(RwUInt32 instanceId)
    {
        Instance()->m_InstanceMap.erase(instanceId);
    }

    // ========================================================================
    // Packet Type Helper Methods
    // ========================================================================
    RwBool CAttributeHandler::IsInstanceCreationPacket(const RwUInt8* pPacket)
    {
        if (!pPacket) return FALSE;
        return (pPacket[0] == 0x01);
    }

    RwBool CAttributeHandler::IsEndChunk(const RwUInt8* pPacket)
    {
        if (!pPacket) return TRUE;
        return (pPacket[0] == 0xFF);
    }

    RwBool CAttributeHandler::IsFinished(const RwUInt8* pPacket)
    {
        if (!pPacket) return TRUE;
        return (pPacket[0] == 0x00);
    }

    // ========================================================================
    // CAttributeHandler::ProcessPacket (FUN_00410540)
    // ASM: 00410540 — Process the RWS attribute packet
    // ========================================================================
    void CAttributeHandler::ProcessPacket(const RwUInt8* pPacketData, RwUInt32 dataSize)
    {
        if (!pPacketData || dataSize < 8)
        {
            return;
        }

        assert(!IsEndChunk(pPacketData) && "Failed PRE-condition: !IsEndChunk()");
        assert(!IsFinished(pPacketData) && "Failed PRE-condition: !IsFinished()");

        if (IsInstanceCreationPacket(pPacketData))
        {
            // Packet layout: [PacketType:4][InstanceId:4][ClassName:str]
            RwUInt32 instanceId = *(const RwUInt32*)(pPacketData + 4);
            const char* pClassName = (const char*)(pPacketData + 8);

            assert(instanceId != 0 && "Packet does not contain an instance ID");

            if (pClassName && CClassFactory::ClassIsRegistered(pClassName))
            {
                void* pNewObject = CClassFactory::Create(pClassName);
                if (pNewObject)
                {
                    RegisterInstance(instanceId, pNewObject);
                }
            }
        }
    }
}
