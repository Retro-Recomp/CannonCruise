// ============================================================================
// CannonCruise - Physics Backend Test (physics_test.cpp)
// Verified binary parameters (CC_ida.asm):
//   - gravity y = -9.8          (sub_55E6D0[+0x04], 0C11CCCCDh)
//   - test step 1/60            (port-side input; original world-update
//                                cadence is still being traced)
//   - friction 0.6 / restitution 0.3 (sub_55E6D0[+0x44]/[+0x58])
// Free fall, ground-collision friction stop, trigger overlap,
// impulse and shape ownership paths.
// ============================================================================

#include "PhysicsBackend.h"

#include <cmath>
#include <cstdio>
#include <limits>

static int g_checks = 0;
static int g_failures = 0;

static void Check(bool cond, const char *pExpr, int line) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::printf("FAIL line %d: %s\n", line, pExpr);
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

static bool Near(float a, float b, float eps) {
    return std::fabs(a - b) <= eps;
}

static void TestGravityDefault() {
    PhysicsWorldHandle *pWorld = PhysicsWorld_Create();
    CHECK(pWorld != 0);
    float gx = 0, gy = 0, gz = 0;
    PhysicsWorld_GetGravity(pWorld, &gx, &gy, &gz);
    CHECK(Near(gx, 0.0f, 1e-6f));
    CHECK(Near(gy, -9.8f, 1e-6f)); // sub_55E6D0[+0x04]
    CHECK(Near(gz, 0.0f, 1e-6f));
    PhysicsWorld_Destroy(pWorld);
}

static void TestFreeFall() {
    // free fall — 1 second (60 steps × 1/60): v ≈ -9.8, y ≈ 10 - 4.984
    // (semi-implicit Euler: y = y0 - g·dt²·n(n+1)/2)
    PhysicsWorldHandle *pWorld = PhysicsWorld_Create();

    PhysicsBodyBoxDesc desc;
    desc.halfExtents[0] = desc.halfExtents[1] = desc.halfExtents[2] = 0.5f;
    desc.mass = 1.0f;
    desc.friction = -1.0f;  // default (0.6)
    desc.restitution = -1.0f;
    desc.pos[0] = 0.0f;
    desc.pos[1] = 10.0f;
    desc.pos[2] = 0.0f;
    for (int i = 0; i < 9; ++i) {
        desc.rot3x3[i] = 0.0f;
    }
    desc.rot3x3[0] = desc.rot3x3[4] = desc.rot3x3[8] = 1.0f;

    void *pBody = PhysicsBody_CreateBox(&desc);
    CHECK(pBody != 0);
    PhysicsWorld_AddRigidBody(pWorld, pBody);

    const float dt = 1.0f / 60.0f; // fixed step for this backend test
    for (int i = 0; i < 60; ++i) {
        PhysicsWorld_Step(pWorld, dt);
    }

    float x = 0, y = 0, z = 0;
    PhysicsBody_GetPosition(pBody, &x, &y, &z);
    const float expectedY = 10.0f - 9.8f * dt * dt * (60.0f * 61.0f / 2.0f);
    CHECK(Near(y, expectedY, 0.02f));

    float vx = 0, vy = 0, vz = 0;
    PhysicsBody_GetLinearVelocity(pBody, &vx, &vy, &vz);
    CHECK(Near(vy, -9.8f, 0.02f)); // gravity velocity accumulated over 1 second

    PhysicsWorld_RemoveRigidBody(pWorld, pBody);
    PhysicsBody_Destroy(pBody);
    PhysicsWorld_Destroy(pWorld);
}

static void TestGroundRestAndFriction() {
    // ground (static box, top surface y=0) + dynamic box + friction 0.6
    PhysicsWorldHandle *pWorld = PhysicsWorld_Create();

    float groundHalf[3] = {50.0f, 0.5f, 50.0f};
    void *pGround = PhysicsShape_CreateBox(groundHalf);
    CHECK(pGround != 0);
    float groundPos[3] = {0.0f, -0.5f, 0.0f};
    void *pGroundBody = PhysicsBody_CreateWithShape(pGround, 0.0f, groundPos, 0);
    CHECK(pGroundBody != 0);
    PhysicsWorld_AddRigidBody(pWorld, pGroundBody);

    PhysicsBodyBoxDesc desc;
    desc.halfExtents[0] = desc.halfExtents[1] = desc.halfExtents[2] = 0.5f;
    desc.mass = 1.0f;
    desc.friction = -1.0f;   // sub_55E6D0 default 0.6
    desc.restitution = -1.0f; // default 0.3 → settles with a slight bounce
    desc.pos[0] = 0.0f;
    desc.pos[1] = 5.0f;
    desc.pos[2] = 0.0f;
    for (int i = 0; i < 9; ++i) {
        desc.rot3x3[i] = 0.0f;
    }
    desc.rot3x3[0] = desc.rot3x3[4] = desc.rot3x3[8] = 1.0f;
    void *pBox = PhysicsBody_CreateBox(&desc);
    PhysicsWorld_AddRigidBody(pWorld, pBox);

    // initial velocity toward +x — friction 0.6 → a = -μ·g = -5.88 m/s²
    PhysicsBody_SetLinearVelocity(pBox, 5.0f, 0.0f, 0.0f);

    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 300; ++i) { // 5 seconds
        PhysicsWorld_Step(pWorld, dt);
    }

    float y = 0;
    PhysicsBody_GetPosition(pBox, 0, &y, 0);
    // half extent 0.5 → rest ~y=0.5 (above ground); band with bounce damping
    CHECK(y > 0.4f && y < 1.2f);

    float vx = 0;
    PhysicsBody_GetLinearVelocity(pBox, &vx, 0, 0);
    CHECK(std::fabs(vx) < 0.5f); // friction stopped it (friction 0.6 verification)

    PhysicsWorld_RemoveRigidBody(pWorld, pBox);
    PhysicsWorld_RemoveRigidBody(pWorld, pGroundBody);
    PhysicsBody_Destroy(pBox);
    PhysicsBody_Destroy(pGroundBody);
    PhysicsShape_Destroy(pGround); // CreateWithShape did not take ownership → caller destroys
    PhysicsWorld_Destroy(pWorld);
}

static void TestImpulse() {
    PhysicsWorldHandle *pWorld = PhysicsWorld_Create();

    PhysicsBodyBoxDesc desc;
    desc.halfExtents[0] = desc.halfExtents[1] = desc.halfExtents[2] = 0.5f;
    desc.mass = 1.0f;
    desc.friction = -1.0f;
    desc.restitution = -1.0f;
    desc.pos[0] = desc.pos[1] = desc.pos[2] = 0.0f;
    for (int i = 0; i < 9; ++i) {
        desc.rot3x3[i] = 0.0f;
    }
    desc.rot3x3[0] = desc.rot3x3[4] = desc.rot3x3[8] = 1.0f;
    void *pBody = PhysicsBody_CreateBox(&desc);
    PhysicsWorld_AddRigidBody(pWorld, pBody);

    // cannonball-like vertical impulse (mass 1) → Δv = J/m = 12
    PhysicsBody_ApplyCentralImpulse(pBody, 0.0f, 12.0f, 0.0f);
    float vy = 0;
    PhysicsBody_GetLinearVelocity(pBody, 0, &vy, 0);
    CHECK(Near(vy, 12.0f, 0.01f));

    PhysicsWorld_RemoveRigidBody(pWorld, pBody);
    PhysicsBody_Destroy(pBody);
    PhysicsWorld_Destroy(pWorld);
}

static void TestTriggerOverlap() {
    // hkpPhantom → btGhostObject mapping: body enters the trigger box
    PhysicsWorldHandle *pWorld = PhysicsWorld_Create();

    float half[3] = {2.0f, 2.0f, 2.0f};
    float pos[3] = {0.0f, 0.0f, 0.0f};
    void *pTrigger = PhysicsTrigger_CreateBox(half, pos);
    CHECK(pTrigger != 0);
    PhysicsWorld_AddTrigger(pWorld, pTrigger);

    PhysicsBodyBoxDesc desc;
    desc.halfExtents[0] = desc.halfExtents[1] = desc.halfExtents[2] = 0.5f;
    desc.mass = 1.0f;
    desc.friction = -1.0f;
    desc.restitution = -1.0f;
    desc.pos[0] = desc.pos[1] = desc.pos[2] = 0.0f; // inside the trigger
    for (int i = 0; i < 9; ++i) {
        desc.rot3x3[i] = 0.0f;
    }
    desc.rot3x3[0] = desc.rot3x3[4] = desc.rot3x3[8] = 1.0f;
    void *pBody = PhysicsBody_CreateBox(&desc);
    PhysicsWorld_AddRigidBody(pWorld, pBody);

    PhysicsWorld_Step(pWorld, 1.0f / 60.0f); // update broadphase

    void *pHits[8] = {0};
    const RwInt32 n = PhysicsTrigger_GetOverlaps(pTrigger, pHits, 8);
    CHECK(n == 1);
    CHECK(pHits[0] == pBody);

    PhysicsWorld_RemoveRigidBody(pWorld, pBody);
    PhysicsWorld_RemoveTrigger(pWorld, pTrigger);
    PhysicsBody_Destroy(pBody);
    PhysicsTrigger_Destroy(pTrigger);
    PhysicsWorld_Destroy(pWorld);
}

static void TestTriMeshStatic() {
    // static triangle mesh (ground tri) + dynamic box resting on top
    static const float verts[9] = {
        -10.0f, 0.0f, -10.0f,
         10.0f, 0.0f, -10.0f,
         0.0f,  0.0f,  10.0f,
    };
    static const RwInt32 indices[3] = {0, 1, 2};

    PhysicsTriMeshDesc mesh;
    mesh.pVertices = verts;
    mesh.vertexCount = 3;
    mesh.vertexStride = 3 * static_cast<RwInt32>(sizeof(RwReal));
    mesh.pIndices = indices;
    mesh.indexCount = 3;
    mesh.pos[0] = mesh.pos[1] = mesh.pos[2] = 0.0f;

    PhysicsWorldHandle *pWorld = PhysicsWorld_Create();
    void *pMeshBody = PhysicsBody_CreateTriMesh(&mesh);
    CHECK(pMeshBody != 0);
    PhysicsWorld_AddRigidBody(pWorld, pMeshBody);

    PhysicsBodyBoxDesc desc;
    desc.halfExtents[0] = desc.halfExtents[1] = desc.halfExtents[2] = 0.5f;
    desc.mass = 1.0f;
    desc.friction = -1.0f;
    desc.restitution = -1.0f;
    desc.pos[0] = 0.0f;
    desc.pos[1] = 3.0f;
    desc.pos[2] = 0.0f; // on the mesh plane y=0
    for (int i = 0; i < 9; ++i) {
        desc.rot3x3[i] = 0.0f;
    }
    desc.rot3x3[0] = desc.rot3x3[4] = desc.rot3x3[8] = 1.0f;
    void *pBox = PhysicsBody_CreateBox(&desc);
    PhysicsWorld_AddRigidBody(pWorld, pBox);

    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 240; ++i) { // 4 seconds
        PhysicsWorld_Step(pWorld, dt);
    }

    float y = 0;
    PhysicsBody_GetPosition(pBox, 0, &y, 0);
    CHECK(y > 0.3f && y < 1.2f); // settled on the mesh (y≈0.5)

    PhysicsWorld_RemoveRigidBody(pWorld, pBox);
    PhysicsWorld_RemoveRigidBody(pWorld, pMeshBody);
    PhysicsBody_Destroy(pBox);
    PhysicsBody_Destroy(pMeshBody);
    PhysicsWorld_Destroy(pWorld);
}

static void TestInvalidInputs() {
    const RwReal validPos[3] = {0.0f, 0.0f, 0.0f};
    const RwReal validHalf[3] = {1.0f, 1.0f, 1.0f};
    const RwReal zeroHalf[3] = {0.0f, 1.0f, 1.0f};
    CHECK(PhysicsShape_CreateBox(0) == 0);
    CHECK(PhysicsShape_CreateBox(zeroHalf) == 0);
    CHECK(PhysicsShape_CreateSphere(
              std::numeric_limits<RwReal>::quiet_NaN()) == 0);
    CHECK(PhysicsTrigger_CreateBox(0, validPos) == 0);
    CHECK(PhysicsTrigger_CreateBox(validHalf, 0) == 0);
    CHECK(PhysicsTrigger_CreateBox(zeroHalf, validPos) == 0);

    static const RwReal vertices[9] = {
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };
    CHECK(PhysicsShape_CreateConvexHull(
              vertices, 2, 3 * static_cast<RwInt32>(sizeof(RwReal))) == 0);
    static const RwInt32 badIndices[3] = {0, 1, 3};
    static const RwInt32 incompleteIndices[4] = {0, 1, 2, 0};
    PhysicsTriMeshDesc mesh = {};
    mesh.pVertices = vertices;
    mesh.vertexCount = 3;
    mesh.vertexStride = 3 * static_cast<RwInt32>(sizeof(RwReal));
    mesh.pIndices = badIndices;
    mesh.indexCount = 3;
    mesh.pos[0] = mesh.pos[1] = mesh.pos[2] = 0.0f;
    CHECK(PhysicsBody_CreateTriMesh(&mesh) == 0);
    CHECK(PhysicsShape_CreateTriMesh(&mesh) == 0);

    mesh.pIndices = incompleteIndices;
    mesh.indexCount = 4;
    CHECK(PhysicsBody_CreateTriMesh(&mesh) == 0);
    mesh.indexCount = 3;
    mesh.vertexStride = static_cast<RwInt32>(sizeof(RwReal));
    CHECK(PhysicsShape_CreateTriMesh(&mesh) == 0);
    mesh.vertexStride = 3 * static_cast<RwInt32>(sizeof(RwReal)) + 1;
    CHECK(PhysicsShape_CreateTriMesh(&mesh) == 0);

    RwReal vx = 11.0f;
    RwReal vy = 12.0f;
    RwReal vz = 13.0f;
    PhysicsBody_GetLinearVelocity(0, &vx, &vy, &vz);
    CHECK(vx == 11.0f && vy == 12.0f && vz == 13.0f);

    PhysicsBodyBoxDesc box = {};
    box.halfExtents[0] = box.halfExtents[1] = box.halfExtents[2] = 0.5f;
    box.mass = 0.0f;
    box.pos[0] = box.pos[1] = box.pos[2] = 0.0f;
    box.friction = -1.0f;
    box.restitution = -1.0f;
    void *pBody = PhysicsBody_CreateBox(&box);
    CHECK(pBody != 0);
    RwReal rotation[9] = {};
    PhysicsBody_GetRotation(pBody, rotation);
    CHECK(Near(rotation[0], 1.0f, 1e-6f));
    CHECK(Near(rotation[4], 1.0f, 1e-6f));
    CHECK(Near(rotation[8], 1.0f, 1e-6f));
    CHECK(Near(rotation[1], 0.0f, 1e-6f));
    CHECK(Near(rotation[3], 0.0f, 1e-6f));
    CHECK(Near(rotation[6], 0.0f, 1e-6f));
    CHECK(Near(rotation[7], 0.0f, 1e-6f));
    CHECK(Near(rotation[2], 0.0f, 1e-6f));
    CHECK(Near(rotation[5], 0.0f, 1e-6f));
    PhysicsBody_Destroy(pBody);
}

int main() {
    TestGravityDefault();
    TestFreeFall();
    TestGroundRestAndFriction();
    TestImpulse();
    TestTriggerOverlap();
    TestTriMeshStatic();
    TestInvalidInputs();

    std::printf("physics_test: %d checks, %d failures\n", g_checks,
                g_failures);
    return g_failures == 0 ? 0 : 1;
}
