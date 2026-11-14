// ============================================================================
// CannonCruise - Placeholder Player Boat (PlayerShip.h)
// Faz 4.6s (tekne follow) -- 2026-10-17.
//
// Gerçek oyuncu gemisi bilgisi: CShipPlayer (ShipPlayer.cpp, binary
// sub_4AE6C0; 0x488 bayt) fabrikaya kayıtlı ama 3B modeli ({GUID}.dff)
// elimizde yok -- dünya mesh'i gibi harici build-output varlığı eksik.
// Bu modül, oyun döngüsü doğrulanabilsin diye DÜŞÜK POLİGONLU bir
// placeholder tekne (gövde + direk) ve basit bir kinematik hareket
// modeli sağlar. Fizik gövdesi (Bullet) sonraki fazda bağlanır
// (CPhysicsEntity::SetPosition/GetPosition hazır).
//
// Hareket modeli (deneysel, binary-esinli değil):
//   - throttle: W=+1 (ileri), S=-0.5 (geri)
//   - speed -> throttle * kMaxSpeed (yumuşak geçiş, exp)
//   - rudder: A=+1 (iskele), D=-1 (sancak); yaw hızı hızla orantılı
//   - konum: y düzleminde ileri vektör + dalga bob'u
// ============================================================================
#pragma once

#include "rwcore.h"

struct PlayerShipState {
    RwV3d pos;      ///< Dünya konumu (y = su yüzeyi + bob)
    RwReal yaw;     ///< Radyan; ileri vektör = (sin(yaw), 0, cos(yaw))
    RwReal speed;   ///< Anlık ileri hız (birim/sn)
    RwReal throttle;///< -1..+1
    RwReal rudder;  ///< -1..+1 (pozitif = iskele/sancak dönüşü)
};

/// Tekneyi sahneye koyar. pPos null ise (0,0,0) kullanılır.
void PlayerShip_Spawn(const RwV3d *pPos, RwReal yaw);

/// Spawnten yaratılan kinematic rigid body'yi PhysicsWorld'den çıkarıp
/// bellekten temizler (uygulama kapanırken Bullet'ın dtor'daki boş-vector
/// assertion'ını önlemek için).
void PlayerShip_Shutdown(void);

/// throttle/rudder -> kinematik adım. waveT dalga fazı (sn).
void PlayerShip_Update(RwReal deltaTime, RwReal waveT,
                       RwReal throttle, RwReal rudder);

/// Aktif sahne durumu (Spawn çağrılmadıysa FALSE).
RwBool PlayerShip_GetState(PlayerShipState *pOut);

/// Placeholder gövde çizimi -- rw::im3d TRILIST (GL3 derlemesinde gerçek
/// GPU çizimi; NULL derlemede indeks üretimi doğrulanır, çizim atlanır).
void PlayerShip_Render();
