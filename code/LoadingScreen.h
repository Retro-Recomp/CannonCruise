// ============================================================================
// CannonCruise - Loading Screen (LoadingScreen.h)
// Faz 4.6ab — GLOBAL.RWS'taki Loading.anm ortamı: parchment şeridi, dalga
// bordürü, Hugo kayığı sağa ilerlerken "LOADING" metni (SCR.mov frames 4/7).
//
// Global boot sonrası, dünya AddClump edilmeden ÖNCE birkaç saniye gösterilir.
// Art (Load_NT_001/003/005, rwID_2DMAESTRO dokusu) GLOBAL boot fazında
// yüklenmiş olur; yoksa fallback çizim (koyu zemin + bitmap metin) devreye
// girer. CANNONCRUISE_LOADING_FRAMES=0 → her zaman atla.
// ============================================================================
#pragma once

#include "rwcore.h"

/// Yükleme ekranını `frames` kare boyunca çizer. Esc/Space/Enter veya pencere
/// kapanırsa erken sonlanır.
/// @return FALSE → uygulama kapanmalı; TRUE → devam (dünya setup'ı başlar).
RwBool LoadingScreen_Play(int frames, int screenW, int screenH);
