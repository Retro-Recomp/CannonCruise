# CClassFactory + Behaviour Spawnsistor — Binary vs Port Auditi (2026-11-14)

Bu belge, CannonCruise.exe binary'sinin çalışma-zamanı sınıf fabrika
kayıtlarını (`call sub_413700`, i.e. `CClassFactory::Register`),
portimizin `RWS_REGISTER_CLASS` ve `BehaviourLoader::Register`
çağrılarının yarattığı kayıt tablosuyla karşılaştırır. Gözlem dualdır:
(a) **.RWS tarafından adı geçen 22 sınıf** .BINARY VERİLERİNİ (`shipscan_test`
çıktısı) karşımıza çıkarır, (b) portimizin hangi kayıtları bildiğini
loglar ve raporlar. .RWS ile port tablosu **asla örtüşmez**.

## 1. Binary çalışma-zamanı sınıf kayıt tablosu

`call sub_413700` preceded by
```asm
push 1                 ; enable flag
push <sizeof class>     ; uygulama class size
push offset sub_<ctor>  ; sınıf kurucusu
push offset a<Name>     ; .rdata sınıf adı
mov ecx, offset unk_658474  ; CClassFactory dict → singleton
call sub_413700         ; CClassFactory::Register
```
`sub_4C2CE0` (CWbCameraBehavior) tipik numune; `sub_4AE6C0` CShipPlayer.

Örnek .RWS kayıt logları üstünden doğrulandı; 50 sınıf:

```
AudioGlobalMixer  CActor  CAfterGameMenuBehavior  CAmmoCrate  CAnimatedEnvironment
CAudioManager  CAudioObject  CAudioStream  CAudioStreamEntity  CBirdSeaGull
CCameraShaker  CCannonball  CCollisionTrigger  CDebris  CDifficultyLevels
CEventProxy  CEventTrigger  CFade  CFernandoActor  CFishGreen  CFMVBehavior
CGUIHealthBar  CGUIMap  CHealthDebris  CHugoActor  CLifebuoy  CLoadLevel
CLogic_OR  CMenuBehavior  CMine  CMissionIndicator  COcean  CParticleEntity
CParticleFountain  CParticleManager  CPerformanceVisualizer  CPhysicsWorldBehaviour
CProfilerBehaviour  CProgressIndicator  CShipEnemy  CShipPlayer  CShowText
CSoldierAi  CSpeakEntity  CThunder  CTowerCatapult  CWaypoint  CWbCameraBehavior
CWbRenderBehavior  CWin32Keyboard
```

## 2. Portin `RWS_REGISTER_CLASS` kayıtları (üretim kod dışı)

`Production` kodu içindeki `RWS_REGISTER_CLASS` çağrıları CClassFactory'
`(“Register”)`'e servis vermez üretimde hiçbir yerde
`CClassFactory::Create(...)` çağrılmaz — yani `RWS_REGISTER_CLASS`
sadece **test ortamında** kullanılır.

Tüm port kayıtları (üretim içindeki):

| # | Port sınıfı | Binary | Notlar |
|---|---|---|---|
| 1 | `CActor` | OK | |
| 2 | `CAfterGameMenuBehavior` | OK | |
| 3 | `CAmmoCrate` | OK | |
| 4 | `CBird` | — | portta eklenti; binary'da yok |
| 5 | `CBirdSeaGull` | OK | |
| 6 | `CCannonball` | OK | |
| 7 | `CCollisionTrigger` | OK | |
| 8 | `CDebris` | OK | |
| 9 | `CEventProxy` | OK | |
| 10 | `CFade` | OK | |
| 11 | `CFernandoActor` | OK | |
| 12 | `CFish` | — | port eklentisi |
| 13 | `CFishGreen` | OK | |
| 14 | `CFlotsam` | — | port eklentisi |
| 15 | `CGUIHealthBar` | OK | |
| 16 | `CGUIMap` | OK | |
| 17 | `CHealthDebris` | OK | |
| 18 | `CLifebuoy` | OK | |
| 19 | `CMenuBehavior` | OK | |
| 20 | `CMissionIndicator` | OK | |
| 21 | `COcean` | OK | |
| 22 | `CPerformanceVisualizer` | OK | |
| 23 | `CPhysicsWorldBehaviour` | OK | |
| 24 | `CProfilerBehaviour` | OK | |
| 25 | `CShipEnemy` | OK | |
| 26 | `CShipPlayer` | OK | |
| 27 | `CShowText` | OK | |
| 28 | `CSoldier` | **isim uyumsuzluğu** (bin: `CSoldierAi`) |
| 29 | `CSpawnable` | — | port eklentisi |
| 30 | `CSpeakEntity` | OK | |
| 31 | `CTowerCatapult` | OK | |
| 32 | `CTrigger` | — | port eklentisi |
| 33 | `CWbCamera` | **isim uyumsuzluğu** (bin: `CWbCameraBehavior`) |
| 34 | `CWbRender` | **isim uyumsuzluğu** (bin: `CWbRenderBehavior`) |
| 35 | `CAudioStreamEntity` | OK | |

23 tam uyum, 3 isim uyuşmazlığı, 5 port eklentisi, 20 binary sınıf portta
hiç kayıtlı değil.

## 3. .RWS'ta görünen davranış sınıfları (`shipscan_test` ile doğrulandı)

Tüm .RWS (GLOBAL, MAINMENU, MAPMENU, VIKING01–05, BEAVER01–05, IDUNA01–03,
NORDIC01–02) içinde geçen davranış sınıf adlarının tam listesi:

```
AudioGlobalMixer      CAttributeHandler      CAudioManager
CAudioObject          CAudioStream           CAudioStreamEntity
CBirdSeaGull          CCameraShaker          CCollisionTrigger
CDifficultyLevels     CDirectorsCamera       CEventTrigger
CFernandoActor        CFMVBehavior           CHealthDebris
CMenuBehavior         CMissionIndicator      CParticleEntity
CParticleFountain     CParticleManager       CProgressIndicator
CSystemCommands       CTowerCatapult         CWbCameraBehavior
CWbRenderBehavior     CWin32Keyboard
```

Toplam **26** benzersiz sınıf. Hiçbiri `"CShipPlayer"` veya `"CShipEnemy"`
içermez — gemi davranışları kaynağa (behavior) değil Level koduna tabi.

### Hangileri portta "RWS_REGISTER_CLASS" yolu ile kayıtlı?

| .RWS sınıfı | Portta RWS_REGISTER_CLASS? |
|---|---|
| `AudioGlobalMixer` | yok |
| `CAttributeHandler` | yok (attribute handler doğrudan C++ kaynak) |
| `CAudioManager` | yok |
| `CAudioObject` | yok |
| `CAudioStream` | yok |
| `CAudioStreamEntity` | var |
| `CBirdSeaGull` | var |
| `CCameraShaker` | yok |
| `CCollisionTrigger` | var |
| `CDifficultyLevels` | yok |
| `CDirectorsCamera` | yok |
| `CEventTrigger` | yok |
| `CFernandoActor` | var |
| `CFMVBehavior` | yok |
| `CHealthDebris` | var |
| `CMenuBehavior` | var |
| `CMissionIndicator` | var |
| `CParticleEntity` | yok |
| `CParticleFountain` | yok |
| `CParticleManager` | yok |
| `CProgressIndicator` | yok |
| `CSystemCommands` | yok |
| `CTowerCatapult` | var |
| `CWbCameraBehavior` | var (portta `CWbCamera` adıyla) |
| `CWbRenderBehavior` | var (portta `CWbRender` adıyla) |
| `CWin32Keyboard` | yok |

13/26 portta mevcut; 13/26 portta tamamen yok. `skipped` payının büyük
olmasının nedeni: sorgulanır liste (`BehaviourLoader::Register`'deki
entries) yalnız 4 sınıfa sahiptir, `Find(name)` arama her isim için böyle
NULL döner ve `.RWS` behaviour'u elenir.

## 4. `GameBehaviours_Register()` (runtime spawn yolu)

`WorldSpawn_FromFile()` → `.RWS` 0x0704 records → `BehaviourLoader::Register`
çağrı listesi. Port şu 4'ünü kaydeder:

| Port adı | Binary adı | sizeof (port) |
|---|---|---|
| `"CWbRenderBehavior"` | `"CWbRenderBehavior"` | `sizeof(CRenderBehaviourSub)` |
| `"CWbCameraBehavior"` | `"CWbCameraBehavior"` | `sizeof(CCameraState)` |
| `"CAudioStreamEntity"` | `"CAudioStreamEntity"` | `sizeof(CAudioStreamEntity)` |
| `"CPhysicsWorldBehaviour"` | `"CPhysicsWorldBehaviour"` | `sizeof(CPhysicsWorldBehaviour)` |

Yalnızca bu 4'ü .RWS’i birer başarılı `instancesCreated` yapabilir; diğer 46+
davranış (4'ü kapsam dışı) portta bulunmadığı için **instancesSkipped** olarak
atılır. Testlerde `WorldSpawn: records=497 created=97 skipped=152 applied=97`
okomparışır.

## 5. Tespit edilen uyumsuzluklar (öncelik P1–P3)

### P1 — .RWS'da olan ama port tablosunda olmayan 13 sınıf

`WorldSpawn_FromFile()`'ın `BehaviourLoader::Register` listesindeki isimler,
.RWS'deki davranış kaydını okuduktan sonra **adıyla arayıp yaratmalıdır**
(be kayda alınmazsa `Find()` NULL döner ve skip sayar):

| .RWS adı | Portta? | `GameBehaviours_Register()`'te? | .RWS kayıt yeri |
|---|---|---|---|
| `CAttributeHandler` | yok (Attribute handler) | yok | her kayıt yanında |
| `CDirectorsCamera` | yok | yok | VIKING/IDUNA/NORDIC/BEAVER |
| `CEventTrigger` | yok | yok | VIKING, BEAVER |
| `CParticleEntity` | yok | yok | VIKING/IDUNA/NORDIC |
| `CParticleManager` | yok | yok | VIKING, MAINMENU, MAPMENU |
| `CSystemCommands` | yok | yok | her kayıt yanında |
| `CWin32Keyboard` | yok | yok | MAINMENU/MAPMENU |
| `CMenuBehavior` | portanmiş | yok | tüm .RWS |
| `CFernandoActor` | portanmiş | yok | VIKING, IDUNA, NORDIC |
| `CHealthDebris` | portanmiş | yok | VIKING, IDUNA, NORDIC, BEAVER |
| `CTowerCatapult` | portanmiş | yok | VIKING, BEAVER, IDUNA, NORDIC |
| `CCollisionTrigger` | portanmiş | yok | VIKING, BEAVER, IDUNA, NORDIC |
| `CBirdSeaGull` | portanmiş | yok | VIKING, BEAVER |

13'ten 9'u port'ta "RWS_REGISTER_CLASS" ile kaydedilir ama üretim
path'inde **yok** (`GameBehaviours_Register`).
9'u da port formunda entry tablosuna yazılamaz ise **spawned kare** yalnız
skip sayar.

### P2 — isim uyuşmazlıkları port <-> binary / .RWS

| Port (RWS_REGISTER_CLASS) | Binary | .RWS |
|---|---|---|
| `"CSoldier"` | `"CSoldierAi"` | — |
| `"CWbCamera"` | `"CWbCameraBehavior"` | `"CWbCameraBehavior"` |
| `"CWbRender"` | `"CWbRenderBehavior"` | `"CWbRenderBehavior"` |

Bu üçü production yolunda doğrudan null sonuç döner; eğer runtime `.RWS`
kayıtları `CWbCameraBehavior` adıyla aranırsa ve RWS_REGISTER_CLASS
`CWbCamera` adıyla kaydedilirse, hiçbir zaman `CClassFactory::Create`
ilan çağırmayacak üretim kodudur (yok).

### P3 — HEURISTIK: port tablomuzu binary şemasına göre yenilemeliyiz

`GameBehaviours_Register()` 5–10 giriş yerine 50'ye yakın düzene geçmeli.
Hem .RWS'de görülenler hem de bügüne kadar'da (örneğin HUD bileşenleri)
üretim kayıtları da temsil etmelidir.

Left to plan:
```text
{ "CWbRenderBehavior", sizeof(CRenderBehaviourSub), &CreateWbRender, &ApplyWbRender },
{ "CWbCameraBehavior", sizeof(CCameraState), &CreateWbCamera, &ApplyWbCamera },
{ "CAudioStreamEntity", sizeof(CAudioStreamEntity), &CreateAudio, &ApplyAudio },
{ "CPhysicsWorldBehaviour", sizeof(CPhysicsWorldBehaviour), &CreatePhysicsWorld, &ApplyPhysicsWorld },
/* + öner: */
{ "CMenuBehavior", sizeof(CMenuBehavior), &CreateMenuBehavior, &ApplyMenuBehavior },
{ "CFernandoActor", sizeof(CFernandoActor), &CreateFernandoActor, &ApplyFernandoActor },
{ "CHealthDebris", sizeof(CHealthDebris), &CreateHealthDebris, &ApplyHealthDebris },
{ "CTowerCatapult", sizeof(CTowerCatapult), &CreateTowerCatapult, &ApplyTowerCatapult },
{ "CCollisionTrigger", sizeof(CCollisionTrigger), &CreateCollisionTrigger, &ApplyCollisionTrigger },
{ "CBirdSeaGull", sizeof(CBirdSeaGull), &CreateBirdSeaGull, &ApplyBirdSeaGull },
{ "CParticleManager", sizeof(CParticleManager), &CreateParticleManager, &ApplyParticleManager },
…
```

Left to verify:

- allocation scaled by `class .size` and `.RWS's block.size` (matches a
  convention test exists in `behaviourloader_test.cpp` only among 4).
- `CAfterGameMenuBehavior`, `CBeamEmitter` etc may be needed at runtime as
  well — but those aren't in .RWS files either; those are spawned via
  code-path (e.g. Bullet-body objects) instead.
- `.RWS`'daki `CAttributeHandler` ve `CSystemCommands` çiftlerin
  yaratılması her kaydın içinde yapılır; onlar factory' dışında tutulur.

## 6. Sonuç

| Boyut | Değer |
|---|---|
| Binary 50 sınıf kaydından, port olarak 35 kayıt | %70 |
| .RWS'da görünen 26 benzersiz davranıştan portta 13'ü bulunur | %50 |
| 3 isim uyuşmazlığı (`CSoldier`, `CWbCamera`, `CWbRender`) | geçersiz |
| `WorldSpawn_FromFile` → `97 created, 152 skipped` — sadece 4 kayıttan oluşturulur | runtime etkisi |
| Hiçbiri graceful-crash yaratmaz — sadece boş davranışlar atlanır | çalışır gösterir ama verimsiz |

Bu audit'ten sonra yapılacak mantıklı adım: önce isim uyumsuzluklarını
(doğrudan) düzeltmek (`CWbRender`/`CWbCamera` → `CWbRenderBehavior`/
`CWbCameraBehavior`, `CSoldier` kaydını ise yalnız binary adına benzet
olarak `CSoldierAi` ile değiştirmek), sonra `GameBehaviours_Register()`'a
6–8 kritik davranışı eklemek (`CTowerCatapult`, `CCollisionTrigger`,
`CHealthDebris`, `CFernandoActor`, `CMenuBehavior`). Bu adımlar küçük;
beklenen kazanım net: `instancesCreated` artar, `instancesSkipped` düşer.
