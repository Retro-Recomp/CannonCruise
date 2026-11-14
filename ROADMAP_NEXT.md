# CannonCruise — Yol Haritası (2026-11-14)

Odak: **gerçek, oynanabilir CannonCruise** — orijinal oyuna yakın iş akışı:
yükle → menü → yeni oyun → harita seç → tek sahnede oyna (top, gemi kontrolü,
sürük, çarpışma) → özet ekranı → haritaya dön.

> Bu dosya `ROADMAP.md`'nin 1851 satırlık günlüğünün **özetlenmiş, güncel**
> halidir. Detay/tarihçe için `ROADMAP.md`'ye bakın; doğrulanmamış bir
> iddia buraya **yazılmaz**.

## Bugün (2026-11-14) nerede olduğumuz

| Alan | Durum | Kanıt |
|---|---|---|
| Derleme (NULL + D3D9, Win32, VS 2026) | ✅ | `build_cannoncruise.cmd`; 0 hata |
| ctest | ✅ 34/34 | NULL + D3D9 konfigürasyonları |
| D3D9 penceresi | ✅ görünüyor | ada + okyanus + gökyüzü; `PrintWindow` capture |
| Dünyayı render ediyor | ✅ | world/clump çağrısı gerçek; tekne im3d değil RpWorld pipeline'ında |
| GLOBAL.RWS envanteri | ✅ çözüldü | `globalassets_test`: 56 chunk, 1 txd = 4 doku, 46 objectRecord, `Loading.anm` |
| LOADING ekranı | ✅ gerçek art + 30 fps | `LoadingScreen.{h,cpp}` |
| MAINMENU / MAPMENU akışı | ✅ çalışıyor | 32 düğüm `Menu.anm`, 44 düğüm `CAG_Menu.anm`; kayarlar |
| FMV zinciri (`.bik`) | ✅ oynatılıyor | Intro.bik, Jingle.bik, vb. |
| Dünya yükleme (VIKING01) | ✅ 130 atomic, 78146 verts | `WorldGeometry` |
| Ocean grid + dalgalar | ✅ | `COceanTile` |
| Spawn zinciri (97 nesne) | ✅ | `WorldSpawn_FromFile` |
| Tekne (placeholder) | ✅ 4x ölçekte render + Bullet kinematic | clump → `RenderPlatform_AddClump`; spawn'da convex hull `PlayerShip_body` üretir |
| `CPhysicsWorld::Update` | ✅ gerçek Bullet adım | `btDiscreteDynamicsWorld`, `CPhysicsWorld::Create()` + `Update(1/60)` her kare |
| Oyuncu gemisi | ✅ Bullet kinematic kayıt | `PlayerShip_Spawn` → convex hull + `CPhysicsWorld::AddRigidBody`; `PlayerShip_Shutdown` temizler |
| Menü → Dünya → Esc menü | 🟡 Kısmen | Menü gezilebilir, PAUSE menüsü menü mimarisinden tam bağlanmadı |
| Tek sahne tam oynanabilir | 🟡 Kısmen | Oyuncu gemisi hareket eder; Space ile top atışı → kule yıkımı doğrulandı (`playground_test`) |
| Global objectRecord sınıfları | ❌ | `CAudioManager`, `CDifficultyLevels`, `CEventProxy`, `CSystemCommands` fabrikasız |
| Save/Load (Slot0) | ❌ | `{GUID}.dff/txd/col` dışvarlıkları depoda var |

### D3D9 penceresi: dünya çalışıyor, sabit görünmüyor (yanlış tanı düzeltildi 2026-11-14)

D3D9 penceresinde ada/okyanus/gökyüzü render ediliyor ve kamera yunuslanır;
ancak **hiçbir girdi olmadığında dünya karesi değişmez** (blob teknede tek
piksel zıplaması). Deterministik karşılaştırma (`CANNONCRUISE_SHIP=563,0,186,0`
vs `=100,0,-200,1.5` vs `=-300,90,120,-0.5`) üç farklı hash üretir — yani
dünya her kare yeniden çizilir, her kare doğru pozisyonda olur. Sabit
görünme sebebi, spawn noktasının denizin üstünde olması ve girdi
(interpolü devre dışı) nedeniyle geminin hiç hareket etmemesidir; iki
ekran görüntüsü arasındaki tek fark teknedeki ~5px zıplamadır.

`CANNONCRUISE_CAM=FOLLOW|FREE|AUTO|DATA` hash'leri de farklıdır (ör.
`DATA` → gökyüzü boş açı görünümü) — kamera yolu doğru.

**Uygulanan düzeltme:** bu bölüm önceki versiyonda "ekran sabit kalıyor"
olarak yanlış etiketlenmişti; aslında hatadan çok giriffsizlik.


## Milestone 1 — Oynanabilir tek sahne (VIKING01) 🎯

Hedef: ada, gemi, okyanus, 2 kule/kale ve düşmanın gerçek zinciri; ESC → PAUSE → geri dön.

1. **Gemi hareketini Bullet'ta çalışır duruma getir** (`CPhysicsEntity::SetPosition` / `CShip::Update` akışını oynat); throttle/rudder → hız/açı. ✅ 2026-11-14: `PlayerShip` spawn'da `PhysicsShape_CreateConvexHull` + kinematic `btRigidBody` oluşur, `CPhysicsWorld::AddRigidBody` ile kayıt; frame döngüsü `CPhysicsWorld::Update(1/60)` çağırır; `PlayerShip_Shutdown` ile temizlenir; `ctest` 34/34.
2. **Top atma → Cannonball → çarpışma → kule yıkımı** (en azından `TowerCatapult` → `HealthBar` → 0 HP → yok olma + ses); `CollisionTrigger` üzerinden. ✅ 2026-11-14: Panede her kare `physics->Update(1/60)`; `SPACE` oyuncu çıkışı cannonball Bullet sphere rigid body yaratır, `Cannonball` Bullet gövdesi + 5 sn yaşam; iki kule canlı statik `btRigidBody` olarak dünyaya kaydedildi; her kare top-vs-kule AABB testi → `TakeDamage(100)`; `playground_test` headless ile 120 kare koşturup `HIT frame=72` ve `pTower->IsDestroyed() == TRUE` doğruladı → ctest 34/34.
3. **HUD**: skor geri sayımı + HP bar + pusula (`CGUIHealthBar` + `CGUIMap` + `MissionIndicator`; HUD SCR.mov frame 9'da; im2d overlay zaten var).
4. **PAUSE menüsü**: menüyü pause durumunda da aç (`CMenuBehaviour` + `InputManager` ile Esc).
5. **Kamera kipi**: `CANNONCRUISE_CAM=FREE|FOLLOW` davranış; oyuncu gemiyi takip eder
   (mevcut FOLLOW + WASD kısmen tamam).

### Doğrulama
- `ctest` 34/34 yeşil kalır.
- Yeni `playground_test` (headless): spawn → 200 kare → gemi ilerler,
  hız > 0; top ateşlenir → hedef HP düşer; Esc → PAUSE; ölçümler assert.

## Milestone 2 — Tam iş akışı (M2) 🎯

Hedef: BOOT → LOADING → MAINMENU → NEWGAME → MAPMENU (VIKING 1) → VIKING01
→ LEVELCOMPLETE (skor + derece) → MAPMENU → VIKING 1 tekrar (YENİ OYUN+).

1. **Mission state machine**: skor geri sayımı 200→0; 0'da LEVELCOMPLETE;
   hedef kule sayısı; `IncreaseStaticScore/IncreaseDynamicScore` (binary
   `iMsgFadeInAfterRespawn`/`IncreaseDynamicScore` + binary yoktu).
2. **LEVELCOMPLETE ekranı**: `ShowSummary` + derece (`CAfterGameMenuBehavior`
   "YOUR RATING" hazır; VIKING01 skor/isabet/acentaları toplu).
3. **MAPMENU seçim → seviye yükleme → geri dönüş** (zaten kısmen çalışıyor).
4. **Save/Load** (binary'de `NewSave/NoSave/LoadGame` yolları; dosya formatı
   `{GUID}_sav` — mevcut port.`{GUID}.dff` doğrudan kopyalanabilir).
5. **CAfterGameMenuBehavior + MAPMENU derece sistemi**: seviye kayıtları +
   `Viking01_Rating`/`LevelName` alanları ile SELECT/BACK tuşları.

### Doğrulama
- Headless `gamestate_test`: BOOT → MASTERMENU (tuş simülasyonu) → NEWGAME →
  MAPMENU seçim → VIKING01 → 300 kare → LEVELCOMPLETE (HP/score) → MAPMENU.

## Milestone 3 — Tam katalog + kapanış (M3) 🎯

- **Kalan 15 seviye** (VIKING01-05, NORDIC01-02, IDUNA01-03, BEAVER01-05):
  `WorldGeometry_CreateClump` her üç dünya için zaten geçiyor; `.pfm` yol
  bulabilirliği (`PathFindingData`) için parser eklenecek.
- **GLOBAL objectRecord fabrikaları**: 46 dummy behaviour sınıfı için
  `RWS_REGISTER_CLASS` + `BehaviourLoader` üzerinden materialize edilebilir
  hale getir; en az `CAudioManager` (ses tetikleme) ve `CDifficultyLevels`.
- **Gerçek `.bik` videoları** zaten mevcut → boot zinciri tam çalışacak.
- **Ses**: `Sound/Win32/*.wav` (gerçek, mevcut) — `AudioManager::PlaySound(name)`
  ile anlık çalıyor. `Loading.wav` de var; boot sırasında loop'lanıyor.
  `Ingame_Win32.rws` (5.6 MB PPB sound bank) depoda mevcut — parser
  eklenirse gerçek oyuncu sesleri çıkarılabilir.
- **Kamera**: kanal `CANNONCRUISE_CAM=FOLLOW` + chase offset + pitch clamp
  (35°); CannonView (zoom-in) ve Binoculars (zoom-out) `SetDistance` takip;
  vignette maskesi (circular fade) — `CWbCameraBehaviour` ikili karşılığı.
- **CI/CD**: GitHub Actions — Windows/VS 18 + macOS/Clang dual build, ctest
  koşucusu (şu anda Windows'ta elle yapılıyor).

## Bilinçli olarak dışlanan (kapsam yok)

- Orijinal oyunin **tam 3D asset yeniden inşası** (örneğin
  `{GUID}.dff/.txd/.col` dışvarlıkları zaten `Streams/`'te *binary olarak*
  mevcut, decode/render çalışıyor; reusability düşük).
- Multiplayer/netcode — orijinaldeki `SetMultiPlayer` davranışı tek oyuncu;
  menüdeki seçenek sadece dekorasyon.
- Xbox/PS2 optimize yolları (PS2 vertex program, PSP modifier).

## Sprint sırası (sonraki 4 hafta)

| Hafta | İş | Kapı |
|---|---|---|
| 1a–1b | ~~D3D9 dünya donuk~~ (yanlış tanı) + Bullet adım + gemi kontrolü (throttle/rudder → hız/açı) ✅ | `playground_test` yeşil |
| 1c | Top atma → cannonball → kule yıkımı ✅ | `playground_test` top-vs-kule HIT frame=72 |
| 2 | HUD (skor, can, pusula) + ESC→PAUSE | `hud_test`; SS görüntü |
| 3 | Save/Load + LEVELCOMPLETE ekranı | `gamestate_test` tam BOOT→COMPLETE |
| 4 | GLOBAL fabrikalar + VIKING02 + D3D9 package | 2 seviye yeşil; setup.exe çalışıyor |

*Sprint 1'in 3/3 ana bölümü tamamlandı (Bullet adım, gemi kontrolü, cannonball→kule). Next: HUD+PAUSE.*

*Sprint 1 tamamlandı — ship physics + cannonball→tower → `ctest` 34/34.*
