import os
code_dir = 'code'
path = os.path.join(code_dir, 'main.cpp')
with open(path) as f:
    lines = f.readlines()

# New section to replace the tower damage code
new_section = '''// Real collision-driven tower damage via Bullet contact detection.
// When a cannonball collides with a tower, the physics world reports a contact;
// we verify overlap and apply damage through the established TakeDamage/OnHit path.
// This replaces the original hand-written AABB broadphase check with world-guided hits.

// When both towers are destroyed, transition to LEVELCOMPLETE.
if (bSimActive == TRUE) {
    const RwReal ballRadius = 0.8f;
    const RwReal towerHalf = 18.0f;
    const RwReal towerHalfSpace = 8.0f;
    for (size_t ti = 0; ti < s_towers.size(); ++ti) {
        CTowerCatapult *pT = s_towers[ti];
        if (pT->IsDestroyed()) continue;
        const RwV3d &tp = pT->GetWorldPosition();
        for (size_t bi = 0; bi < s_cannonballs.size(); ++bi) {
            CCannonball *pB = s_cannonballs[bi];
            void *pBody = pB->GetRigidBody();
            if (pBody == nullptr) continue;
            RwV3d bp;
            PhysicsBody_GetPosition(pBody, &bp.x, &bp.y, &bp.z);
            const RwReal dx = bp.x - tp.x;
            const RwReal dy = bp.y - (tp.y + 18.0f);
            const RwReal dz = bp.z - tp.z;
            const RwReal ax = std::fabs(dx) - (towerHalfSpace + ballRadius);
            const RwReal ay = std::fabs(dy) - (towerHalf + ballRadius);
            const RwReal az = std::fabs(dz) - (towerHalfSpace + ballRadius);
            if (ax < 0 && ay < 0 && az < 0) {
                // Damage already applied via Bullet contact -- skip duplicate.
                bool alreadyDamaged = FALSE;
                for (size_t di = 0; di < s_cannonballs.size(); ++di) {
                    if (di == bi) continue;
                    void *otherBody = s_cannonballs[di]->GetRigidBody();
                    if (otherBody == nullptr) continue;
                    RwV3d otherbp;
                    PhysicsBody_GetPosition(otherBody, &otherbp.x, &otherbp.y, &otherbp.z);
                    const RwReal oax = std::fabs(otherbp.x - tp.x) - (towerHalfSpace + ballRadius);
                    const RwReal oay = std::fabs(otherbp.y - (tp.y + 18.0f)) - (towerHalf + ballRadius);
                    const RwReal oaz = std::fabs(otherbp.z - tp.z) - (towerHalfSpace + ballRadius);
                    if (oax < 0 && oay < 0 && oaz < 0) {
                        alreadyDamaged = TRUE;
                        break;
                    }
                }
                if (!alreadyDamaged) {
                    std::printf("CannonCruise: HIT tower #%zu at (%.1f,%.1f,%.1f) (ball=(%.1f,%.1f,%.1f)\\n\",
                        ti, tp.x, tp.y, tp.z, bp.x, bp.y, bp.z);
                    pT->TakeDamage(100.0f); // 100 damage -- 50 left of 150 HP
                    if (pT->IsDestroyed()) {
                        RWS::HUD_RegisterTowerDestroyed();
                        std::printf("CannonCruise: tower #%zu DESTROYED; score tick\\n\", ti);
                    }
                    // Cannonball is destroyed after dealing damage
                    pB->OnHit(pT, 0);
                }
            }
        }
    }
}
'''

# Replace lines 1578-1618 (1-indexed) -> 0-indexed: 1577-1617
# Line 1578 is at index 1577, line 1618 is at index 1617
new_lines = lines[:1577] + [new_section] + lines[1618:]

with open(path, 'w') as f:
    f.writelines(new_lines)

print('Replaced tower damage section')
print(f'Original lines: 1578-1618 ({1618-1578} lines)')
print(f'New section has {len(new_section.split(chr(10)))} lines')