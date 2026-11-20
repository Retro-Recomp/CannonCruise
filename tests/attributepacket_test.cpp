// ============================================================================
// CannonCruise - attribute packet / CAttributeCommandIterator test
//
// 1) Synthetic packets (no game data): class-block filtering, GUID segments,
//    unfiltered mode, empty and truncated packets.
// 2) With a VIKING01.RWS argument: the first CTowerCatapult record is used as
//    a real packet (layout verified against a hexdump) and the packet factory
//    path of CClassFactory is exercised.
//
// Usage: attributepacket_test [VIKING01.RWS]
// ============================================================================

#include "RwStudioStream.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using RWS::CAttributeCommandIterator;
using RWS::CAttributePacket;

static int s_checks = 0;
static int s_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++s_checks;                                                            \
        if (!(cond)) {                                                         \
            ++s_failures;                                                      \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                \
        }                                                                      \
    } while (0)

// ---- packet builder (same layout as a 0x0704 record body) ----
struct PacketBuilder {
    std::vector<RwUInt8> bytes;
    PacketBuilder() { U32(0); } // field0
    void U32(RwUInt32 v) {
        const RwUInt8 *p = reinterpret_cast<const RwUInt8 *>(&v);
        bytes.insert(bytes.end(), p, p + 4);
    }
    void Item(RwUInt32 word, const void *pData, RwUInt32 len) {
        U32(8 + len);
        U32(word);
        const RwUInt8 *p = static_cast<const RwUInt8 *>(pData);
        bytes.insert(bytes.end(), p, p + len);
    }
    void Name(RwUInt32 flags, const char *pName) {
        std::vector<RwUInt8> s(pName, pName + std::strlen(pName) + 1);
        while (s.size() % 4 != 0) {
            s.push_back(0xBF);
        }
        Item(flags, &s[0], (RwUInt32)s.size());
    }
    void Float(RwUInt32 id, RwReal v) { Item(id, &v, 4); }
    void Int(RwUInt32 id, RwInt32 v) { Item(id, &v, 4); }
    CAttributePacket Packet() const {
        return CAttributePacket(&bytes[0], (RwUInt32)bytes.size());
    }
};

static int Count(const CAttributePacket &packet, const char *pClass) {
    int n = 0;
    for (CAttributeCommandIterator it(packet, pClass); !it.IsFinished(); ++it) {
        ++n;
    }
    return n;
}

static void TestSynthetic() {
    PacketBuilder b;
    const RwUInt8 guid[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    b.Name(0x20000000, "CThing");
    b.Item(0x40000000, guid, 16);         // object GUID: skipped
    b.Name(0x80000000, "CSystemCommands");
    b.Item(0, guid, 16);                  // resource GUID = command 0
    b.Name(0x80000000, "CThing");
    b.Float(0, 2.5f);
    b.Int(1, 7);
    b.Name(0x80000000, "CBase");
    b.Float(0, 9.0f);
    b.Name(0x80000000, "CThing");        // second block of the same class
    b.Int(2, 42);
    const CAttributePacket packet = b.Packet();

    CHECK(Count(packet, "CThing") == 3);
    CHECK(Count(packet, "CBase") == 1);
    CHECK(Count(packet, "CSystemCommands") == 1);
    CHECK(Count(packet, "CMissing") == 0);
    CHECK(Count(packet, 0) == 5); // all commands, segments excluded

    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(CThing));
    CHECK(!it.IsFinished() && it->GetCommandId() == 0 &&
          it->GetDataSize() == 4 &&
          *static_cast<const RwReal *>(it->GetCommandData()) == 2.5f);
    ++it;
    CHECK(!it.IsFinished() && it->GetCommandId() == 1 &&
          *static_cast<const RwInt32 *>(it->GetCommandData()) == 7);
    ++it;
    CHECK(!it.IsFinished() && it->GetCommandId() == 2 &&
          *static_cast<const RwInt32 *>(it->GetCommandData()) == 42);
    ++it;
    CHECK(it.IsFinished());

    CAttributeCommandIterator sys(packet, "CSystemCommands");
    CHECK(!sys.IsFinished() && sys->GetCommandId() == 0 &&
          sys->GetDataSize() == 16 &&
          std::memcmp(sys->GetCommandData(), guid, 16) == 0);

    // Name match is exact: a prefix must not match.
    CHECK(Count(packet, "CThin") == 0);

    // Empty / tiny packets finish immediately.
    CHECK(CAttributeCommandIterator(CAttributePacket(), "CThing").IsFinished());
    const RwUInt8 tiny[2] = {0, 0};
    CHECK(CAttributeCommandIterator(CAttributePacket(tiny, 2), 0).IsFinished());

    // A length running past the end stops the walk instead of overreading.
    PacketBuilder bad;
    bad.Name(0x20000000, "CThing");
    bad.Float(0, 1.0f);
    bad.U32(0x1000); // bogus length
    bad.U32(5);
    CHECK(Count(bad.Packet(), "CThing") == 1);
}

// ---- packet factory path ----
static RwUInt32 s_lastPacketSize = 0;
static void *MakeProbe(const CAttributePacket &packet) {
    s_lastPacketSize = packet.GetSize();
    static int s_dummy = 0;
    return &s_dummy;
}
static void *MakePlain() {
    static int s_plain = 0;
    return &s_plain;
}

static void TestFactory() {
    PacketBuilder b;
    b.Name(0x20000000, "CProbe");
    const CAttributePacket packet = b.Packet();

    RWS::CClassFactory::RegisterPacketFactory("CProbe", &MakeProbe, 16);
    CHECK(RWS::CClassFactory::ClassIsRegistered("CProbe") == TRUE);
    CHECK(RWS::CClassFactory::Create("CProbe", packet) != 0);
    CHECK(s_lastPacketSize == packet.GetSize());
    // No plain factory: the packet-less Create returns NULL.
    CHECK(RWS::CClassFactory::Create("CProbe") == 0);

    // Plain-only class: the packet overload falls back to it.
    RWS::CClassFactory::Register("CPlainOnly", &MakePlain, 16, 1);
    CHECK(RWS::CClassFactory::Create("CPlainOnly", packet) == MakePlain());

    RWS::CClassFactory::UnRegister("CProbe");
    RWS::CClassFactory::UnRegister("CPlainOnly");
}

// ---- real VIKING01 record ----
static void TestViking(const char *pPath) {
    RwStudioFile *pFile = RwStudioFileOpen(pPath);
    CHECK(pFile != 0);
    if (!pFile) {
        return;
    }
    RwStudioAsset asset;
    std::vector<RwUInt8> tower;
    std::vector<RwUInt8> ammo;
    while (RwStudioFileNextAsset(pFile, &asset) == TRUE &&
           (tower.empty() || ammo.empty())) {
        const RwStudioObject *pObj = RwStudioFileGetObject(pFile);
        if (!pObj || pObj->behaviourCount == 0) {
            continue;
        }
        const RwUInt8 *pRaw = 0;
        const RwUInt32 n = RwStudioFileGetObjectRaw(pFile, &pRaw);
        const char *pOwner = pObj->behaviours[0].className;
        if (tower.empty() && std::strcmp(pOwner, "CTowerCatapult") == 0) {
            tower.assign(pRaw, pRaw + n);
        } else if (ammo.empty() && std::strcmp(pOwner, "CAmmoCrate") == 0) {
            ammo.assign(pRaw, pRaw + n);
        }
    }
    RwStudioFileClose(pFile);

    CHECK(tower.size() == 1080); // chunk length from the hexdump
    CHECK(!ammo.empty());
    if (tower.empty()) {
        return;
    }
    const CAttributePacket packet(&tower[0], (RwUInt32)tower.size());

    CHECK(Count(packet, "CTowerCatapult") == 7);    // ids 0..6
    CHECK(Count(packet, "CSystemCommands") == 10);  // 6 resource GUIDs + ids 1..4
    CHECK(Count(packet, "CCannon") == 21);          // ids 0..20
    CHECK(Count(packet, "CEnemyHealthBar") == 10);  // ids 0..9
    CHECK(Count(packet, "CAttributeHandler") == 3);

    std::string deathEvent;
    RwReal f0 = 0.0f;
    for (CAttributeCommandIterator it(packet, "CTowerCatapult"); !it.IsFinished();
         ++it) {
        if (it->GetCommandId() == 0) {
            f0 = *static_cast<const RwReal *>(it->GetCommandData());
        } else if (it->GetCommandId() == 3 && it->GetDataSize() > 4) {
            deathEvent = static_cast<const char *>(it->GetCommandData());
        }
    }
    CHECK(f0 == 4.5f);
    CHECK(deathEvent == "MissionTower1Dead");

    int guids = 0;
    RwBool gotMatrix = FALSE;
    for (CAttributeCommandIterator it(packet, "CSystemCommands"); !it.IsFinished();
         ++it) {
        if (it->GetCommandId() == 0 && it->GetDataSize() == 16) {
            ++guids;
        } else if (it->GetCommandId() == 1 && it->GetDataSize() == 64) {
            RwReal m[16];
            std::memcpy(m, it->GetCommandData(), sizeof(m));
            gotMatrix = (m[0] == 1.0f && m[12] == 100.0f && m[13] == 0.0f &&
                         m[14] == -200.0f) ? TRUE : FALSE;
        }
    }
    CHECK(guids == 6);
    CHECK(gotMatrix == TRUE);

    // The real CAmmoCrate packet factory accepts the record.
    if (!ammo.empty()) {
        const CAttributePacket ammoPacket(&ammo[0], (RwUInt32)ammo.size());
        void *pCrate = RWS::CClassFactory::Create("CAmmoCrate", ammoPacket);
        CHECK(pCrate != 0);
        // (not deleted: CAmmoCrate's destructor is not exported through the
        // factory; process exit reclaims it)
    }
}

int main(int argc, char **argv) {
    TestSynthetic();
    TestFactory();
    if (argc > 1) {
        if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
            RwEngineStart() != TRUE) {
            std::fprintf(stderr, "engine init failed\n");
            return 2;
        }
        TestViking(argv[1]);
        RwEngineStop();
        RwEngineClose();
        RwEngineTerm();
    }
    std::printf("attributepacket_test: %d checks, %d failures\n", s_checks,
                s_failures);
    return s_failures == 0 ? 0 : 1;
}
