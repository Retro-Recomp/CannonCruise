// SDK-independent regression test for RWS::CClassFactory.
// Covers Register / ClassIsRegistered / Create / UnRegister / Shutdown.
#include "attributehandler/cclassfactory.h"
#include "attributehandler/cattributehandler.h"
#include <cstdio>
#include <cstring>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL at line %d: %s\n", __LINE__, #condition); \
    return 1; } } while (0)

namespace
{
    struct FakeEntity
    {
        int marker;
    };

    void* CreateFakeEntity()
    {
        FakeEntity* p = new FakeEntity();
        p->marker = 0x1234;
        return p;
    }

    int g_packetCtorRuns = 0;

    struct FakePacketEntity
    {
        explicit FakePacketEntity(const RWS::CAttributePacket&)
        {
            ++g_packetCtorRuns;
        }
    };

    void* CreateFakePacketEntity(const RWS::CAttributePacket&)
    {
        return new FakePacketEntity(RWS::CAttributePacket());
    }
} // namespace

int main()
{
    using RWS::CClassFactory;

    // -----------------------------------------------------------------
    // Register + ClassIsRegistered
    // -----------------------------------------------------------------
    CClassFactory::Register("FakeEntity", &CreateFakeEntity, sizeof(FakeEntity));
    CHECK(CClassFactory::ClassIsRegistered("FakeEntity") == TRUE);
    CHECK(CClassFactory::ClassIsRegistered("NoSuchClass") == FALSE);

    // -----------------------------------------------------------------
    // Create: factory function is called, object returned
    // -----------------------------------------------------------------
    FakeEntity* pEntity = static_cast<FakeEntity*>(CClassFactory::Create("FakeEntity"));
    CHECK(pEntity != 0);
    CHECK(pEntity->marker == 0x1234);
    delete pEntity;

    // Unregistered class → NULL
    CHECK(CClassFactory::Create("NoSuchClass") == 0);

    // -----------------------------------------------------------------
    // Packet-based registration (ClassPacketCreationFunc path)
    // -----------------------------------------------------------------
    CClassFactory::Register("FakePacketEntity",
                            (RWS::ClassCreationFunc)&CreateFakePacketEntity,
                            sizeof(FakePacketEntity));
    CHECK(CClassFactory::ClassIsRegistered("FakePacketEntity") == TRUE);

    // -----------------------------------------------------------------
    // UnRegister removes a single class
    // -----------------------------------------------------------------
    CClassFactory::UnRegister("FakeEntity");
    CHECK(CClassFactory::ClassIsRegistered("FakeEntity") == FALSE);
    CHECK(CClassFactory::ClassIsRegistered("FakePacketEntity") == TRUE);

    // -----------------------------------------------------------------
    // Shutdown: clears all remaining classes
    // -----------------------------------------------------------------
    CClassFactory::Shutdown();
    CHECK(CClassFactory::ClassIsRegistered("FakePacketEntity") == FALSE);

    std::puts("PASS: class factory register/create/unregister/shutdown");
    return 0;
}
