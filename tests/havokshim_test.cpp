// ============================================================================
// CannonCruise - Havok 2.2.1 Shim Test (havokshim_test.cpp)
//
// This test verifies the REAL implementation (havokimpl.cpp — built on the
// Bullet engine) of the Havok 2.2.1 API surface in code/havok/*.h.
//
// Two dimensions verified:
//   1) BINARY FIDELITY — class names faithful to Havok 2.2.1 ("hkWorld",
//      "hkRigidBody"); NO "hkp" prefix (that prefix is Havok 4+). The
//      reverse-engineered game code speaks the names the original
//      binary calls.
//   2) PHYSICS BEHAVIOR — sub_55E6D0 defaults (gravity -9.8,
//      friction 0.6, restitution 0.3) + stepDeltaTime (FUN_0043a040)
//      free-fall/contact/phantom overlap paths.
// ============================================================================

#include "havok.h"

#include <cmath>
#include <cstdio>
#include <string>

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

// ---------------------------------------------------------------------------
// 1) Binary fidelity — 2.2.1 naming (NO hkp prefix)
// ---------------------------------------------------------------------------
static void TestNamingFidelity() {
    hkWorld world;
    hkSphereShape sphere(1.0f);
    hkVector3 mass(1.0f, 1.0f, 1.0f);
    hkRigidBody body(mass, &sphere);

    CHECK(world.getClass() != 0);
    CHECK(body.getClass() != 0);

    // Havok 2.x naming — the "hkp" prefix must definitely be absent
    CHECK(world.getClass()->getName() != 0);
    CHECK(std::string(world.getClass()->getName()) == "hkWorld");
    CHECK(std::string(body.getClass()->getName()) == "hkRigidBody");
}

// ---------------------------------------------------------------------------
// 2) Shape types + AABB (2.2.1 enum order)
// ---------------------------------------------------------------------------
static void TestShapes() {
    // TYPE_SPHERE=0, TYPE_CAPSULE=1, TYPE_TRIANGLE=2, TYPE_BOX=3,
    // TYPE_CONVEX_VERTICES=4, TYPE_MESH=5
    hkSphereShape sphere(2.0f);
    CHECK(sphere.getType() == hkShape::TYPE_SPHERE);
    CHECK(Near(sphere.getRadius(), 2.0f, 1e-6f));
    hkAabb aabb;
    sphere.getAabb(aabb);
    CHECK(Near(aabb.m_max.x, 2.0f, 1e-5f));
    CHECK(Near(aabb.m_min.x, -2.0f, 1e-5f));
    CHECK(sphere.getBulletShape() != 0);

    hkBoxShape box(hkVector3(1.0f, 2.0f, 3.0f));
    CHECK(box.getType() == hkShape::TYPE_BOX);
    box.getAabb(aabb);
    CHECK(Near(aabb.m_max.x, 1.0f, 1e-5f));
    CHECK(Near(aabb.m_max.y, 2.0f, 1e-5f));
    CHECK(Near(aabb.m_max.z, 3.0f, 1e-5f));

    // convex vertex list — indices ignored (hull from points)
    const hkVector3 verts[4] = {
        hkVector3(-1, -1, -1), hkVector3(1, -1, -1),
        hkVector3(1, 1, -1), hkVector3(-1, 1, 1)};
    hkConvexVerticesShape hull(verts, 4, 0, 0);
    CHECK(hull.getType() == hkShape::TYPE_CONVEX_VERTICES);
    hull.getAabb(aabb);
    CHECK(aabb.m_max.x >= 1.0f - 1e-4f); // at least as wide as the points

    // triangle mesh — 1 triangle
    const hkVector3 mverts[3] = {
        hkVector3(0, 0, 0), hkVector3(4, 0, 0), hkVector3(0, 4, 0)};
    const hkUint32 mindices[3] = {0, 1, 2};
    hkMeshShape mesh(mverts, 3, mindices, 3);
    CHECK(mesh.getType() == hkShape::TYPE_MESH);
    mesh.getAabb(aabb);
    CHECK(Near(aabb.m_max.x, 4.0f, 1e-4f));

    // capsule — segment-based ctor (approximate height)
    hkCapsuleShape capsule(hkVector3(0, 0, 0), hkVector3(0, 2, 0), 0.5f);
    CHECK(capsule.getType() == hkShape::TYPE_CAPSULE);
    CHECK(capsule.getBulletShape() != 0);
}

// ---------------------------------------------------------------------------
// 3) Free fall — gravity -9.8 (sub_55E6D0[+0x04]) + stepDeltaTime
// ---------------------------------------------------------------------------
static void TestFreeFall() {
    hkWorld world;
    hkBoxShape box(hkVector3(0.5f, 0.5f, 0.5f));
    hkVector3 mass(1.0f, 1.0f, 1.0f);
    hkRigidBody body(mass, &box);
    body.setPosition(hkVector3(0.0f, 10.0f, 0.0f));
    CHECK(world.addEntity(&body).isSuccess());

    // 60 steps × 1/60 = 1 second — v ≈ -9.8
    const hkReal dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; ++i) {
        world.stepDeltaTime(dt);
    }

    hkVector3 pos, vel;
    body.getPosition(pos);
    body.getLinearVelocity(vel);
    // y should decrease (falling), velocity downward
    CHECK(pos.y < 10.0f);
    CHECK(vel.y < 0.0f);
    // free-fall speed ≈ g·t = -9.8 (no speed limit, no contact)
    CHECK(Near(vel.y, -9.8f, 0.35f));
}

// ---------------------------------------------------------------------------
// 4) Static body — mass 0 → does not fall
// ---------------------------------------------------------------------------
static void TestStaticBody() {
    hkWorld world;
    hkBoxShape box(hkVector3(1.0f, 1.0f, 1.0f));
    hkVector3 mass(0.0f, 0.0f, 0.0f); // static
    hkRigidBody body(mass, &box);
    body.setPosition(hkVector3(0.0f, 5.0f, 0.0f));
    CHECK(world.addEntity(&body).isSuccess());

    const hkReal dt = 1.0f / 60.0f;
    for (int i = 0; i < 30; ++i) {
        world.stepDeltaTime(dt);
    }
    hkVector3 pos;
    body.getPosition(pos);
    CHECK(Near(pos.y, 5.0f, 1e-4f)); // did not move
}

// ---------------------------------------------------------------------------
// 5) Contact — falling body rests on the ground (restitution 0.6/0.3 defaults)
// ---------------------------------------------------------------------------
static void TestGroundContact() {
    hkWorld world;

    // ground (static)
    hkBoxShape groundShape(hkVector3(50.0f, 1.0f, 50.0f));
    hkVector3 zeroMass(0.0f, 0.0f, 0.0f);
    hkRigidBody ground(zeroMass, &groundShape);
    ground.setPosition(hkVector3(0.0f, 0.0f, 0.0f));
    CHECK(world.addEntity(&ground).isSuccess());

    // falling body
    hkBoxShape box(hkVector3(0.5f, 0.5f, 0.5f));
    hkVector3 mass(1.0f, 1.0f, 1.0f);
    hkRigidBody body(mass, &box);
    body.setPosition(hkVector3(0.0f, 3.0f, 0.0f));
    CHECK(world.addEntity(&body).isSuccess());

    const hkReal dt = 1.0f / 60.0f;
    for (int i = 0; i < 180; ++i) { // 3 seconds
        world.stepDeltaTime(dt);
    }
    hkVector3 pos;
    body.getPosition(pos);
    // should rest on top of the ground + half body height ≈ 1.5
    CHECK(pos.y > 1.0f);
    CHECK(pos.y < 2.5f); // settled without bouncing (restitution 0.3 damped)
}

// ---------------------------------------------------------------------------
// 6) Phantom overlap — entity entering the trigger volume (hkpPhantom path)
// ---------------------------------------------------------------------------
static void TestPhantomOverlap() {
    hkWorld world;

    hkBoxShape triggerShape(hkVector3(2.0f, 2.0f, 2.0f));
    hkCachingShapePhantom phantom(&triggerShape);
    phantom.setPosition(hkVector3(0.0f, 0.0f, 0.0f));
    CHECK(world.addPhantom(&phantom).isSuccess());

    // static body positioned inside the phantom
    hkBoxShape box(hkVector3(0.5f, 0.5f, 0.5f));
    hkVector3 zeroMass(0.0f, 0.0f, 0.0f);
    hkRigidBody body(zeroMass, &box);
    body.setPosition(hkVector3(0.5f, 0.5f, 0.5f)); // inside the trigger
    CHECK(world.addEntity(&body).isSuccess());

    // one step for the ghost pair callback to fill in
    world.stepDeltaTime(1.0f / 60.0f);

    hkEntity *pHits[8];
    int numHits = 0;
    phantom.getPenetratingEntityList(pHits, numHits);
    CHECK(numHits >= 1);
    bool found = false;
    for (int i = 0; i < numHits; ++i) {
        if (pHits[i] == &body) found = true;
    }
    CHECK(found); // registration table btCollisionObject → hkEntity mapping

    // leave the phantom → clean
    CHECK(world.removePhantom(&phantom).isSuccess());
}

// ---------------------------------------------------------------------------
// 7) impulse — applyImpulse accelerates (Cannonball/Cannon path)
// ---------------------------------------------------------------------------
static void TestImpulse() {
    hkWorld world;
    hkSphereShape sphere(0.5f);
    hkVector3 mass(1.0f, 1.0f, 1.0f);
    hkRigidBody body(mass, &sphere);
    CHECK(world.addEntity(&body).isSuccess());

    body.applyImpulse(hkVector3(0.0f, 10.0f, 0.0f));
    hkVector3 vel;
    body.getLinearVelocity(vel);
    CHECK(vel.y > 0.0f); // gained upward velocity

    // mass access
    CHECK(Near(body.getMass(), 1.0f, 1e-6f));
}

int main() {
    TestNamingFidelity();
    TestShapes();
    TestFreeFall();
    TestStaticBody();
    TestGroundContact();
    TestPhantomOverlap();
    TestImpulse();

    std::printf("havokshim_test: %d checks, %d failures\n", g_checks,
                g_failures);
    return (g_failures == 0) ? 0 : 1;
}
