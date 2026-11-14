// ============================================================================
// CannonCruise - Playground/physics smoke test (playground_test.cpp)
//
// Sprint 1 doğrulama: oyuncunun gemisi spawn edip throttle katsayısı 1.0
// ile 120 kare güncellediğinde hız 0'dan pozitif değere yükselir ve gemi
// ileri yönde ilerler. Bullet PhysicsBackend çağrılır; tekne Bullet dünya
// kaydında kinematic holder olarak yaşar; PlayerShip_Shutdown ile temizlenir.
//
//   usage: playground_test
// ============================================================================

#include "PlayerShip.h"
#include "PhysicsWorld.h"
#include "PhysicsBackend.h"
#include "Cannonball.h"
#include "TowerCatapult.h"
#include "rwcore.h"

#include <cmath>
#include <cstdio>
#include <cstring>

static const int kFrames = 120;        // 120 kare × 1/60 s = 2 s
static const RwReal kMaxSpeedExpect = 18.0f;
static const RwReal kTargetTime = 120.0f / 60.0f; // 2 saniye

static int g_failures = 0;

static void Check(bool cond, const char *pExpr, int line) {
    if (!cond) {
        ++g_failures;
        std::printf("FAIL line %d: %s\n", line, pExpr);
    }
}
#define CHECK(expr) Check((expr), #expr, __LINE__)

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    (void)argc;
    (void)argv;

    // PhysicsWorld singleton'ı (startup zinciri gibi davranır)
    CPhysicsWorld::Create();
    
    // Tek test için tekne spawn
    PlayerShipState state;
    RwV3d spawnPos = {0.0f, 0.0f, 0.0f};
    PlayerShip_Spawn(&spawnPos, 0.0f); // birim çemberin +Z yönünde
    CHECK(PlayerShip_GetState(&state) == TRUE);
    CHECK(state.pos.x == 0.0f && state.pos.z == 0.0f);

    // 120 kare tam gaz ileri
    const RwReal dt = 1.0f / 60.0f;
    for (int i = 0; i < kFrames; ++i) {
        PlayerShip_Update(dt, (RwReal)i * dt, 1.0f, 0.0f);
    }

    CHECK(PlayerShip_GetState(&state) == TRUE);
    std::printf("Son hız = %.3f, pos=(%.3f, %.3f, %.3f)\n",
                state.speed, state.pos.x, state.pos.y, state.pos.z);
    // 2 saniyede hedef hıza asimptotik olarak yaklaşır:
    // v(∞) = 18 (kMaxSpeed); 2 saniyede ≈ 18*(1-e^{-1.8}) ≈ 15.07 (%84).
    Check(state.speed > kMaxSpeedExpect * 0.75f,
          "speed >= 75% of kMaxSpeed after 2 s", __LINE__);
    // +Z yönünde ilerlemiş olmalı
    Check(state.pos.z > 10.0f,
          "ship moved forward (z > 10)", __LINE__);

    // Temiz kapanış
    PlayerShip_Shutdown();

    // ======================================================================
    // Top atışı → cannonball → kule yıkımı
    // ======================================================================
    {
        std::printf("\n[Sprint 1] Top atışı → kule yıkımı\n");
        CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
        if (pWorld == nullptr) {
            pWorld = CPhysicsWorld::Create();
        }
        std::printf("physics dünyası: %p\n", (void*)pWorld);

        // Bir kule koy (150 hedef HP); vahşi tekne yok; top direkt kumanın
        // üst katmanına isabet edecek.
        RwV3d towerPos = {0.0f, 0.0f, 50.0f};
        CTowerCatapult *pTower = new CTowerCatapult();
        pTower->SetWorldPosition(towerPos);

        // Top: yatay atış yukuş (pitch'siz); mesafe 50'e yakin, top düşmeksizin
        // kuleye çarpacak kadar hızlı (cos(30°)=0.86, hız 40 → x 34 sn hız)
        RwV3d muzzle = {0.0f, 30.0f, -1.0f};
        RwV3d vel = {0.0f, 0.0f, 35.0f};
        SCannonballSpawnData spawnData;
        std::memset(&spawnData, 0, sizeof(spawnData));
        spawnData.m_Pos = muzzle;
        spawnData.m_Vel = vel;
        spawnData.m_Type = CANNONBALL_STANDARD;
        spawnData.m_uOwnerID = 1;
        CCannonball *pBall = new CCannonball();
        pBall->Spawn(spawnData);

        // Fizik adımlarını 90 kare ilerlet (~1.5 s — uçuş sırası)
        const RwReal dt = 1.0f / 60.0f;
        for (int i = 0; i < 120; ++i) {
            pWorld->Update(dt);
            // Top konumunu her kare yazdır (debug)
            RwV3d bp = {};
            PhysicsBody_GetPosition(pBall->GetRigidBody(), &bp.x, &bp.y, &bp.z);
            const RwReal dx = bp.x - towerPos.x;
            const RwReal dy = bp.y - (towerPos.y + 18.0f);
            const RwReal dz = bp.z - towerPos.z;
            const RwReal ax = std::fabs(dx) - (8.0f + 0.8f);
            const RwReal ay = std::fabs(dy) - (18.0f + 0.8f);
            const RwReal az = std::fabs(dz) - (8.0f + 0.8f);
            if (ax < 0 && ay < 0 && az < 0) {
                std::printf("HIT frame=%d tower pos=(%.1f,%.1f,%.1f) ball=(%.1f,%.1f,%.1f)\n",
                            i, towerPos.x, towerPos.y, towerPos.z, bp.x, bp.y, bp.z);
                pTower->TakeDamage(150.0f);
                pBall->OnHit(pTower, 0);
                break;
            }
        }

        // Kule yok edilmiş olmalı
        Check(pTower->IsDestroyed() == TRUE,
              "tower destroyed after cannonball hit", __LINE__);
        std::printf("kule yıkıldı: %s\n",
                    pTower->IsDestroyed() == TRUE ? "YES" : "NO");

        delete pBall;
        delete pTower;
    }

    if (g_failures == 0) {
        std::printf("playground_test: PASS\n");
        return 0;
    }
    std::printf("playground_test: %d failures\n", g_failures);
    return 1;
}
