// ============================================================================
// CannonCruise - RenderWare Studio Framework (cresourcemanager.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\resourcemanager\cresourcemanager.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
//
// Binary Functions:
//   FUN_004059a0 — CResourceManager::FindResource
//   FUN_00405ef0 — CResourceManager::RegisterResource
//   FUN_00406360 — CResourceManager::UnRegisterResource
//   FUN_00406680 — CResourceManager::UnRegisterAll
// ============================================================================

#include "cresourcemanager.h"
#include <cassert>

namespace RWS
{
    CResourceManager* CResourceManager::sm_pInstance = 0;

    // ========================================================================
    // CResourceManager Constructor & Destructor
    // ========================================================================
    CResourceManager::CResourceManager()
    {
    }

    CResourceManager::~CResourceManager()
    {
        UnRegisterAll();
    }

    // ========================================================================
    // CResourceManager::Instance
    // ========================================================================
    CResourceManager* CResourceManager::Instance()
    {
        if (sm_pInstance == 0)
        {
            sm_pInstance = new CResourceManager();
        }
        return sm_pInstance;
    }

    // ========================================================================
    // CResourceManager::Initialize / Shutdown
    // ========================================================================
    void CResourceManager::Initialize()
    {
        Instance();
    }

    void CResourceManager::Shutdown()
    {
        if (sm_pInstance != 0)
        {
            delete sm_pInstance;
            sm_pInstance = 0;
        }
    }

    // ========================================================================
    // CResourceManager::FindResource (FUN_004059a0)
    // ASM: 004059a0 — Find the resource by its ID and type
    // ========================================================================
    void* CResourceManager::FindResource(const char* pResId, const char* pStrType)
    {
        assert(pResId != 0 && "Failed PRE-condition: pResId");

        CResourceManager* pManager = Instance();
        std::map<std::string, CResource>::iterator it = pManager->m_Resources.find(pResId);
        if (it != pManager->m_Resources.end())
        {
            if (pStrType != 0)
            {
                assert(pStrType != 0 && "Failed PRE-condition: pStrType");
                if (it->second.m_Type == pStrType)
                {
                    return it->second.m_pData;
                }
                return 0;
            }
            return it->second.m_pData;
        }

        return 0;
    }

    // ========================================================================
    // CResourceManager::RegisterResource (FUN_00405ef0)
    // ASM: 00405ef0 — Register a new resource with the manager
    // ========================================================================
    RwBool CResourceManager::RegisterResource(const char* pResId, const char* pStrType, const char* pStrName, void* pData)
    {
        assert(pResId != 0 && "Failed PRE-condition: pResId");
        assert(pStrType != 0 && "Failed PRE-condition: pStrType");
        assert(pStrName != 0 && "Failed PRE-condition: pStrName");

        CResource res;
        res.m_Type = pStrType;
        res.m_Name = pStrName;
        res.m_pData = pData;
        res.m_RefCount = 1;

        Instance()->m_Resources[pResId] = res;
        return TRUE;
    }

    // ========================================================================
    // CResourceManager::UnRegisterResource (FUN_00406360)
    // ASM: 00406360 — Remove the resource's registration
    // ========================================================================
    void CResourceManager::UnRegisterResource(const char* pResId)
    {
        if (pResId)
        {
            Instance()->m_Resources.erase(pResId);
        }
    }

    // ========================================================================
    // CResourceManager::UnRegisterAll (FUN_00406680)
    // ASM: 00406680 — Clear all resource registrations
    // ========================================================================
    void CResourceManager::UnRegisterAll()
    {
        Instance()->m_Resources.clear();
    }
}
