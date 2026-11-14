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
| ctest | ✅ 32/32 | NULL + D3D9 konfigürasyonları |
| D3D9 penceresi | ✅ görünüyor | ada + okyanus + gökyüzü; `PrintWindow` capture |
| Dünyayı render ediyor | ✅ | world/clump çağrısı gerçek; tekne im3d değil RpWorld pipeline'ında |
| GLOBAL.RWS envanteri | ✅ çözüldü | `globalassets_test`: 56 chunk, 1 txd = 4 doku, 46 objectRecord, `Loading.anm` |
| LOADING ekranı | ✅ gerçek art + 30 fps | `LoadingScreen.{h,cpp}` |
| MAINMENU / MAPMENU akışı | ✅ çalışıyor | 32 düğüm `Menu.anm`, 44 düğüm `CAG_Menu.anm`; kayarlar |
| FMV zinciri (`.bik`) | ✅ oynatılıyor | Intro.bik, Jingle.bik, vb. |
| Dünya yükleme (VIKING01) | ✅ 130 atomic, 78146 verts | `WorldGeometry` |
| Ocean grid + dalgalar | ✅ | `COceanTile` |
| Spawn zinciri (97 nesne) | ✅ | `WorldSpawn_FromFile` |
| Tekne (placeholder) | ⚠️ 4x ölçekte render | clump → `RenderPlatform_AddClump` (sprite-uzakta görünür; oyun kamera DÜŞÜŞÜ büyük) |
| `CPhysicsWorld::Update` | ✅ gerçek Bullet adım | `btDiscreteDynamicsWorld` |
| Menü → Dünya → Esc menü | 🟡 Kısmen | Menü gezilebilir, PAUSE menüsü menü mimarisinden tam bağlanmadı |
| Tek sahne tam oynanabilir | ❌ | TOP atan gemi + çarpışma + HUD (skor/can) |
| Global objectRecord sınıfları | ❌ | `CAudioManager`, `CDifficultyLevels`, `CEventProxy`, `CSystemCommands` fabrikasız |
| Save/Load (Slot0) | ❌ | `{GUID}.dff/txd/col` dışvarlıkları depoda var |

### Bilinen açık: D3D9 penceresinde dünya sabit/resize yok

D3D9 `CAMERATEXTURE` RT → backbuffer `StretchRect` yolu çalışıyor ve pencerede
ada/okyanus/gökyüzü görünüyor; ancak **görüntü Kare-1'den itibaren aynıdır**
(kamera hareketi pencereye yansımaz; tekne tek piksel zıplamasıyla ilerler).
Sebep: `cameraPath`/`ShowRaster`/swap dizisi içinde her karede backbuffer
re-bind edilmiyor ya da RT her karede aynı lazerle present ediliyor. Okyanus
bildirimleri çerçeve öncesi `RwPlatformPollEvents` her karede çağrılıyor
ancak `world->render()` sonrası pencere kaplaması yeni kareyi göstermiyor
(muhtemelen `SWAPEFFECT_DISCARD` + `Present` sırasındaki backbuffer
tex-uyumsuzluğu). **Not 2026-11-14**: `RenderPlatform_SetCameraPose`
çağrıları her karede yapılıyor, `Camera::beginUpdate` da `devView`'i
taze `getLTM()`'den güncellerdiği doğrulandı; sorun PİKÜL doğrulaması
derinine bırakıldı (muhteme parser: RT değil backbuffer stale present).

## Milestone 1 — Oynanabilir tek sahne (VIKING01) 🎯

Hedef: ada, gemi, okyanus, 2 kule/kale ve düşmanın gerçek zinciri; ESC → PAUSE → geri dön.

1. **Gemi hareketini Bullet'ta çalışır duruma getir** (`CPhysicsEntity::SetPosition` / `CShip::Update` akışını oynat); throttle/rudder → hız/açı.
2. **Top atma → Cannonball → çarpışma → kule yıkımı** (en azından `TowerCatapult` → `HealthBar` → 0 HP → yok olma + ses); `CollisionTrigger` üzerinden.
3. **HUD**: skor geri sayımı + HP bar + pusula (`CGUIHealthBar` + `CGUIMap` + `MissionIndicator`; HUD SCR.mov frame 9'da; im2d overlay zaten var).
4. **PAUSE menüsü**: menüyü pause durumunda da aç (`CMenuBehaviour` + `InputManager` ile Esc).
5. **Kamera kipi**: `CANNONCRUISE_CAM=FREE|FOLLOW` davranış; oyuncu gemiyi takip eder
   (mevcut FOLLOW + WASD kısmen tamam).

### Doğrulama
- `ctest` 32/32 yeşil kalır.
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
| 1 | Bullet adım + gemi kontrolü (throttle/rudder → hız/açı) | `playground_test` yeşil |
| 2 | HUD (skor, can, pusula) + ESC→PAUSE | `hud_test`; SS görüntü |
| 3 | Save/Load + LEVELCOMPLETE ekranı | `gamestate_test` tam BOOT→COMPLETE |
| 4 | GLOBAL fabrikalar + VIKING02 + D3D9 package | 2 seviye yeşil; setup.exe çalışıyor |

*Bir sonraki güncelleme: Sprint 1 sonu — ship physics → `ctest` 34/34.*
