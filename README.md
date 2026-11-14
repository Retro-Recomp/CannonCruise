# CannonCruise

CannonCruisePC oyununun tersine mühendislikle yeniden inşa edilen kaynak kodu.
Orijinal 2004 tarihli 32-bit Windows oyunu (`CannonCruise.exe`) Ghidra ve IDA
analizleri (`CC.asm`, `CC_ida.asm`) temel alınarak C++14 ile yeniden yazılmaktadır.

**Durum:** Aktif geliştirme — Faz 4 (derleme/link) doğrulandı: macOS/Clang'de
`CannonCruiseLib` + link doğrulama hedefi; Windows'ta VS 18 (2026) ile Win32
derlemesi ve ctest 30/30 geçti (NULL ve D3D9 platformları). Çalışma zamanı
doğrulaması kısmi (D3D9 penceresi açılıyor, 45/45 kare; screenshot D3D9'da
henüz yok).

## Dizin yapısı

| Yol | İçerik |
|---|---|
| `code/` | Yeniden yazılmış kaynak kodu (oyun + framework) |
| `code/framework/` | RenderWare Studio framework çekirdeği (event/attribute/stream/memory) |
| `code/havok/` | Havok Physics 2.2.1 stub header'ları (Faz 4.3) |
| `code/rwstubs.cpp` | RenderWare SDK link stub'ları (Faz 4.5) |
| `code/build/` | CMake çıktıları (izlenmez) |
| `CC.asm` / `CC_ida.asm` | Ghidra / IDA tersine mühendislik dökümleri (referans) |
| `tests/` | SDK bağımsız birim testleri (ör. Havok allocator) |
| `tools/`, `validation/` | Arayüz denetim betikleri ve doğrulama JSON çıktıları |
| `DLLs/`, `Streams/`, `Sound/`, `Localiza/`, `Setup/` | Orijinal oyun dosyaları (referans/varlık) |

## Derleme

### macOS (doğrulanmış)

```bash
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/CannonCruise        # link doğrulama girişi — "link verification OK" yazmalıdır
```

- `CannonCruiseLib` statik kütüphanesi + `CannonCruise` link doğrulama hedefi üretilir.
- Hedef, `-Wl,-force_load` ile tüm arşiv nesnelerini bağlar; eksik sembol varsa
  link hatası verir. Ayrıca `CannonCruise` hedefi yalnızca sistem kütüphanelerine
  dinamik bağlanır (oyun SDK'ları henüz link edilmiyor).

### Windows (doğrulandı — VS 18 2026)

```bat
cd code
cmake -B build -G "Visual Studio 18 2026" -A Win32 -DCANNONCRUISE_RW_PLATFORM=NULL
cmake --build build --config Release --parallel 8
ctest --test-dir build -C Release --output-on-failure
```

- `CANNONCRUISE_RW_PLATFORM`: `NULL` (headless) | `D3D9` (pencereli) | `GL3`.
- **Dikkat — izlenen build çıktıları:** `code/build_win32_v180/` repo ile birlikte
  gelir ve eski (VS 17 2022 generator'lı, D3D9) bayat bir CMakeCache içerir;
  VS 18 ile yerinde kullanamazsınız. Temiz yol: yeni bir build dizini kullanın
  (`cmake -B build_win32 ...`) ve iş bitince silin; ya da `build_win32_v180/`
  içindeki `CMakeCache.txt`/`CMakeFiles/`/`*.vcxproj*` silinip yeniden
  yapılandırılabilir (izlenen çıktılar git'te durur).
- D3D9 notu: ekran görüntüsü desteği yok (`GetRenderTargetData` ayrı iş
  kalemi); pencere 45/45 kare ile doğrulandı. ctest 30/30 (NULL ve D3D9).
- `build_cannoncruise.cmd` VS 18 recipe'i içerir ama `build_win32_v180/`
  bayat cache ile birlikte gelir — yukarıdaki temiz dizin yaklaşımını izleyin.

## Belgeler

- [ROADMAP.md](ROADMAP.md) — fazlar, doğrulama kapıları, güncel durum
- [ROADMAP_MACOS.md](ROADMAP_MACOS.md) — macOS port yol haritası
- [BUILDING.md](BUILDING.md) — Windows/MSVC derleme reçetesi ve bağımlılık envanteri
- [SDK_INTERFACE_AUDIT.md](SDK_INTERFACE_AUDIT.md) — RenderWare/Havok arayüz denetimi
- [LICENSE](LICENSE) — GPL-3.0

## Testler

SDK bağımsız testler `tests/` altındadır. Tümü CMake/ctest ile çalışır:

```bash
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

Kapsam: `havok_memory_manager` (allocator), `ceventhandler` (RegisterMsg/
LinkMsg/SendMsg/yaşam döngüsü), `cclassfactory` (Register/Create/Shutdown),
`cattributehandler` (instance registry + paket öngörücüler),
`worldcycle_test` (VIKING01/IDUNA03/BEAVER01 load→clump→unload stresi,
Faz 4.6s), `fmvscene_test` (FMV geçiş grafiği, 8 tanımlayıcı). Tüm paket
**ctest 30/30** ile çalışır. Tek testi
sanitizer ile elle de derleyebilirsiniz:

```bash
c++ -std=c++14 -fsanitize=address,undefined \
    tests/havok_memory_manager_test.cpp code/HavokMemoryManager.cpp \
    -Icode -o /tmp/havok_test && /tmp/havok_test
```

## Mimari notlar

- **RenderWare 3.6 / RenderWare Studio:** Yerel stub başlıklar (`code/rwcore.h`
  vb.) bildirim sağlar; gerçek SDK kütüphaneleri bağlanmamıştır. Oyun kodu
  RenderWare tipleriyle (`RpClump*`, `RwFrame*` vb.) yazar.
- **Havok Physics 2.2.1:** `code/havok/` stub header'ları binary adlandırmasıyla
  (`hkWorld`, `hkRigidBody` — `hkp` öneki yok) eşleşir. `CPhysicsWorld` /
  `CPhysicsEntity` şimdilik `void*` kullanır; SDK entegrasyonu bekliyor.
- **Framework:** `RWS::CEventHandler`, `CAttributeHandler`, `CClassFactory`
  RenderWare Studio davranışını taklit eder; `FUN_*` binary adresleri kod
  yorumlarında korunur.

## Uyarı

Bu proje tersine mühendislik içerir; orijinal oyun telif hakkıyla korunur.
Depodaki orijinal oyun dosyaları yalnızca analiz/referans amaçlıdır.
