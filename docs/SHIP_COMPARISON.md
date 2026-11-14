# Gemi Sistemi — Binary vs Port Karşılaştırması (2026-11-14)

Bu belge, CannonCruise.exe binary'sinin (`CC.asm`, `CC_ida.asm`) gemi alt
kodlarını bu projedeki yeniden yazılmış sınıflara karşı karşılaştırır.
İçerik `CC_ida.asm` üzerinden Elle doğrulandı; RWS içeriği yeni `shipscan_test`
aracıyla enumerate edildi.

## Tespit: RWS gemi varlıkları doğrulandı

`.RWS` dosyaları `RwStudioStream` ile açıldı; gemi asset'lerinin `.dff`
formatta olduğu doğrulandı. Çalışma ağacı temiz.

### Test envanteri (`shipscan_test.exe <file.RWS>`)

`tests/shipscan_test.cpp` + `shipscan_test` hedefi eklendi; bütün
seviye paketlerinde gemi asset'lerinin kaydolduğu görüldü.

### Tüm level'lerde bulunan gemi asset'leri

| Asset | Level'lerde | Tür | Boy |
|---|---|---|---|
| `Hugo_Ship.dff` | tümü | rwID_CLUMP | 172,940 B |
| `Hugo_CollHull_noinstance.dff` | tümü | rwID_CLUMP | 1,632 B |
| `Wheel_tag300.anm` | tümü | rwID_HANIMANIMATION | 2,608 B |
| `Rudder_tag400.anm` | tümü | rwID_HANIMANIMATION | 268 B |
| `Wing_Left_tag100.anm` / `Wing_Right_tag200.anm` | tümü | rwID_HANIMANIMATION | 1,348 B |
| `Gun_Barrel_tag600.anm` | tümü | rwID_HANIMANIMATION | 340 B |
| `Gun_Base_tag500.anm` | tümü | rwID_HANIMANIMATION | 556 B |
| `Viking_Destroyer.dff` | VIKING01–05 | rwID_CLUMP | 139,864 B |
| `Pirate_Destroyer.dff` | VIKING01,02,04,05 + BEAVER01–05 | rwID_CLUMP | 139,588 B |
| `Roman_Destroyer.dff` | BEAVER01–05 | rwID_CLUMP | 115,932 B |
| `Roman_Battleship.dff` | BEAVER01,04,05 | rwID_CLUMP | 266,460 B |
| `Viking_battleship.dff` | VIKING02,04,05 | rwID_CLUMP | 119,832 B |
| `Scylla_Destroyer.dff` | IDUNA01–03 + NORDIC02 + VIKING03 | rwID_CLUMP | 89,936 B |
| `Debris_Dark_02/03/04` + `Debris_Light_01/02` + `Debris_Mast_01` + `Debris_Paddle` + `Debris_Barrel` | tümü | rwID_CLUMP | ~1.5–3 KB |

### Hugo_Ship.dff iç yapısı

Librw `Clump::streamRead` ile başarılı okundu:
- 8 atomiks, 2,757 veritex, 3,533 tri, 1–2 malzeme (Hugo dokusu)
- 16-bayt özel preamble = `0x0002686 | 0x00000010 | 0x0002686-12 | 0x1803FFFF`
  ([u32 fileSize-6][u32 0x10][u32 fileSize-18][libid]) ardından standart
  RW 3.6 chunklarında: `STRUCT → FRAMELIST → GEOMETRYLIST → GEOMETRY'ler → ATOMIC'ler`

### Collision hull (`Hugo_CollHull_noinstance.dff`)

Havok "CompressedMeshShape" binary'dir; `Textures\0` işaretiyle başlar.
İçinde 3dsmax "User Properties" listesi asılı (Mass, Friction, Elasticity,
Unyielding = 0, Simulation_Geometry = 2). Havok 2.2 binary yapısı; çözülmesi
iki seviyeli analiz gerektirir.

## Port'ta eksikler

### 🔴 1. Gemi sınıfları runtime'da hiç yaratılmaz

`WorldSpawn_FromFile` sadece 4 behaviour kaydediyor:
`CWbRenderBehaviour`, `CWbCameraBehavior`, `CAudioStreamEntity`,
`CPhysicsWorldBehaviour`. Ship hiçbirinin kayıtlı kütüphanesinde yok.
RWS içerisinde hiçbir level .RWS'de `CShipPlayer`/`CShipEnemy`/`CShip`
behaviour kaydı yok — gemi spawn'u binary'nin level kodundan (örneğin
`sub_4069A0` veya benzeri) doğrudan kodlayıcı C++'tan yapılır.

**Gereken:** `GameBehaviours_Register()` içinde CShip, CShipPlayer,
CShipPlayerInputProxy, CShipEnemy kayıtları; `InitShipWorld/LevelCode` ile
olguolarak yaratılış; `CShip::Initialize(clump)` ile clump aktarmak.

### 🔴 2. Gerçek Hugo_Ship.dff hiç yüklenmez

`RwsPipeline_Load` yalnızca `WorldGeometry` ve `StaticCollisionData` asset'lerini işler.
Gemi asset'leri belli sözleşmeye göre listelenir:

```
[ASSETRECORD] rwID='rwID_CLUMP' src='X:\GFX\Ships\...\Hugo_Ship.dff' resource=172940
```

**Gereken:** `*.RWS` içindeki `rwID_CLUMP` asset'leri için bir filtre — `WorldGeometry` ile
avynı ayrışım yolu kullanılabilir:
1. `RwStudioAssetReadResource` ile payload oku
2. Gizli `RpReadClump(stream)`-style stream open (`rw::Clump::streamRead`)
3. `CShipPlayer::Initialize(clump)` ile geçir

### 🔴 3. `PlayerShip.cpp` placeholder gerçek değil

Ekran görüntüsünde `X` biçimi dümen kanatlarıyla bir toy-bücün; 13 vertices
13 trianç_test placeholder. Gerçek Hugo modeli 8 atomiks, 2,757 tris, 2 malzeme.

**Gereken:** placeholder kaldırılıp `RwsShip_LoadClump` ile gerçek modelin instantiate
edilmesi.

### 🟠 4. CShip::Initialize / CShipPlayer::UpdatePlayer sadece iskelet

Binary `FUN_004a7430` (CShip ctor), `FUN_004a76d0` (CShip::Update),
`FUN_004a7bc0` (CShip::TakeDamage) tam olarak koşturulmuş kod; port stub.
CShipPlayer'ın `sub_4AE700` (event wiring) ve `sub_4AF3C0` (per-frame)
implementasyonlarını içermez.

### 🟠 5. `CShipEnemy::UpdateAI` yok, `FUN_004aa2e0` tam atlanmış

2000+ byte binary code ile paralel: `sub_4AB200` (sub koruma + waypoint
arama), `sub_4AB520` (grup yok = deals cycle), `sub_49C940` (HUD push),
`sub_4ACB40` (can arama), `sub_4AC920` (durum değiştir). Port'ta bu fonksiyon
sayfalarca eksik.

### 🟠 6. `.anm` (HANIMANIMATION) parse edilmiyor

`Wheel_tag300.anm`, `Rudder_tag400.anm`, `Wing_Left/Right_*.anm`, `Gun_*.anm`
hiç okunmaz; `Rudder`/`PADDLE` tuzaklarını fizik günlerinde oynatan
kod yoktur.

### 🟡 7. Collision hull çözülmüyor

`Hugo_CollHull.dff` Havok binary; Havok 2.2'nin `bhkCompressedMeshShape`
formatında (~1.6 KB); bunları çözmenin textbook yolu Havok 2.2 binary
decompiler VR veya Bullet bot için kaya-kapsülü taklit etmek.

### 🟡 8. `CPhysicsWorld::Update` ve `btDiscreteDynamicsWorld` bağlı ama Bullet adım çağırıcısı CShip'in loop'una bağlı değil

## Binary fonksiyon adresleri ile ilgilison (CShip davranışı)

| Binary Adı | İşlev |
|---|---|
| `sub_4A5F80` | CMsg apply (27-way switch; physics health) |
| `sub_4A99E0` | CShipEnemy HE iMsgRunningTick |
| `sub_4AB200` | Enemy AI ana güncelleme |
| `sub_4AB520` | Grup senkronizasyonu (burst/release) |
| `sub_4AC920` | Ai durum değiştirme (PATROL → HUNT) |
| `sub_4ACB40` | Dost gemisi bul |
| `sub_4AC740` | Menü/seçim bekleyen AI |
| `sub_4A99E0` | Enemy silah menüsü |
| `sub_4AE6C0` | CShipPlayer class registration (RWS `CClassFactory register`) |
| `sub_4AE700` | CShipPlayer ctor + event wiring |
| `sub_4AF3C0` | CShipPlayer per-frame |
| `sub_4B2FE0` | CShipPlayerInputProxy::Update (W/S/A/D) |

## Sonuç

Port hata-hata `CShipPlayer`/`CShipEnemy` şablonlarını yazarken binary
aynen bunu kendi framework'üne bağlıyor (ör. `EntityMessageProcessor`
hedefi game-loop'ta Tick çağırdıran merkezi bir sistem). Şu anki seviye:

- **Placeholder verme hızı**: Oyun penceresi ada + nesneler render ediyor,
  bir oyuncu gemisi dünya erişiminde değil — oyun içinde "gemi" olarak
  hiçbir şey görünmez.
- **Oynananlık**: Paddle butonu maçı hull-temelli değil, fren-rt-ler eksen-
  eksen bağımsız.
- **Debug kanıt eksikliği**: CShipPlayer/CShipEnemy'nin sadece sınıf
  kayıtları yazıldı; davranışın hiçbiri yok.
