// ============================================================================
// CannonCruise - RenderWare Studio Framework (csystemcommands.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\attributehandler\csystemcommands.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
//
// Binary Functions:
//   FUN_00415620 — CSystemCommands::HandleLoadMatrix
//   FUN_00415ac0 — CSystemCommands::HandleSetParent
// ============================================================================

#include "csystemcommands.h"
#include "cattributehandler.h"
#include "RwsResources.h"

#include <cstring>
#include <cassert>
#include <cstring>

namespace RWS
{
    // ========================================================================
    // CSystemCommands::HandleLoadMatrix (FUN_00415620)
    // ASM: 00415620 — Read matrix data and apply it to the frame
    // ========================================================================
    void CSystemCommands::HandleLoadMatrix(RwFrame* pFrame, const CAttributeCommand& attrCmd)
    {
        assert(pFrame != 0 && "Failed PRE-condition: pObject");
        assert(CMD_LoadMatrix == attrCmd.GetCommandId() && "Failed PRE-condition: CMD_LoadMatrix == attrCmd.GetCommandId()");

        const RwMatrix* pMatrix = (const RwMatrix*)attrCmd.GetData();
        if (pMatrix)
        {
            RwMatrix matrixCopy;
            memcpy(&matrixCopy, pMatrix, sizeof(RwMatrix));
            
            // Matris optimizasyonu & normalizasyon (FUN_0050f690 / RwMatrixUpdate)
            RwMatrixUpdate(&matrixCopy);

            // Update the frame transform (FUN_0050e910 / RwFrameTransform)
            RwFrameTransform(pFrame, &matrixCopy, rwCOMBINEREPLACE);
        }
    }

    // ========================================================================
    // CSystemCommands::HandleSetParent (FUN_00415ac0)
    // ASM: 00415ac0 — Link the frame parent hierarchy
    // ========================================================================
    void CSystemCommands::HandleSetParent(RwFrame* pChildFrame, RwFrame* pParentFrame)
    {
        assert(pChildFrame != 0 && "Failed PRE-condition: pSrcObject");
        assert(pParentFrame != 0 && "Failed PRE-condition: pDstObject");

        RwFrameAddChild(pParentFrame, pChildFrame);
    }

    // ========================================================================
    // CSystemCommands::HandleAttachAtomic
    // ========================================================================
    void CSystemCommands::HandleAttachAtomic(RpAtomic* pAtomic, RwFrame* pFrame)
    {
        assert(pAtomic != 0 && "Failed PRE-condition: pAtomic");
        assert(pFrame != 0 && "Failed PRE-condition: pObject");

        RpAtomicSetFrame(pAtomic, pFrame);
    }

    // ========================================================================
    // CSystemCommands::HandleSetFlags
    // ========================================================================
    void CSystemCommands::HandleSetFlags(RpAtomic* pAtomic, RwUInt32 flags)
    {
        assert(pAtomic != 0 && "Failed PRE-condition: pAtomic");

        RpAtomicSetFlags(pAtomic, flags);
    }

    const void* CSystemCommands::ExtractClump(const CAttributePacket& packet)
    {
        for (CAttributeCommandIterator it(packet, "CSystemCommands");
             !it.IsFinished(); ++it)
        {
            if (it->GetCommandId() != 0 || it->GetDataSize() != 16)
            {
                continue;
            }
            const RwsResource* pRes = RwsResources_Find(
                static_cast<const RwUInt8*>(it->GetCommandData()));
            // Resources are listed model first (then collision hull, damaged
            // model, animations), so the first clump is the visible model.
            if (pRes && pRes->embeddedType == rwsRESOURCE_CLUMP)
            {
                return pRes;
            }
        }
        return 0;
    }

    RwBool CSystemCommands::ExtractMatrix(const CAttributePacket& packet,
                                          RwReal outMatrix[16])
    {
        for (CAttributeCommandIterator it(packet, "CSystemCommands");
             !it.IsFinished(); ++it)
        {
            if (it->GetCommandId() == CMD_LoadMatrix &&
                it->GetDataSize() == 16 * sizeof(RwReal))
            {
                std::memcpy(outMatrix, it->GetCommandData(),
                            16 * sizeof(RwReal));
                return TRUE;
            }
        }
        return FALSE;
    }
}
